#pragma once

#include "formats/common/xml_book_parser.h"
#include "shared/orientation_utils.h"

#include <unistd.h>

namespace {

// A buffer-only continuity assertion misses text abandoned by Page::Draw
// when the paginator has placed more lines on a screen than it can display.
void TestXmlPageRenderingContinuity() {
  for (unsigned char orientation = 0; orientation < 3; orientation++) {
    for (int pixel_size : {12, 14, 20, 24}) {
      for (int line_spacing : {0, 2, 6}) {
        for (bool coalesced : {false, true}) {
          TestCtx tc;
          tc.ctx.orientation = &orientation;
          tc.paragraph_spacing = 0;
          tc.text.SetPixelSize((u8)pixel_size);
          tc.text.linespacing = line_spacing;
          tc.text.display.width = 240;
          tc.text.landscape = orientation_utils::IsLandscape(orientation);
          tc.text.capture_rendered_text = true;
          u16 left = 0, right = 0;
          tc.text.screenleft = &left;
          tc.text.screenright = &right;
          Book book(tc.ctx);

          std::string expected;
          std::string html = "<html><body>";
          for (int paragraph = 0; paragraph < 10; paragraph++) {
            html += "<p>";
            for (int word = 0; word < 150; word++) {
              char token[32];
              snprintf(token, sizeof(token), "w%02d%03d", paragraph, word);
              expected += token;
              html += token;
              html += ' ';
            }
            html += "</p>";
          }
          html += "</body></html>";

          if (coalesced) {
            // EPUB combines text callbacks. Start with a valid baseline here
            // to isolate the screen-metrics regression from initialization.
            parsedata_t p = MakeParseData(tc, book);
            p.base_font_size_px = (u8)pixel_size;
            p.pen.y = tc.text.margin.top + tc.text.GetHeight();
            p.coalesce_text_segments = true;
            const xml_parse_utils::XmlParseResult result =
                xml_parse_utils::ParseXmlString(html, MakeXmlOpts(&p));
            ExpectTrue("coalesced XML parses", result.ok);
          } else {
            // Exercise production initialization and streamed XML callbacks,
            // the same entry point used by FB2.
            char filename[] = "/tmp/3dslibris-render-XXXXXX";
            const int fd = mkstemp(filename);
            ExpectTrue("render fixture created", fd >= 0);
            FILE *file = fdopen(fd, "wb");
            ExpectTrue("render fixture opened", file != nullptr);
            const size_t written = fwrite(html.data(), 1, html.size(), file);
            fclose(file);
            ExpectTrue("render fixture written", written == html.size());
            const u8 result = xml_book_parser::ParseXmlBookFile(
                &book, filename, true, BuildBookParseDeps(&book), nullptr,
                nullptr);
            unlink(filename);
            ExpectIntEq("streamed XML parses", result, 0);
          }

          ExpectTrue("render fixture spans pages", book.GetPageCount() > 1);
          for (int page = 0; page < book.GetPageCount(); page++)
            book.GetPage(page)->Draw(&tc.text);
          if (tc.text.rendered_ascii != expected) {
            fprintf(stderr,
                    "render continuity: orientation=%u px=%d spacing=%d "
                    "coalesced=%d expected=%zu drawn=%zu clipped=%d\n",
                    orientation, pixel_size, line_spacing, coalesced,
                    expected.size(), tc.text.rendered_ascii.size(),
                    tc.text.clipped_glyphs);
          }
          ExpectTrue("every word is drawn once and in order",
                     tc.text.rendered_ascii == expected);
          ExpectIntEq("no glyph falls outside the reading area",
                      tc.text.clipped_glyphs, 0);
        }
      }
    }
  }
}

} // namespace
