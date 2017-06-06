#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdio.h>

#include "Configs.h"
#include "bitmap.h"
#include "feature.h"
#include "feature_extraction.h"
#include "feature_matching.h"

using namespace std;

const static BitmapColor<uint8_t> red(255, 0, 0);
const static BitmapColor<uint8_t> green(0, 255, 0);
const static BitmapColor<uint8_t> white(255, 255, 255);
const static BitmapColor<uint8_t> blue(0, 0, 255);

int main ( int argc, char **argv )
{
    if (argc > 1 && (argv[1] == "-h" || argv[1] == "--help")) {
        cout << "Usage:\n\t" << argv[0] << "<image1> <image2> [<output>]\n";
    }

    Bitmap img1, img2;
    string imgurl1 = CMAKE_SOURCE_DIR "/images/site1.jpg";
    string imgurl2 = CMAKE_SOURCE_DIR "/images/site2.jpg";
    string imgurl3 = "matched.jpg";
    if (argc > 1) { imgurl1 = argv[1]; }
    if (argc > 2) { imgurl2 = argv[2]; }
    if ( !img1.Read(imgurl1, true) ) {
        cout << "Error reading image '" << imgurl1 << "'\n";
        return 1;
    }
    if ( !img2.Read(imgurl2, true) ) {
        cout << "Error reading image '" << imgurl2 << "'\n";
        return 1;
    }

    FeatureKeypoints keypoints1, keypoints2;
    FeatureDescriptors descriptors1, descriptors2;
    SiftOptions sift_options;
    if (
        !ExtractSiftFeaturesCPU(img1, keypoints1, descriptors1, sift_options) ||
        !ExtractSiftFeaturesCPU(img2, keypoints2, descriptors2, sift_options)
    ) {
        cout << "Feature extraction error\n";
        return 2;
    }
    cout << "#FeaturePoints: " << keypoints1.size() << ", " << keypoints2.size() << "\n";
    img1.DrawPoints(keypoints1, blue);
    img2.DrawPoints(keypoints2, green);

    Bitmap img12;
    img12.Allocate(
        max(img1.Width(),img2.Width()), 
        img1.Height()+img2.Height(), img1.IsRGB()
    );
    const int lineBytes = (img1.IsRGB() ? 3 : 1) * img1.Width();
    for (int i = 0; i < img1.Height(); ++i) {
        const uint8_t *line1 = img1.GetScanline(i);
        const uint8_t *line2 = img2.GetScanline(i);
        memcpy(img12.GetScanline(i), line1, lineBytes);
        memcpy(img12.GetScanline(i+img1.Height()), line2, lineBytes);
    }

    FILE *ofp = fopen("matches.txt", "w");
    if (ofp) {
        fprintf(ofp, "a: %s, b: %s\n", imgurl1, imgurl2);
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
            int y1 = keypoints2[idx2].y;
            if (ofp) {
                fprintf(ofp, 
                    "%lf, %lf <-> %lf, %lf\n",
                    (double)x0/img1.Width(), (double)y0/img1.Height(),
                    (double)x1/img2.Width(), (double)y1/img2.Height()
                );
            }
            y1 += img1.Height();
            img12.DrawLine(x0, y0, x1, y1, red);
            img12.DrawLine(x0, y0, x1, y1, red);
            img12.DrawPoint(x0, y0, white);
            img12.DrawPoint(x1, y1, white);
        }
    }
    fclose(ofp);
    cout << "#matches: " << matches.size() << "\n";
    img12.Write(imgurl3);

    return 0;
}
