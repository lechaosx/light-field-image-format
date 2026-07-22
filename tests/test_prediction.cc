#include <gtest/gtest.h>

#include <block_predictor.h>

#include <array>
#include <cstddef>
#include <cstdint>

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
