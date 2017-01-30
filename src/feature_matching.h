#ifndef COLMAP_SRC_BASE_FEATURE_MATCHING_H_
#define COLMAP_SRC_BASE_FEATURE_MATCHING_H_

#include <array>
#include <string>
#include <unordered_set>
#include <vector>

#include "feature.h"

struct SiftMatchOptions;

// Match the given SIFT features on the CPU.
void MatchSiftFeaturesCPU(const SiftMatchOptions& match_options,
                          const FeatureDescriptors& descriptors1,
                          const FeatureDescriptors& descriptors2,
                          FeatureMatches &matches);

struct SiftMatchOptions {
  // Maximum distance ratio between first and second best match.
  double max_ratio = 0.8;

  // Maximum distance to best match.
  double max_distance = 0.7;

  // Whether to enable cross checking in matching.
  bool cross_check = true;

  // Maximum number of matches.
  int max_num_matches = 32768;

  // Maximum epipolar error in pixels for geometric verification.
  double max_error = 4.0;

  // Confidence threshold for geometric verification.
  double confidence = 0.999;

  // Minimum/maximum number of RANSAC iterations. Note that this option
  // overrules the min_inlier_ratio option.
  int min_num_trials = 30;
  int max_num_trials = 10000;

  // A priori assumed minimum inlier ratio, which determines the maximum
  // number of iterations.
  double min_inlier_ratio = 0.25;

  // Minimum number of inliers for an image pair to be considered as
  // geometrically verified.
  int min_num_inliers = 15;

  // Whether to attempt to estimate multiple geometric models per image pair.
  bool multiple_models = false;

  // Whether to perform guided matching, if geometric verification succeeds.
  bool guided_matching = false;
};

#endif  // COLMAP_SRC_BASE_FEATURE_MATCHING_H_