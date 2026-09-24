#include <cassert>
#include <cstdint>
#include <cstdio>
static uint64_t now = 0;
uint64_t osGetTime() { return now; }
struct Prefs {
  bool write_pending = false;
  uint64_t write_due_ms = 0;
  int writes = 0;
  void RequestWrite();
  bool FlushPendingWrite(bool force = false);
  int Write() { ++writes; write_pending = false; return 0; }
};
#include "prefs_deferred.inc"
int main() {
  Prefs p;
  assert(!p.FlushPendingWrite(true));
  p.RequestWrite(); now = 300; p.RequestWrite();
  now = 2299; assert(!p.FlushPendingWrite());
  now = 2300; assert(p.FlushPendingWrite() && p.writes == 1);
  assert(!p.FlushPendingWrite());
  p.RequestWrite(); assert(p.FlushPendingWrite(true) && p.writes == 2);
  assert(!p.write_pending);
  puts("PASS: preference changes coalesce and force-flush on exit");
}
