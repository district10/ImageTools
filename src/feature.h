#ifndef COLMAP_SRC_BASE_FEATURE_H_
#define COLMAP_SRC_BASE_FEATURE_H_

#include <vector>
#include <Eigen/Core>

struct FeatureKeypoint {
  // Location of the feature, with the origin at the upper left image corner,
  // i.e. the upper left pixel has the coordinate (0.5, 0.5).
  float x = 0.0f;
  float y = 0.0f;

  // Shape of the feature.
  float scale = 0.0f;
  float orientation = 0.0f;
};

// 特征点包括位置、方向以及尺度，所有特征点存在一个 vector 里
typedef std::vector<FeatureKeypoint> FeatureKeypoints;
// 特征点的特征则是一串数字，所有特征点描述存成一个矩阵，每行是一个 feature descriptor
typedef Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
    FeatureDescriptors;

// 可以输出为这样的格式
// Load keypoints and descriptors from text file in the following format:
//
//    LINE_0:            NUM_FEATURES DIM
//    LINE_1:            X Y SCALE ORIENTATION D_1 D_2 D_3 ... D_DIM
//    LINE_I:            ...
//    LINE_NUM_FEATURES: X Y SCALE ORIENTATION D_1 D_2 D_3 ... D_DIM
//
// where the first line specifies the number of features and the descriptor
// dimensionality followed by one line per feature: X, Y, SCALE, ORIENTATION are
// of type float and D_J represent the descriptor in the range [0, 255].
//
// For example:
//
//    2 4
//    0.32 0.12 1.23 1.0 1 2 3 4
//    0.32 0.12 1.23 1.0 1 2 3 4
//

// Index per image, i.e. determines maximum number of 2D points per image.
typedef uint32_t point2D_t;
const point2D_t kInvalidPoint2DIdx = std::numeric_limits<point2D_t>::max();

struct FeatureMatch {
  // Feature index in first image.
  point2D_t point2D_idx1 = kInvalidPoint2DIdx;

  // Feature index in second image.
  point2D_t point2D_idx2 = kInvalidPoint2DIdx;
};

// 一对一对的匹配点
typedef std::vector<FeatureMatch> FeatureMatches;

// Convert feature keypoints to vector of points.
std::vector<Eigen::Vector2d> FeatureKeypointsToPointsVector(
    const FeatureKeypoints& keypoints);

// L2-normalize feature descriptor, where each row represents one feature.
Eigen::MatrixXf L2NormalizeFeatureDescriptors(
    const Eigen::MatrixXf& descriptors);

// L1-Root-normalize feature descriptors, where each row represents one feature.
// See "Three things everyone should know to improve object retrieval",
// Relja Arandjelovic and Andrew Zisserman, CVPR 2012.
Eigen::MatrixXf L1RootNormalizeFeatureDescriptors(
    const Eigen::MatrixXf& descriptors);

// Convert normalized floating point feature descriptor to unsigned byte
// representation by linear scaling from range [0, 0.5] to [0, 255]. Truncation
// to a maximum value of 0.5 is used to avoid precision loss and follows the
// common practice of representing SIFT vectors.
FeatureDescriptors FeatureDescriptorsToUnsignedByte(
    const Eigen::MatrixXf& descriptors);

// Extract the descriptors corresponding to the largest-scale features.
FeatureDescriptors ExtractTopScaleDescriptors(
    const FeatureKeypoints& keypoints, const FeatureDescriptors& descriptors,
    const size_t num_features);

#endif  // COLMAP_SRC_BASE_FEATURE_H_