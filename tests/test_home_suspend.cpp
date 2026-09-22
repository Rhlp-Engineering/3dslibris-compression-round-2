#include <cassert>
#include <cstdio>
#define DBG_LOGF(...) ((void)0)
enum APT_HookType { APTHOOK_ONSUSPEND, APTHOOK_ONRESTORE, APTHOOK_ONSLEEP,
                    APTHOOK_ONWAKEUP, APTHOOK_ONEXIT };
struct State {
  bool suspended=false, resume=false, handled=false, exit=false;
  void SetSuspended(bool v) { suspended=v; }
  void SetResumePending(bool v) { resume=v; }
  void SetSuspendHandled(bool v) { handled=v; }
  void SetExitRequested(bool v) { exit=v; }
  bool IsSuspended() const { return suspended; }
};
struct Book;
struct App {
  State lifecycle_state_;
  int prepared=0;
  Book *current=nullptr;
  Book *GetCurrentBook() { return current; }
  void SetPdfTouchDragActive(bool) {}
  void SetPdfTouchLastX(int) {}
  void SetPdfTouchLastY(int) {}
  void SetPdfDeferredReadyAtMs(int) {}
  void OnReaderAppletSuspendRequested() { ++prepared; }
  void HandleAppletHook(APT_HookType);
};
static int signals=0, joins=0, releases=0;
struct Book {
  struct MuPdfState { void *worker=(void*)1; bool worker_init_attempted=true; } pdf;
  struct CbzState {} cbz;
  MuPdfState *mupdf_state=&pdf;
  CbzState *cbz_state=&cbz;
  bool IsPdf() { return true; }
  bool IsCbz() { return true; }
  void SuspendFixedLayoutWorkers();
  void ResetCbzTransientViewState(bool) { ++releases; }
  void ReleaseMuPdfMemoryForSuspend() { ++releases; }
  void ClearInlineImageCache() { ++releases; }
};
void SignalMuPdfWorkerShutdown(Book::MuPdfState *) { ++signals; }
void SignalCbzWorkerShutdown(Book::CbzState *) { ++signals; }
void ShutdownCbzWorker(Book::CbzState *) { ++joins; }
struct ReaderController {
  App &app_;
  explicit ReaderController(App &app) : app_(app) {}
  void OnAppletSuspendRequested();
};
#include "home_suspend_under_test.inc"
int main() {
  App app;
  // Model libctru: the app's main-loop body cannot run between these hooks.
  app.HandleAppletHook(APTHOOK_ONSUSPEND);
  assert(app.prepared == 1 && "must prepare before aptMainLoop returns");
  assert(app.lifecycle_state_.suspended);
  app.HandleAppletHook(APTHOOK_ONRESTORE);
  assert(!app.lifecycle_state_.suspended && app.lifecycle_state_.resume);
  app.HandleAppletHook(APTHOOK_ONSLEEP);
  assert(app.prepared == 2);
  app.HandleAppletHook(APTHOOK_ONWAKEUP);
  app.HandleAppletHook(APTHOOK_ONEXIT);
  assert(app.lifecycle_state_.exit);
  Book b;
  b.SuspendFixedLayoutWorkers();
  assert(signals == 2);
  assert(joins == 0 && releases == 0 && "hook must neither join nor free worker resources");
  ReaderController reader(app);
  reader.OnAppletSuspendRequested(); // No current book, no worker to signal.
  assert(signals == 2);
  app.current = &b;
  reader.OnAppletSuspendRequested();
  assert(signals == 4 && joins == 0 && releases == 0);
  puts("HOME suspend ordering and nonblocking preparation passed");
}
