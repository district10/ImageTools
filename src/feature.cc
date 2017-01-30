#include "feature.h"

#include "misc.h"

std::vector<Eigen::Vector2d> FeatureKeypointsToPointsVector(
    const FeatureKeypoints& keypoints) {
  std::vector<Eigen::Vector2d> points(keypoints.size());
  for (size_t i = 0; i < keypoints.size(); ++i) {
    points[i] = Eigen::Vector2d(keypoints[i].x, keypoints[i].y);
  }
  return points;
}

Eigen::MatrixXf L2NormalizeFeatureDescriptors(
    const Eigen::MatrixXf& descriptors) {
  return descriptors.rowwise().normalized();
}

Eigen::MatrixXf L1RootNormalizeFeatureDescriptors(
    const Eigen::MatrixXf& descriptors) {
  Eigen::MatrixXf descriptors_normalized(descriptors.rows(),
                                         descriptors.cols());
  for (Eigen::MatrixXf::Index r = 0; r < descriptors.rows(); ++r) {
    const float norm = descriptors.row(r).lpNorm<1>();
    descriptors_normalized.row(r) = descriptors.row(r) / norm;
    descriptors_normalized.row(r) =
        descriptors_normalized.row(r).array().sqrt();
  }
  return descriptors_normalized;
}

FeatureDescriptors FeatureDescriptorsToUnsignedByte(
    const Eigen::MatrixXf& descriptors) {
  FeatureDescriptors descriptors_unsigned_byte(descriptors.rows(),
                                               descriptors.cols());
  for (Eigen::MatrixXf::Index r = 0; r < descriptors.rows(); ++r) {
    for (Eigen::MatrixXf::Index c = 0; c < descriptors.cols(); ++c) {
      const float scaled_value = std::round(512.0f * descriptors(r, c));
      descriptors_unsigned_byte(r, c) =
          TruncateCast<float, uint8_t>(scaled_value);
    }
  }
  return descriptors_unsigned_byte;
}

FeatureDescriptors ExtractTopScaleDescriptors(
    const FeatureKeypoints& keypoints, const FeatureDescriptors& descriptors,
    const size_t num_features) {
  CHECK_EQ(keypoints.size(), descriptors.rows());
  CHECK_GT(num_features, 0);

  FeatureDescriptors top_scale_descriptors;

  if (static_cast<size_t>(descriptors.rows()) <= num_features) {
    top_scale_descriptors = descriptors;
  } else {
    std::vector<std::pair<size_t, float>> scales;
    scales.reserve(static_cast<size_t>(keypoints.size()));
    for (size_t i = 0; i < keypoints.size(); ++i) {
      scales.emplace_back(i, keypoints[i].scale);
    }

    std::partial_sort(scales.begin(), scales.begin() + num_features,
                      scales.end(), [](const std::pair<size_t, float> scale1,
                                       const std::pair<size_t, float> scale2) {
                        return scale1.second > scale2.second;
                      });

    top_scale_descriptors.resize(num_features, descriptors.cols());
    for (size_t i = 0; i < num_features; ++i) {
      top_scale_descriptors.row(i) = descriptors.row(scales[i].first);
    }
  }

  return top_scale_descriptors;
}