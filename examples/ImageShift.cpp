#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

#include "Configs.h"
#include "bitmap.h"

using namespace std;

const static BitmapColor<uint8_t> red(255, 0, 0);
const static BitmapColor<uint8_t> green(0, 255, 0);
const static BitmapColor<uint8_t> blue(0, 0, 255);

int main ( int argc, char **argv )
{
    Bitmap input, output;

    string inputUrl  = CMAKE_SOURCE_DIR "/images/site1.jpg";
    if (argc > 1) {  inputUrl = argv[1]; }
    if ( !input.Read(inputUrl, true) ) {
        cout 
            << "Usage:\n\t" 
            << argv[0] << "<input-image> <shift(0.0~1.0)> <output-image>\n";
        return 1;
    }

    double shift = 0.0;
    if (argc > 2) { shift = atof(argv[2]); }
    while (shift < 0) { shift += 1.0; }
    while (shift > 1) { shift -= 1.0; }

    string outputUrl = inputUrl + "_shifted.jpg";
    if (argc > 3) { outputUrl = argv[3]; }

    output.Allocate(input.Width(), input.Height(), input.IsRGB());
    const int lineBytes = (input.IsRGB() ? 3 : 1) * input.Width();
    const int offset = (input.IsRGB() ? 3 : 1) * (int)(input.Width() * shift);
    for (int i = 0; i < input.Height(); ++i) {
        const uint8_t *line = input.GetScanline(i);
        memcpy(output.GetScanline(i), line+offset, lineBytes-offset);
        memcpy(output.GetScanline(i)+(lineBytes-offset), line, offset);
    }

    output.Write(outputUrl);
    return 0;
}