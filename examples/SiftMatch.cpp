#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

#include "Configs.h"
#include "bitmap.h"
#include "feature.h"
#include "feature_extraction.h"
#include "feature_matching.h"

using namespace std;

const static BitmapColor<uint8_t> red(255, 0, 0);
const static BitmapColor<uint8_t> green(0, 255, 0);
const static BitmapColor<uint8_t> blue(0, 0, 255);

int main ( int argc, char **argv )
{
    Bitmap img1, img2;
    string imgurl1 = CMAKE_SOURCE_DIR "/images/site1.jpg";
    string imgurl2 = CMAKE_SOURCE_DIR "/images/site2.jpg";
    if (argc >= 3) {
        imgurl1 = argv[1];
        imgurl2 = argv[2];
    }
    if (
        !img1.Read(imgurl1, true) || !img2.Read(imgurl2, true) ||
        img1.Width() != img2.Width() || img1.Height() != img2.Height()
    ) {
        cout << "read image error\n";
        cout << "Usage:\n"
            << "\t" << argv[0] << "\n"
            << "\t" << argv[0] << "image1 image2\n";
        return 1;
    }
    cout << "input: " << imgurl1 << ", " << imgurl2 << "\n";

    FeatureKeypoints keypoints1, keypoints2;
    FeatureDescriptors descriptors1, descriptors2;
    SiftOptions sift_options;
    if (
        !ExtractSiftFeaturesCPU(img1, keypoints1, descriptors1, sift_options) ||
        !ExtractSiftFeaturesCPU(img2, keypoints2, descriptors2, sift_options)
    ) {
        cout << "feature extraction error\n";
        return 2;
    }
    cout << "#size: " << keypoints1.size() << ", " << keypoints2.size() << "\n";
    img1.DrawPoints(keypoints1, red);
    img2.DrawPoints(keypoints2, green);

    Bitmap img12;
    img12.Allocate(img1.Width(), img1.Height()*2, img1.IsRGB());
    const int lineBytes = (img1.IsRGB() ? 3 : 1) * img1.Width();
    for (int i = 0; i < img1.Height(); ++i) {
        const uint8_t *line1 = img1.GetScanline(i);
        const uint8_t *line2 = img2.GetScanline(i);
        memcpy(img12.GetScanline(i), line1, lineBytes);
        memcpy(img12.GetScanline(i+img1.Height()), line2, lineBytes);
    }

    FeatureMatches matches;
    MatchSiftFeaturesCPU(SiftMatchOptions(), descriptors1, descriptors2, matches);
    for (int i = 0; i < matches.size(); ++i) {
        point2D_t idx1 = matches[i].point2D_idx1;
        point2D_t idx2 = matches[i].point2D_idx2;
        if (idx1 != kInvalidPoint2DIdx && idx2 != kInvalidPoint2DIdx) {
            int x0 = keypoints1[idx1].x;
            int y0 = keypoints1[idx1].y;
            int x1 = keypoints2[idx2].x;
            int y1 = keypoints2[idx2].y + img1.Height();
            // img12.DrawLine(x0, y0, x1, y1, blue);
            // img12.DrawLine(x0, y0, x1, y1, blue);
            img12.DrawPoint(x0, y0, red);
            img12.DrawPoint(x1, y1, green);
        }
    }
    cout << "#matches: " << matches.size() << "\n";
    img12.Write("matched.jpg");

    return 0;
}
