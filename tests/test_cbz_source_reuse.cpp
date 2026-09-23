#include "formats/cbz/cbz_decode.h"
#include "formats/common/pdf_view_utils.h"
#include "formats/common/fixed_layout_preview_constants.h"
#include "shared/fixed_layout_perf.h"
#include <algorithm>
#include <cassert>
#include <string>
#include <cstdio>
typedef uint16_t u16;
namespace debug_runtime { static bool sync = true; bool ForceSynchronousCbzDecode() { return sync; } }
namespace format_limits { const size_t kMaxCbzPageEntryBytes = 1000000; }
struct Book {
  struct CbzState {
    struct PageBitmap {
      int page = -1, zoom_index = -1, original_width = 0, original_height = 0;
      CbzBitmap bitmap;
    } current_source;
    struct Cache {
      int page = -1, zoom_index = -1, bitmap_width = 0, bitmap_height = 0;
      std::vector<u16> pixels;
    } current_preview, current_interactive;
    struct { int zoom_index = 3; } viewport;
    std::vector<int> entries = {0, 1};
    std::string archive_path, last_error;
    int target_top_width = 240, target_top_height = 400;
    int target_bottom_width = 240, target_bottom_height = 320;
    int failed_page = -1, logged_failed_page = -1;
    float page_width = 600, page_height = 900;
  };
};
static int reads = 0, decodes = 0, max_decode_zoom = 99;
void PromoteCbzAdjacentSlotIfMatching(Book::CbzState *, int) {}
bool CbzPreviewCacheValid(const Book::CbzState::Cache &c, int p) { return c.page == p && !c.pixels.empty(); }
bool CbzBitmapCacheValid(const Book::CbzState::Cache &c, int p, int z) { return CbzPreviewCacheValid(c,p) && c.zoom_index == z; }
void ResetCbzPageBitmap(Book::CbzState::PageBitmap *p) { *p = Book::CbzState::PageBitmap(); }
bool ReadCbzArchiveEntryBytes(const std::string &, int, std::vector<unsigned char> *b, size_t) { ++reads; b->assign(10, 1); return true; }
const char *GetLastCbzArchiveError() { return "read failed"; }
const char *GetLastCbzDecodeError() { return "decode failed"; }
bool DecodeCbzPageImage(const std::vector<unsigned char> &, int z, int, int, CbzDecodedPage *d) {
  ++decodes;
  if (z > max_decode_zoom) return false;
  d->original_width = 600; d->original_height = 900;
  d->source_bitmap.width = 240 * (z+1); d->source_bitmap.height = 360 * (z+1);
  d->source_bitmap.pixels.assign(d->source_bitmap.width*d->source_bitmap.height, 42);
  return true;
}
bool ScaleCbzBitmap(const CbzBitmap &, int w, int h, bool, CbzBitmap *o) {
  o->width=w; o->height=h; o->pixels.assign(w*h,42); return true;
}
#include "cbz_source_under_test.inc"
int main() {
  Book::CbzState s;
  assert(EnsureCbzPreviewCache(&s, 0));
  assert(EnsureCbzInteractiveCache(&s, 0));
  assert(reads == 1 && decodes == 1 && "synchronous page must read and decode once");
  assert(s.current_source.zoom_index == 3);
  assert(EnsureCbzInteractiveCache(&s, 0) && reads == 1);
  s.viewport.zoom_index = 2;
  assert(EnsureCbzInteractiveCache(&s, 0) && reads == 1);
  s.viewport.zoom_index = 4;
  assert(EnsureCbzInteractiveCache(&s, 0) && reads == 2);
  assert(EnsureCbzPreviewCache(&s, 1));
  assert(EnsureCbzInteractiveCache(&s, 1) && reads == 3);
  // The deferred path still decodes a cheap preview, not full zoom.
  debug_runtime::sync = false;
  Book::CbzState deferred;
  assert(EnsureCbzPreviewCache(&deferred, 0));
  assert(deferred.current_source.zoom_index == 0);
  // A failed high-resolution decode must retain the low-resolution fallback.
  debug_runtime::sync = true; max_decode_zoom = 0;
  Book::CbzState fallback;
  assert(EnsureCbzPreviewCache(&fallback, 0));
  assert(fallback.current_source.zoom_index == 0);
  puts("CBZ source reuse passed");
}
