#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

#include "Configs.h"
#include "bitmap.h"

using namespace std;

int main ( int argc, char **argv )
{
    Bitmap input, output;
    if (argc > 1 && (strcmp(argv[1],"-h")==0||strcmp(argv[1],"--help")==0)) {
        cout << "Usage:\n\t" << argv[0] 
            << " <input-image> [<shift(0.0~1.0,+nPixel,-nPixel)>] [<output-image>]\n";
        return -1;
    }

    string inputUrl  = CMAKE_SOURCE_DIR "/images/site1.jpg";
    if (argc > 1) {  inputUrl = argv[1]; }
    if ( !input.Read(inputUrl, true) ) {
        cout << "Error reading '" << inputUrl << "'\n";
        return 1;
    }

    double shift = 0.5;
    if (argc > 2) { shift = atof(argv[2]); }
    if (shift > 1) {
        // specify center pixel position
        shift = shift / input.Width();
        shift += 0.5;
    } else if (shift < -1) {
        // specify left pixel position
        shift = (-shift) / input.Width();
    }
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