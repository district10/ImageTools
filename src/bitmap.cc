#include "bitmap.h"

#include <unordered_map>
#include <algorithm> 
#include <utility> 

#include "VLFeat/imopv.h"
#include "misc.h"

Bitmap::Bitmap()
    : data_(nullptr, &FreeImage_Unload), width_(0), height_(0), channels_(0) {}

Bitmap::Bitmap(const Bitmap& other) : Bitmap() {
  SetPtr(FreeImage_Clone(data_.get()));
}

Bitmap::Bitmap(FIBITMAP* data) : Bitmap() { SetPtr(data); }

bool Bitmap::Allocate(const int width, const int height, const bool as_rgb) {
  FIBITMAP* data = nullptr;
  width_ = width;
  height_ = height;
  if (as_rgb) {
    const int kNumBitsPerPixel = 24;
    data = FreeImage_Allocate(width, height, kNumBitsPerPixel);
    channels_ = 3;
  } else {
    const int kNumBitsPerPixel = 8;
    data = FreeImage_Allocate(width, height, kNumBitsPerPixel);
    channels_ = 1;
  }
  data_ = FIBitmapPtr(data, &FreeImage_Unload);
  return data != nullptr;
}

std::vector<uint8_t> Bitmap::ConvertToRawBits() const {
  const unsigned int scan_width = ScanWidth();
  const unsigned int bpp = BitsPerPixel();
  const bool kTopDown = true;
  std::vector<uint8_t> raw_bits(bpp * scan_width * height_, 0);
  FreeImage_ConvertToRawBits(raw_bits.data(), data_.get(), scan_width, bpp,
                             FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK,
                             FI_RGBA_BLUE_MASK, kTopDown);
  return raw_bits;
}

std::vector<uint8_t> Bitmap::ConvertToRowMajorArray() const {
  std::vector<uint8_t> array(width_ * height_ * channels_);
  size_t i = 0;
  for (int y = 0; y < height_; ++y) {
    const uint8_t* line = FreeImage_GetScanLine(data_.get(), height_ - 1 - y);
    for (int x = 0; x < width_; ++x) {
      for (int d = 0; d < channels_; ++d) {
        array[i] = line[x * channels_ + d];
        i += 1;
      }
    }
  }
  return array;
}

std::vector<uint8_t> Bitmap::ConvertToColMajorArray() const {
  std::vector<uint8_t> array(width_ * height_ * channels_);
  size_t i = 0;
  for (int d = 0; d < channels_; ++d) {
    for (int x = 0; x < width_; ++x) {
      for (int y = 0; y < height_; ++y) {
        const uint8_t* line =
            FreeImage_GetScanLine(data_.get(), height_ - 1 - y);
        array[i] = line[x * channels_ + d];
        i += 1;
      }
    }
  }
  return array;
}

bool Bitmap::GetPixel(const int x, const int y,
                      BitmapColor<uint8_t>* color) const {
  if (x < 0 || x >= width_ || y < 0 || y >= height_) {
    return false;
  }

  const uint8_t* line = FreeImage_GetScanLine(data_.get(), height_ - 1 - y);

  if (IsGrey()) {
    color->r = line[x];
    return true;
  } else if (IsRGB()) {
    color->r = line[3 * x + FI_RGBA_RED];
    color->g = line[3 * x + FI_RGBA_GREEN];
    color->b = line[3 * x + FI_RGBA_BLUE];
    return true;
  }

  return false;
}

bool Bitmap::SetPixel(const int x, const int y,
                      const BitmapColor<uint8_t>& color) {
  if (x < 0 || x >= width_ || y < 0 || y >= height_) {
    return false;
  }

  uint8_t* line = FreeImage_GetScanLine(data_.get(), height_ - 1 - y);

  if (IsGrey()) {
    line[x] = color.r;
    return true;
  } else if (IsRGB()) {
    line[3 * x + FI_RGBA_RED] = color.r;
    line[3 * x + FI_RGBA_GREEN] = color.g;
    line[3 * x + FI_RGBA_BLUE] = color.b;
    return true;
  }

  return false;
}

void Bitmap::DrawLine(int x0, int y0, int x1, int y1, const BitmapColor<uint8_t>& color)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    if (dx == 0) {
        if (dy < 0) { std::swap(y0, y1); }
        for (int y = y0; y <= y1; ++y) {
            SetPixel(x0, y, color);
        }
        return;
    }
    if (dx < 0) { 
        std::swap(x0, x1); 
        std::swap(y0, y1); 
    }
    double deltaerr = fabs((double)dy / dx);
    double error = deltaerr - 0.5;
    for (int x = x0, y = y0; x <= x1; ++x) {
        SetPixel(x, y, color);
        error += deltaerr;
        if (error >= 0.5) {
            ++y;
            error -= 1.0;
        }
    }
}

void Bitmap::DrawPoint(int x, int y, const BitmapColor<uint8_t>& color, int halfwidth)
{
    int hw = halfwidth > 1 ? halfwidth : 1;
    for (int j = -hw; j <= hw; ++j) {
        for (int k = -hw; k <= hw; ++k) {
            SetPixel(x + j, y + k, color);
        }
    }
}

