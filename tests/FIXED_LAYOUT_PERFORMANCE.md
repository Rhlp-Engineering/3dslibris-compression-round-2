# PDF / CBZ performance capture

Use `3dslibris-debug.3dsx` or `3dslibris-debug.cia`. Measurements are enabled only in debug builds and written as `PERF` lines to `sd:/3ds/3dslibris/3dslibris.log`.

## Repeatable capture

1. With the application closed, save the previous log under another name.
2. Record the device (Old/New 3DS or Azahar version), CIA/3DSX, SD card, document name, orientation and zoom. The log records New 3DS/homebrew flags but cannot identify an emulator reliably.
3. Open a PDF or CBZ, then visit the same ten pages on each device. Wait 10–15 seconds on each page, or longer if a PDF is still refining.
4. Return through those pages to compare cached and uncached work. Repeat at another zoom, keeping settings identical across devices.
5. Exit normally and copy the log before the next run. Send the complete log together with the device/settings notes and page numbers that felt slow.

## Reading the measurements

Durations `us` are microseconds (divide by 1000 for milliseconds). `at_us` is the capture timestamp, not the time the line was written. Pages are one-based; page 0 is document opening. Zoom -1 means unspecified.

| Stages | Work measured |
| --- | --- |
| `pdf.open_file`, `pdf.index_metadata`, `pdf.open_total` | MuPDF opening, page count/metadata and total opening |
| `pdf.load_page`, `pdf.display_list` | Page loading/bounds and display-list construction |
| `pdf.raster`, `pdf.rgb565` | Rasterization including pixmap setup; output allocation/conversion to RGB565 |
| `pdf.preview_total`, `pdf.interactive_total` | Whole render calls, including their subphases and cleanup |
| `pdf.strip_worker`, `pdf.strip_sync` | Each final-quality strip, including rasterization and conversion |
| `cbz.index_zip`, `cbz.open_total` | Archive indexing and total document opening |
| `cbz.read_zip`, `cbz.decode` | Entry lookup/read/inflate; image decoding including fallback attempts |
| `cbz.scale_preview`, `cbz.scale_interactive` | Scaling for each cache |
| `cbz.prefetch_*` | Read, decode and combined scaling during prefetch |
| `first_present`, `preview_present`, `interactive_present`, `final_present` | Elapsed time from the first draw of that view until a buffer swap containing that quality |

A new `view` starts when the document, page, zoom or target dimensions change, or after leaving the reader. Presentation timings start in the draw path, **not** at button press/book selection; opening is measured separately. They include scheduled quality delays and intervening logging. A buffer swap is not a physical display-latency measurement. Each quality is recorded only once per view. CBZ uses interactive quality as its best cached level and does not emit `final_present`.

Use `doc` and the filename mapping to associate records. CBZ opening uses a separate `CBZ_OPEN` identity; match it to the rendering identity by filename. Filenames are truncated to 127 bytes. Cached pages can omit read/decode/render stages. Missing completion records after a failure or cancellation do not mean zero duration. Totals include their subphases: do not add both when calculating work. PDF file/image/font reads can occur inside MuPDF parsing and rendering, so these timings do not isolate SD I/O.

`bytes` and `size` describe the relevant payload/output; strip bytes describe the shared destination buffer, not additional allocation per strip. Prefetch scaling size describes the interactive output. Memory rows sample heap usage/free space, free linear memory and free memory regions after presentation, in bytes. Region free space is different from allocator free space. These are snapshots, **not peak-memory measurements**.

Workers enqueue records in a fixed 64-event RAM queue. The main thread writes them after presentation; workers do not write these logs to SD. `PERF dropped=N` means the capture is incomplete. Debug logging itself adds overhead, especially on slow SD cards. Compare equivalent debug captures; emulator timings do not predict console performance. Existing lifecycle/error logs remain available for crash diagnosis.
