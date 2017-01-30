#include "feature_extraction.h"

#include <fstream>

#include "VLFeat/sift.h"
#include "feature.h"
#include "bitmap.h"
#include "misc.h"

void ScaleBitmap(const int max_image_size, double* scale_x, double* scale_y,
                 Bitmap* bitmap) {
  if (static_cast<int>(bitmap->Width()) > max_image_size ||
      static_cast<int>(bitmap->Height()) > max_image_size) {
    // Fit the down-sampled version exactly into the max dimensions
    const double scale = static_cast<double>(max_image_size) /
                         std::max(bitmap->Width(), bitmap->Height());
    const int new_width = static_cast<int>(bitmap->Width() * scale);
    const int new_height = static_cast<int>(bitmap->Height() * scale);

    // These scales differ from `scale`, if we round one of the dimensions.
    // But we want to exactly scale the keypoint locations below.
    *scale_x = static_cast<double>(new_width) / bitmap->Width();
    *scale_y = static_cast<double>(new_height) / bitmap->Height();

    bitmap->Rescale(new_width, new_height);
  } else {
    *scale_x = 1.0;
    *scale_y = 1.0;
  }
}

bool ExtractSiftFeaturesCPU( const Bitmap& bitmap,
                            FeatureKeypoints &keypoints,
                            FeatureDescriptors &descriptors,
                            const SiftOptions &options )
{
  Bitmap scaled_bitmap = bitmap.CloneAsGrey();
  double scale_x;
  double scale_y;
  ScaleBitmap(options.max_image_size, &scale_x, &scale_y, &scaled_bitmap);

  //////////////////////////////////////////////////////////////////////////////
  // Extract features
  //////////////////////////////////////////////////////////////////////////////

  const float inv_scale_x = static_cast<float>(1.0 / scale_x);
  const float inv_scale_y = static_cast<float>(1.0 / scale_y);
  const float inv_scale_xy = (inv_scale_x + inv_scale_y) / 2.0f;

  // Setup SIFT extractor.
  std::unique_ptr<VlSiftFilt, void (*)(VlSiftFilt*)> sift(
      vl_sift_new(scaled_bitmap.Width(), scaled_bitmap.Height(),
                  options.num_octaves, options.octave_resolution,
                  options.first_octave),
      &vl_sift_delete);
  if (!sift) {
    return false;
  }

  vl_sift_set_peak_thresh(sift.get(), options.peak_threshold);
  vl_sift_set_edge_thresh(sift.get(), options.edge_threshold);

  // Iterate through octaves.
  std::vector<size_t> level_num_features;
  std::vector<FeatureKeypoints> level_keypoints;
  std::vector<FeatureDescriptors> level_descriptors;
  bool first_octave = true;
  while (true) {
    if (first_octave) {
      const std::vector<uint8_t> data_uint8 =
          scaled_bitmap.ConvertToRowMajorArray();
      std::vector<float> data_float(data_uint8.size());
      for (size_t i = 0; i < data_uint8.size(); ++i) {
        data_float[i] = static_cast<float>(data_uint8[i]) / 255.0f;
      }
      if (vl_sift_process_first_octave(sift.get(), data_float.data())) {
        break;
      }
      first_octave = false;
    } else {
      if (vl_sift_process_next_octave(sift.get())) {
        break;
      }
    }

    // Detect keypoints.
    vl_sift_detect(sift.get());

    // Extract detected keypoints.
    const VlSiftKeypoint* vl_keypoints = vl_sift_get_keypoints(sift.get());
    const int num_keypoints = vl_sift_get_nkeypoints(sift.get());
    if (num_keypoints == 0) {
      continue;
    }

    // Extract features with different orientations per DOG level.
    size_t level_idx = 0;
    int prev_level = -1;
    for (int i = 0; i < num_keypoints; ++i) {
      if (vl_keypoints[i].is != prev_level) {
        if (i > 0) {
          // Resize containers of previous DOG level.
          level_keypoints.back().resize(level_idx);
          level_descriptors.back().conservativeResize(level_idx, 128);
        }

        // Add containers for new DOG level.
        level_idx = 0;
        level_num_features.push_back(0);
        level_keypoints.emplace_back(options.max_num_orientations *
                                     num_keypoints);
        level_descriptors.emplace_back(
            options.max_num_orientations * num_keypoints, 128);
      }

      level_num_features.back() += 1;
      prev_level = vl_keypoints[i].is;

      // Extract feature orientations.
      double angles[4];
      int num_orientations;
      if (options.upright) {
        num_orientations = 1;
        angles[0] = 0.0;
      } else {
        num_orientations = vl_sift_calc_keypoint_orientations(
            sift.get(), angles, &vl_keypoints[i]);
      }

      // Note that this is different from SiftGPU, which selects the top
      // global maxima as orientations while this selects the first two
      // local maxima. It is not clear which procedure is better.
      const int num_used_orientations =
          std::min(num_orientations, options.max_num_orientations);

      for (int o = 0; o < num_used_orientations; ++o) {
        level_keypoints.back()[level_idx].x = vl_keypoints[i].x + 0.5f;
        level_keypoints.back()[level_idx].y = vl_keypoints[i].y + 0.5f;
        level_keypoints.back()[level_idx].scale = vl_keypoints[i].sigma;
        level_keypoints.back()[level_idx].orientation = angles[o];

        if (scale_x != 1.0 || scale_y != 1.0) {
          level_keypoints.back()[level_idx].x *= inv_scale_x;
          level_keypoints.back()[level_idx].y *= inv_scale_y;
          level_keypoints.back()[level_idx].scale *= inv_scale_xy;
        }

        Eigen::MatrixXf desc(1, 128);
        vl_sift_calc_keypoint_descriptor(sift.get(), desc.data(),
                                         &vl_keypoints[i], angles[o]);
        if (options.normalization == SiftOptions::Normalization::L2) {
          desc = L2NormalizeFeatureDescriptors(desc);
        } else if (options.normalization ==
                   SiftOptions::Normalization::L1_ROOT) {
          desc = L1RootNormalizeFeatureDescriptors(desc);
        }
        level_descriptors.back().row(level_idx) =
            FeatureDescriptorsToUnsignedByte(desc);

        level_idx += 1;
      }
    }

    // Resize containers for last DOG level in octave.
    level_keypoints.back().resize(level_idx);
    level_descriptors.back().conservativeResize(level_idx, 128);
  }

  // Determine how many DOG levels to keep to satisfy max_num_features option.
  int first_level_to_keep = 0;
  int num_features = 0;
  int num_features_with_orientations = 0;
  for (int i = level_keypoints.size() - 1; i >= 0; --i) {
    num_features += level_num_features[i];
    num_features_with_orientations += level_keypoints[i].size();
    if (num_features > options.max_num_features) {
      first_level_to_keep = i;
      break;
    }
  }

  // Extract the features to be kept.
  size_t k = 0;
  keypoints.resize(num_features_with_orientations);
  descriptors.resize(num_features_with_orientations, 128);
  for (size_t i = first_level_to_keep; i < level_keypoints.size(); ++i) {
    for (size_t j = 0; j < level_keypoints[i].size(); ++j) {
      keypoints[k] = level_keypoints[i][j];
      descriptors.row(k) = level_descriptors[i].row(j);
      k += 1;
    }
  }

  return true;
}