void Bitmap::DrawPoints(const FeatureKeypoints &points, const BitmapColor<uint8_t>& color)
{
    for (int i = 0; i < points.size(); ++i) {
        int x = (int)points[i].x;
        int y = (int)points[i].y;
        DrawPoint(x, y, color);
    }
}

uint8_t *Bitmap::GetScanline(const int y) {
  CHECK_GE(y, 0);
  CHECK_LT(y, height_);
  return FreeImage_GetScanLine(data_.get(), height_ - 1 - y);
}

void Bitmap::Fill(const BitmapColor<uint8_t>& color) {
  for (int y = 0; y < height_; ++y) {
    uint8_t* line = FreeImage_GetScanLine(data_.get(), height_ - 1 - y);
    for (int x = 0; x < width_; ++x) {
      if (IsGrey()) {
        line[x] = color.r;
      } else if (IsRGB()) {
        line[3 * x + FI_RGBA_RED] = color.r;
        line[3 * x + FI_RGBA_GREEN] = color.g;
        line[3 * x + FI_RGBA_BLUE] = color.b;
      }
    }
  }
}

bool Bitmap::InterpolateNearestNeighbor(const double x, const double y,
                                        BitmapColor<uint8_t>* color) const {
  const int xx = static_cast<int>(std::round(x));
  const int yy = static_cast<int>(std::round(y));
  return GetPixel(xx, yy, color);
}

bool Bitmap::InterpolateBilinear(const double x, const double y,
                                 BitmapColor<float>* color) const {
  // FreeImage's coordinate system origin is in the lower left of the image.
  const double inv_y = height_ - 1 - y;

  const int x0 = static_cast<int>(std::floor(x));
  const int x1 = x0 + 1;
  const int y0 = static_cast<int>(std::floor(inv_y));
  const int y1 = y0 + 1;

  if (x0 < 0 || x1 >= width_ || y0 < 0 || y1 >= height_) {
    return false;
  }

  const double dx = x - x0;
  const double dy = inv_y - y0;
  const double dx_1 = 1 - dx;
  const double dy_1 = 1 - dy;

  const uint8_t* line0 = FreeImage_GetScanLine(data_.get(), y0);
  const uint8_t* line1 = FreeImage_GetScanLine(data_.get(), y1);

  if (IsGrey()) {
    // Top row, column-wise linear interpolation.
    const double v0 = dx_1 * line0[x0] + dx * line0[x1];

    // Bottom row, column-wise linear interpolation.
    const double v1 = dx_1 * line1[x0] + dx * line1[x1];

    // Row-wise linear interpolation.
    color->r = dy_1 * v0 + dy * v1;
    return true;
  } else if (IsRGB()) {
    const uint8_t* p00 = &line0[3 * x0];
    const uint8_t* p01 = &line0[3 * x1];
    const uint8_t* p10 = &line1[3 * x0];
    const uint8_t* p11 = &line1[3 * x1];

    // Top row, column-wise linear interpolation.
    const double v0_r = dx_1 * p00[FI_RGBA_RED] + dx * p01[FI_RGBA_RED];
    const double v0_g = dx_1 * p00[FI_RGBA_GREEN] + dx * p01[FI_RGBA_GREEN];
    const double v0_b = dx_1 * p00[FI_RGBA_BLUE] + dx * p01[FI_RGBA_BLUE];

    // Bottom row, column-wise linear interpolation.
    const double v1_r = dx_1 * p10[FI_RGBA_RED] + dx * p11[FI_RGBA_RED];
    const double v1_g = dx_1 * p10[FI_RGBA_GREEN] + dx * p11[FI_RGBA_GREEN];
    const double v1_b = dx_1 * p10[FI_RGBA_BLUE] + dx * p11[FI_RGBA_BLUE];

    // Row-wise linear interpolation.
    color->r = dy_1 * v0_r + dy * v1_r;
    color->g = dy_1 * v0_g + dy * v1_g;
    color->b = dy_1 * v0_b + dy * v1_b;
    return true;
  }

  return false;
}

bool Bitmap::Read(const std::string& path, const bool as_rgb) {
    // check file exists

  const FREE_IMAGE_FORMAT format = FreeImage_GetFileType(path.c_str(), 0);

  if (format == FIF_UNKNOWN) {
    return false;
  }

  FIBITMAP* fi_bitmap = FreeImage_Load(format, path.c_str());
  data_ = FIBitmapPtr(fi_bitmap, &FreeImage_Unload);

  const FREE_IMAGE_COLOR_TYPE color_type = FreeImage_GetColorType(fi_bitmap);

  const bool is_grey =
      color_type == FIC_MINISBLACK && FreeImage_GetBPP(fi_bitmap) == 8;
  const bool is_rgb =
      color_type == FIC_RGB && FreeImage_GetBPP(fi_bitmap) == 24;

  if (!is_rgb && as_rgb) {
    FIBITMAP* converted_bitmap = FreeImage_ConvertTo24Bits(fi_bitmap);
    data_ = FIBitmapPtr(converted_bitmap, &FreeImage_Unload);
  } else if (!is_grey && !as_rgb) {
    FIBITMAP* converted_bitmap = FreeImage_ConvertToGreyscale(fi_bitmap);
    data_ = FIBitmapPtr(converted_bitmap, &FreeImage_Unload);
  }

  width_ = FreeImage_GetWidth(data_.get());
  height_ = FreeImage_GetHeight(data_.get());
  channels_ = as_rgb ? 3 : 1;

  return true;
}

