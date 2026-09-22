#include <cassert>
#include <cstdint>
#include <vector>
#include <cstdio>
typedef uint64_t u64;
typedef uint16_t u16;
struct LightEvent {};
static int clears = 0;
void LightEvent_Clear(LightEvent *) { ++clears; }
static u64 now_ms = 0;
u64 osGetTime() { return now_ms; }
void svcSleepThread(long long ns) { now_ms += ns / 1000000; }
struct Book {
  struct MuPdfState {
    struct MuPdfWorker {
      bool job_submitted = true, job_pending = true;
      int job_strip_y0 = 0, job_strip_y1 = 10;
      LightEvent done_event;
    };
    MuPdfWorker *worker = nullptr;
    bool worker_init_attempted = true;
    struct {
      bool active = true;
      int strips_completed = 1, partial_width = 10, partial_height = 10;
      std::vector<u16> partial_pixels;
    } incremental;
  };
};
static bool rendering = false;
static int joins = 0;
void ShutdownMuPdfWorker(Book::MuPdfState *s) {
  // Model a strip still running after the old 100 ms deadline. Its output
  // buffer and job description must survive until the join acknowledges exit.
  assert(s->incremental.partial_pixels.size() == 100);
  assert(s->worker->job_strip_y1 == 10);
  rendering = false;
  ++joins;
  s->worker = nullptr;
}
#include "mupdf_cancel_under_test.inc"
int main() {
  Book::MuPdfState s;
  Book::MuPdfState::MuPdfWorker w;
  s.worker = &w;
  s.incremental.partial_pixels.assign(100, 42);
  rendering = true;
  CancelMuPdfIncrementalRenderState(&s);
  assert(!rendering && "cancellation freed the output of a still-running strip");
  assert(joins == 1);
  assert(!s.worker_init_attempted && "allow the next render to recreate its worker");
  assert(s.incremental.partial_pixels.empty());
  assert(!s.incremental.active);
  // Completed jobs need no worker teardown.
  w.job_pending = false;
  s.worker = &w;
  s.incremental.partial_pixels.assign(100, 42);
  CancelMuPdfIncrementalRenderState(&s);
  assert(joins == 1 && !w.job_submitted && clears == 1);
  CancelMuPdfIncrementalRenderState(nullptr);
  puts("MuPDF cancellation regression passed");
}
