#ifndef COLMAP_SRC_BASE_FEATURE_EXTRACTION_H_
#define COLMAP_SRC_BASE_FEATURE_EXTRACTION_H_

#include <string>
#include <vector>

#include "bitmap.h"
#include "feature.h"
#include "misc.h"

struct SiftOptions;

// Extract SIFT features for the given image on the CPU.
bool ExtractSiftFeaturesCPU( const Bitmap &bitmap, 
                            FeatureKeypoints &keypoints,
                            FeatureDescriptors &descriptors,
                            const SiftOptions &sift_options );

struct SiftOptions {
  // Maximum image size, otherwise image will be down-scaled.
  int max_image_size = 3200;

  // Maximum number of features to detect, keeping larger-scale features.
  int max_num_features = 8192;

  // First octave in the pyramid, i.e. -1 upsamples the image by one level.
  int first_octave = -1;

  // Number of octaves.
  int num_octaves = 4;

  // Number of levels per octave.
  int octave_resolution = 3;

  // Peak threshold for detection.
  double peak_threshold = 0.02 / octave_resolution;

  // Edge threshold for detection.
  double edge_threshold = 10.0;

  // Maximum number of orientations per keypoint.
  int max_num_orientations = 2;

  // Fix the orientation to 0 for upright features.
  bool upright = false;

  // Whether to adapt the feature detection depending on the image darkness.
  // Note that this feature is only available in the OpenGL SiftGPU version.
  bool darkness_adaptivity = true;

  enum class Normalization {
    // L1-normalizes each descriptor followed by element-wise square rooting.
    // This normalization is usually better than standard L2-normalization.
    // See "Three things everyone should know to improve object retrieval",
    // Relja Arandjelovic and Andrew Zisserman, CVPR 2012.
    L1_ROOT,
    // Each vector is L2-normalized.
    L2,
  };
  Normalization normalization = Normalization::L1_ROOT;
};

#endif  // COLMAP_SRC_BASE_FEATURE_EXTRACTION_H_