bool Bitmap::Write(const std::string& path, const FREE_IMAGE_FORMAT format,
                   const int flags) const {
  FREE_IMAGE_FORMAT save_format;
  if (format == FIF_UNKNOWN) {
    save_format = FreeImage_GetFIFFromFilename(path.c_str());
  } else {
    save_format = format;
  }

  int save_flags = flags;
  if (save_format == FIF_JPEG && flags == 0) {
    // Use superb JPEG quality by default to avoid artifacts.
    save_flags = JPEG_QUALITYSUPERB;
  }

  bool success = false;
  if (save_flags == 0) {
    success = FreeImage_Save(save_format, data_.get(), path.c_str());
  } else {
    success =
        FreeImage_Save(save_format, data_.get(), path.c_str(), save_flags);
  }

  return success;
}

void Bitmap::Smooth(const float sigma_x, const float sigma_y) {
  std::vector<float> array(width_ * height_);
  std::vector<float> array_smoothed(width_ * height_);
  for (int d = 0; d < channels_; ++d) {
    size_t i = 0;
    for (int y = 0; y < height_; ++y) {
      const uint8_t* line = FreeImage_GetScanLine(data_.get(), height_ - 1 - y);
      for (int x = 0; x < width_; ++x) {
        array[i] = line[x * channels_ + d];
        i += 1;
      }
    }

    vl_imsmooth_f(array_smoothed.data(), width_, array.data(), width_, height_,
                  width_, sigma_x, sigma_y);

    i = 0;
    for (int y = 0; y < height_; ++y) {
      uint8_t* line = FreeImage_GetScanLine(data_.get(), height_ - 1 - y);
      for (int x = 0; x < width_; ++x) {
        line[x * channels_ + d] =
            TruncateCast<float, uint8_t>(array_smoothed[i]);
        i += 1;
      }
    }
  }
}

void Bitmap::Rescale(const int new_width, const int new_height,
                     const FREE_IMAGE_FILTER filter) {
  SetPtr(FreeImage_Rescale(data_.get(), new_width, new_height, filter));
}

Bitmap Bitmap::Clone() const { return Bitmap(FreeImage_Clone(data_.get())); }

Bitmap Bitmap::CloneAsGrey() const {
  if (IsGrey()) {
    return Clone();
  } else {
    return Bitmap(FreeImage_ConvertToGreyscale(data_.get()));
  }
}

Bitmap Bitmap::CloneAsRGB() const {
  if (IsRGB()) {
    return Clone();
  } else {
    return Bitmap(FreeImage_ConvertTo24Bits(data_.get()));
  }
}

void Bitmap::SetPtr(FIBITMAP* data) {
  data_ = FIBitmapPtr(data, &FreeImage_Unload);

  width_ = FreeImage_GetWidth(data);
  height_ = FreeImage_GetHeight(data);

  const FREE_IMAGE_COLOR_TYPE color_type = FreeImage_GetColorType(data);

  const bool is_grey =
      color_type == FIC_MINISBLACK && FreeImage_GetBPP(data) == 8;
  const bool is_rgb = color_type == FIC_RGB && FreeImage_GetBPP(data) == 24;

  if (!is_grey && !is_rgb) {
    FIBITMAP* data_converted = FreeImage_ConvertTo24Bits(data);
    data_ = FIBitmapPtr(data_converted, &FreeImage_Unload);
    channels_ = 3;
  } else {
    channels_ = is_rgb ? 3 : 1;
  }
}

float JetColormap::Red(const float gray) { return Base(gray - 0.25f); }

float JetColormap::Green(const float gray) { return Base(gray); }

float JetColormap::Blue(const float gray) { return Base(gray + 0.25f); }

float JetColormap::Base(const float val) {
  if (val <= 0.125f) {
    return 0.0f;
  } else if (val <= 0.375f) {
    return Interpolate(2.0f * val - 1.0f, 0.0f, -0.75f, 1.0f, -0.25f);
  } else if (val <= 0.625f) {
    return 1.0f;
  } else if (val <= 0.87f) {
    return Interpolate(2.0f * val - 1.0f, 1.0f, 0.25f, 0.0f, 0.75f);
  } else {
    return 0.0f;
  }
}

float JetColormap::Interpolate(const float val, const float y0, const float x0,
                               const float y1, const float x1) {
  return (val - x0) * (y1 - y0) / (x1 - x0) + y0;
}