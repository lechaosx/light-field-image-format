#include <gtest/gtest.h>

#include <ppm.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace {

class TemporaryFile {
public:
  TemporaryFile() {
    char pattern[] = "/tmp/lfif-ppm-XXXXXX";
    const int descriptor = mkstemp(pattern);
    if (descriptor < 0) {
      throw std::runtime_error("mkstemp failed");
    }
    close(descriptor);
    path_ = pattern;
  }

  explicit TemporaryFile(const std::string &contents): TemporaryFile() {
    std::ofstream(path_, std::ios::binary | std::ios::trunc).write(contents.data(), contents.size());
  }

  ~TemporaryFile() {
    std::filesystem::remove(path_);
  }

  const std::string &path() const {
    return path_;
  }

private:
  std::string path_;
};

} // namespace

TEST(Ppm, CreatesAndMapsEightBitPixels) {
  TemporaryFile file;
  {
    PPM output;
    ASSERT_EQ(output.createPPM(file.path().c_str(), 2, 1, 255), 0);
    output.put(0, {1, 2, 3});
    output.put(1, {253, 254, 255});
  }

  PPM input;
  ASSERT_EQ(input.mmapPPM(file.path().c_str()), 0);
  EXPECT_EQ(input.width(), 2U);
  EXPECT_EQ(input.height(), 1U);
  EXPECT_EQ(input.color_depth(), 255U);
  EXPECT_EQ(input.get(0), (std::array<uint16_t, 3> {1, 2, 3}));
  EXPECT_EQ(input.get(1), (std::array<uint16_t, 3> {253, 254, 255}));
}

TEST(Ppm, CreatesAndMoveAssignsSixteenBitPixels) {
  TemporaryFile file;
  PPM output;
  ASSERT_EQ(output.createPPM(file.path().c_str(), 1, 1, 65535), 0);
  output.put(0, {0x0102, 0x7fff, 0xfffe});

  PPM moved;
  moved = std::move(output);
  EXPECT_EQ(moved.get(0), (std::array<uint16_t, 3> {0x0102, 0x7fff, 0xfffe}));
}

TEST(Ppm, RejectsTruncatedPixelData) {
  TemporaryFile file("P6\n2 1\n255\n\x01\x02\x03");
  PPM ppm;
  EXPECT_LT(ppm.mmapPPM(file.path().c_str()), 0);
}

TEST(Ppm, RejectsOversizedHeaderNumberWithoutThrowing) {
  TemporaryFile file("P6\n999999999999999999999999999999999999999 1\n255\n" + std::string(1, '\0'));
  PPM ppm;
  EXPECT_NO_THROW(EXPECT_LT(ppm.mmapPPM(file.path().c_str()), 0));
}
