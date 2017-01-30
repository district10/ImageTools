#include "feature_matching.h"

#include <fstream>
#include <numeric>

#include "misc.h"

size_t FindBestMatchesOneWay(const Eigen::MatrixXi& dists,
                             const float max_ratio, 
                             const float max_distance,
                             std::vector<int> &matches) {
  // SIFT descriptor vectors are normalized to length 512.
  const float kDistNorm = 1.0f / (512.0f * 512.0f);

  size_t num_matches = 0;
  matches.resize(dists.rows(), -1);

  for (Eigen::MatrixXi::Index i1 = 0; i1 < dists.rows(); ++i1) {
    int best_i2 = -1;
    int best_dist = 0;
    int second_best_dist = 0;
    for (Eigen::MatrixXi::Index i2 = 0; i2 < dists.cols(); ++i2) {
      const int dist = dists(i1, i2);
      if (dist > best_dist) {
        best_i2 = i2;
        second_best_dist = best_dist;
        best_dist = dist;
      } else if (dist > second_best_dist) {
        second_best_dist = dist;
      }
    }

    // Check if any match found.
    if (best_i2 == -1) {
      continue;
    }

    const float best_dist_normed =
        std::acos(std::min(kDistNorm * best_dist, 1.0f));

    // Check if match distance passes threshold.
    if (best_dist_normed > max_distance) {
      continue;
    }

    const float second_best_dist_normed =
        std::acos(std::min(kDistNorm * second_best_dist, 1.0f));

    // Check if match passes ratio test. Keep this comparison >= in order to
    // ensure that the case of best == second_best is detected.
    if (best_dist_normed >= max_ratio * second_best_dist_normed) {
      continue;
    }

    num_matches += 1;
    matches[i1] = best_i2;
  }

  return num_matches;
}

Eigen::MatrixXi ComputeSiftDistanceMatrix(
    const FeatureKeypoints &keypoints1, 
    const FeatureKeypoints &keypoints2,
    const FeatureDescriptors &descriptors1,
    const FeatureDescriptors &descriptors2,
    const std::function<bool(float, float, float, float)>& guided_filter) {
  if (guided_filter != nullptr) {
    CHECK_EQ(keypoints1->size(), descriptors1.rows());
    CHECK_EQ(keypoints2->size(), descriptors2.rows());
  }

  const Eigen::Matrix<int, Eigen::Dynamic, 128> descriptors1_int =
      descriptors1.cast<int>();
  const Eigen::Matrix<int, Eigen::Dynamic, 128> descriptors2_int =
      descriptors2.cast<int>();

  Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> dists(
      descriptors1.rows(), descriptors2.rows());

  for (FeatureDescriptors::Index i1 = 0; i1 < descriptors1.rows(); ++i1) {
    for (FeatureDescriptors::Index i2 = 0; i2 < descriptors2.rows(); ++i2) {
      if (guided_filter != nullptr &&
          guided_filter(keypoints1[i1].x, keypoints1[i1].y,
                        keypoints2[i2].x, keypoints2[i2].y)) {
        dists(i1, i2) = 0;
      } else {
        dists(i1, i2) = descriptors1_int.row(i1).dot(descriptors2_int.row(i2));
      }
    }
  }

  return dists;
}

void FindBestMatches(const Eigen::MatrixXi& dists, const float max_ratio,
                     const float max_distance, const bool cross_check,
                     FeatureMatches &matches) {
  matches.clear();

  std::vector<int> matches12;
  const size_t num_matches12 =
      FindBestMatchesOneWay(dists, max_ratio, max_distance, matches12);

  if (cross_check) {
    std::vector<int> matches21;
    const size_t num_matches21 = FindBestMatchesOneWay(
        dists.transpose(), max_ratio, max_distance, matches21);
    matches.reserve(std::min(num_matches12, num_matches21));
    for (size_t i1 = 0; i1 < matches12.size(); ++i1) {
      if (matches12[i1] != -1 && matches21[matches12[i1]] != -1 &&
          matches21[matches12[i1]] == static_cast<int>(i1)) {
        FeatureMatch match;
        match.point2D_idx1 = i1;
        match.point2D_idx2 = matches12[i1];
        matches.push_back(match);
      }
    }
  } else {
    matches.reserve(num_matches12);
    for (size_t i1 = 0; i1 < matches12.size(); ++i1) {
      if (matches12[i1] != -1) {
        FeatureMatch match;
        match.point2D_idx1 = i1;
        match.point2D_idx2 = matches12[i1];
        matches.push_back(match);
      }
    }
  }
}

void MatchSiftFeaturesCPU(const SiftMatchOptions& match_options,
                          const FeatureDescriptors& descriptors1,
                          const FeatureDescriptors& descriptors2,
                          FeatureMatches &matches) {
  const Eigen::MatrixXi dists = ComputeSiftDistanceMatrix(
      FeatureKeypoints(), FeatureKeypoints(),
      descriptors1, descriptors2, nullptr);

  FindBestMatches(dists, match_options.max_ratio, match_options.max_distance,
                  match_options.cross_check, matches);
}


