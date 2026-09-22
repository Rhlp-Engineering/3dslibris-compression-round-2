#include "book/book.h"

#include "book/page.h"
#include "formats/mupdf/mupdf_worker.h"
#include "formats/cbz/cbz_worker.h"
#include "shared/debug_log.h"

void Book::SuspendFixedLayoutWorkers() {
  // APT calls this before yielding to HOME. Keep worker-owned resources alive
  // and never wait for a decoder here. Resume/close performs the joins.
  if (IsPdf() && mupdf_state && mupdf_state->worker)
    SignalMuPdfWorkerShutdown(mupdf_state);
  if (IsCbz() && cbz_state)
    SignalCbzWorkerShutdown(cbz_state);
}

void Book::ReleaseMuPdfMemoryForSuspend() {
  if (IsPdf() && mupdf_state)
    ReleaseMuPdfMemoryForSuspendImpl(mupdf_state);
}

void Book::ResumeFixedLayoutWorkers() {
  // Restart background worker threads after HOME menu returns control.
  // Called from ReaderController::OnAppletResumed() on the main thread.
  if (IsPdf() && mupdf_state) {
    if (mupdf_state->worker) {
      // Confirm worker exit before freeing any render resources. A decoder
      // may still need time to finish its current strip after wakeup.
      DBG_LOGF(GetStatusReporter(), "[APT][RESUME] MuPDF worker join begin book=%s",
               GetFileName() ? GetFileName() : "");
      ShutdownMuPdfWorker(mupdf_state);
      DBG_LOGF(GetStatusReporter(), "[APT][RESUME] MuPDF worker join done book=%s",
               GetFileName() ? GetFileName() : "");
    }
    ReleaseMuPdfMemoryForSuspend();
    DBG_LOGF(GetStatusReporter(), "[APT][RESUME] MuPDF worker init begin book=%s",
             GetFileName() ? GetFileName() : "");
    InitMuPdfWorker(mupdf_state);
    DBG_LOGF(GetStatusReporter(), "[APT][RESUME] MuPDF worker init done worker=%s book=%s",
             mupdf_state->worker ? "ok" : "null (n/a)", GetFileName() ? GetFileName() : "");
  }
  if (IsCbz()) {
    DBG_LOGF(GetStatusReporter(), "[APT][RESUME] CBZ worker restart book=%s",
             GetFileName() ? GetFileName() : "");
    ResetCbzTransientViewState(true);
    DBG_LOGF(GetStatusReporter(), "[APT][RESUME] CBZ worker restart done book=%s",
             GetFileName() ? GetFileName() : "");
  }
}
