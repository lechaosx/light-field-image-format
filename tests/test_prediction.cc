#include <gtest/gtest.h>

#include <block_predictor.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

TEST(BlockPredictor, ForwardAndBackwardPassRestoreAdjacentBlock) {
  BlockPredictor<2, int32_t> predictor({4, 2});

  DynamicBlock<int32_t, 2> first({2, 2});
  first.fill(12);
  predictor.backwardPass(first, {0, 0}, PredictionType<2> {});

  DynamicBlock<int32_t, 2> adjacent({2, 2});
  adjacent.fill(12);
  const DynamicBlock<int32_t, 2> original = adjacent;
  const PredictionType<2> dc_prediction {1, {}};
  predictor.forwardPass(adjacent, {2, 0}, dc_prediction);
  predictor.backwardPass(adjacent, {2, 0}, dc_prediction);

  for (size_t value = 0; value < adjacent.stride(2); ++value) {
    EXPECT_EQ(adjacent[value], original[value]);
  }
}

TEST(BlockPredictor, ScoresSixteenBitRangeWithoutOverflow) {
  BlockPredictor<2, int32_t> predictor({4, 2});
  DynamicBlock<int32_t, 2> first({2, 2});
  first.fill(65535);
  predictor.backwardPass(first, {0, 0}, PredictionType<2> {});

  DynamicBlock<int32_t, 2> adjacent({2, 2});
  adjacent.fill(65535);
  const PredictionType<2> selected = predictor.selectPredictionType(adjacent, {2, 0});

  EXPECT_EQ(selected.type, 1);
}

TEST(BlockPredictor, AveragesLargeFourDimensionalBoundaryWithoutOverflow) {
  const std::array<size_t, 4> size {26, 26, 26, 26};
  auto input = [](const std::array<int64_t, 4> &) { return int32_t {32767}; };

  EXPECT_EQ((predict_DC<4, int32_t>(size, input)), 32767);
}

TEST(BlockPredictor, RejectsReconstructionOverflow) {
  BlockPredictor<2, int32_t> predictor({2, 1});
  DynamicBlock<int32_t, 2> first({1, 1});
  first[0] = std::numeric_limits<int32_t>::max();
  predictor.backwardPass(first, {0, 0}, PredictionType<2> {});

  DynamicBlock<int32_t, 2> adjacent({1, 1});
  adjacent[0] = 1;
  const PredictionType<2> dc_prediction {1, {}};

  EXPECT_THROW(
      predictor.backwardPass(adjacent, {1, 0}, dc_prediction),
      std::overflow_error);
}

TEST(Prediction, InterpolatesFullRangeValuesWithoutOverflow) {
  const std::array<size_t, 1> size {2};
  const std::array<int64_t, 1> position {1};
  const auto input = [](size_t) {
    return std::numeric_limits<int32_t>::max();
  };
  int32_t output {};

  interpolate<1>(size, input, position, 2, output);

  EXPECT_EQ(output, std::numeric_limits<int32_t>::max());
}

TEST(Prediction, FiltersFullRangeValuesWithoutOverflow) {
  DynamicBlock<int32_t, 1> values({2});
  values.fill(std::numeric_limits<int32_t>::max());

  low_pass_filter(values);

  EXPECT_EQ(values[0], std::numeric_limits<int32_t>::max());
  EXPECT_EQ(values[1], std::numeric_limits<int32_t>::max());
}

TEST(Prediction, AveragesFullRangePlanarValuesWithoutOverflow) {
  DynamicBlock<int32_t, 2> output({1, 1});
  const auto input = [](const std::array<int64_t, 2> &) {
    return std::numeric_limits<int32_t>::max();
  };

  predict_planar<2>(output, input);

  EXPECT_EQ(output[0], std::numeric_limits<int32_t>::max());
}
