#include "formats/cbz/cbz_decode.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {

[[noreturn]] void Fail(const std::string &message) {
  fprintf(stderr, "%s\n", message.c_str());
  std::exit(1);
}

std::vector<unsigned char> ReadFile(const char *path) {
  std::ifstream input(path, std::ios::binary);
  if (!input)
    Fail(std::string("unable to open file: ") + path);
  return std::vector<unsigned char>((std::istreambuf_iterator<char>(input)),
                                    std::istreambuf_iterator<char>());
}

void ExpectTrue(const char *label, bool value) {
  if (!value)
    Fail(std::string(label) + ": expected true");
}

void TestDecodePath(const char *path) {
  const std::vector<unsigned char> bytes = ReadFile(path);
  CbzDecodedPage decoded;
  ExpectTrue("decode", DecodeCbzPageImage(bytes, 5, 240, 400, &decoded));
  ExpectTrue("original width", decoded.original_width > 0);
  ExpectTrue("original height", decoded.original_height > 0);
  ExpectTrue("scaled width", decoded.source_bitmap.width > 0);
  ExpectTrue("scaled height", decoded.source_bitmap.height > 0);
  ExpectTrue("pixels not empty", !decoded.source_bitmap.pixels.empty());

  CbzBitmap scaled;
  ExpectTrue("rescale", ScaleCbzBitmap(decoded.source_bitmap, 4, 4, true, &scaled));
  ExpectTrue("scaled pixels not empty", !scaled.pixels.empty());
}

// Compare against the direct coordinate formula across non-integer ratios,
// single-pixel axes, upscales and downscales. Pixel values must not change.
void TestNearestScaleCoordinates() {
  const int sizes[] = {1, 2, 3, 7, 16, 31};
  for (int sw : sizes) for (int sh : sizes) {
    CbzBitmap src;
    src.width = sw; src.height = sh;
    for (int i = 0; i < sw*sh; ++i) src.pixels.push_back((uint16_t)(i*67));
    for (int dw : sizes) for (int dh : sizes) {
      CbzBitmap out;
      ExpectTrue("nearest scale", ScaleCbzBitmap(src, dw, dh, false, &out));
      ExpectTrue("nearest dimensions", out.width == dw && out.height == dh);
      for (int y = 0; y < dh; ++y) for (int x = 0; x < dw; ++x)
        ExpectTrue("nearest exact pixel", out.pixels[y*dw+x] ==
                   src.pixels[(y*sh/dh)*sw+x*sw/dw]);
    }
  }
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 3)
    Fail("usage: test_cbz_decode <sample.png> <sample.jpg>");
  TestNearestScaleCoordinates();
  TestDecodePath(argv[1]);
  TestDecodePath(argv[2]);
  return 0;
}
