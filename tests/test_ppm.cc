#include <gtest/gtest.h>

#include <ppm.h>

#include <array>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unistd.h>

namespace {

static_assert(!std::default_initializable<PPM>);
static_assert(std::movable<PPM>);
static_assert(std::same_as<
    decltype(std::declval<const PPM &>().pixels()),
    std::span<const uint8_t>>);

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
    std::error_code error;
    std::filesystem::remove(path_, error);
  }

  const std::string &path() const {
    return path_;
  }

private:
  std::string path_;
};

}

TEST(Ppm, CreatesAndMapsEightBitPixels) {
  TemporaryFile file;
  {
    PPM output = PPM::create(file.path(), 2, 1, 255);
    output.put(0, {1, 2, 3});
    output.put(1, {253, 254, 255});
    ASSERT_NO_THROW(output.flush());
  }

  PPM input = PPM::map(file.path());
  EXPECT_EQ(input.width(), 2U);
  EXPECT_EQ(input.height(), 1U);
  EXPECT_EQ(input.color_depth(), 255U);
  EXPECT_EQ(input.pixels().size(), 6U);
  EXPECT_EQ(input.get(0), (std::array<uint16_t, 3> {1, 2, 3}));
  EXPECT_EQ(input.get(1), (std::array<uint16_t, 3> {253, 254, 255}));
}

TEST(Ppm, CreatesAndMoveAssignsSixteenBitPixels) {
  TemporaryFile file;
  PPM output = PPM::create(file.path(), 1, 1, 65535);
  output.put(0, {0x0102, 0x7fff, 0xfffe});

  PPM moved = std::move(output);
  EXPECT_EQ(moved.get(0), (std::array<uint16_t, 3> {0x0102, 0x7fff, 0xfffe}));
  EXPECT_NO_THROW(moved.flush());
}

TEST(Ppm, MoveAssignmentTransfersWritableMapping) {
  TemporaryFile first_file;
  TemporaryFile second_file;
  PPM first = PPM::create(first_file.path(), 1, 1, 255);
  PPM second = PPM::create(second_file.path(), 1, 1, 255);
  second.put(0, {4, 5, 6});

  first = std::move(second);

  EXPECT_EQ(first.get(0), (std::array<uint16_t, 3> {4, 5, 6}));
  EXPECT_NO_THROW(first.flush());
}

TEST(Ppm, ReadsUnalignedSixteenBitPixels) {
  std::string contents = "P6\n1 1\n65535\n";
  contents.append("\x01\x02\x7f\xff\xff\xfe", 6);
  TemporaryFile file(contents);

  PPM input = PPM::map(file.path());
  ASSERT_NE(reinterpret_cast<uintptr_t>(input.pixels().data()) % alignof(uint16_t), 0U);
  EXPECT_EQ(input.get(0), (std::array<uint16_t, 3> {0x0102, 0x7fff, 0xfffe}));
}

TEST(Ppm, RejectsFlushingReadOnlyMapping) {
  TemporaryFile file;
  {
    PPM output = PPM::create(file.path(), 1, 1, 255);
    output.put(0, {1, 2, 3});
    output.flush();
  }

  PPM input = PPM::map(file.path());
  EXPECT_THROW(input.flush(), std::logic_error);
}

TEST(Ppm, RejectsTruncatedPixelData) {
  TemporaryFile file("P6\n2 1\n255\n\x01\x02\x03");
  EXPECT_THROW(
      [[maybe_unused]] PPM ppm = PPM::map(file.path()), std::runtime_error);
}

TEST(Ppm, RejectsSamplesAboveColorDepth) {
  TemporaryFile file("P6\n1 1\n1\nRGB");
  EXPECT_THROW(
      [[maybe_unused]] PPM ppm = PPM::map(file.path()), std::runtime_error);
}

TEST(Ppm, RejectsOversizedHeaderNumber) {
  TemporaryFile file("P6\n999999999999999999999999999999999999999 1\n255\n" + std::string(1, '\0'));
  EXPECT_THROW(
      [[maybe_unused]] PPM ppm = PPM::map(file.path()), std::runtime_error);
}

TEST(Ppm, ReportsMissingInput) {
  EXPECT_THROW(
      [[maybe_unused]] PPM ppm =
          PPM::map("/tmp/lfif-ppm-file-that-does-not-exist"),
      std::system_error);
}

TEST(Ppm, RejectsInvalidOutputDimensions) {
  TemporaryFile file;
  EXPECT_THROW(
      [[maybe_unused]] PPM ppm = PPM::create(file.path(), 0, 1, 255),
      std::invalid_argument);
}
