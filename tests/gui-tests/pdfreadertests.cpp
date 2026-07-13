#include "../../src/data/pdfreader.h"
#include <catch2/catch_test_macros.hpp>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/string.h>

using namespace Wisteria::Data;

namespace
    {
    wxString GetTestFilesDir()
        {
        return wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() + L"/test_files/";
        }
    } // namespace

TEST_CASE("PdfReader loads glyph name and CID-to-Unicode lookup tables", "[pdf][PdfReader]")
    {
    const wxString testFilesDir{ GetTestFilesDir() };

    SECTION("Adobe Glyph List file loads successfully")
        {
        CHECK(PdfReader::LoadGlyphNameTableFromFile(testFilesDir + L"glyphlist.txt"));
        }
    SECTION("Missing glyph name table file fails gracefully")
        {
        CHECK_FALSE(PdfReader::LoadGlyphNameTableFromFile(testFilesDir + L"does_not_exist.txt"));
        }
    SECTION("Adobe-Japan1 CID-to-Unicode table file loads successfully")
        {
        CHECK(PdfReader::LoadCidToUnicodeTableFromFile(L"Adobe-Japan1",
                                                       testFilesDir + L"UniJIS-UTF16-H"));
        }
    SECTION("Missing CID-to-Unicode table file fails gracefully")
        {
        CHECK_FALSE(PdfReader::LoadCidToUnicodeTableFromFile(L"Adobe-Japan1",
                                                             testFilesDir + L"does_not_exist.txt"));
        }
    }

TEST_CASE("PdfReader extracts vertical Japanese text via a CID-to-Unicode table",
          "[pdf][PdfReader]")
    {
    const wxString testFilesDir{ GetTestFilesDir() };

    // vertical.pdf's text is drawn with a Type0/CIDFontType2 font (Identity-V
    // encoding, Adobe-Japan1 CIDSystemInfo ordering) that has no embedded ToUnicode
    // CMap, so its CIDs can only be resolved to Unicode through a loaded
    // CID-to-Unicode table for that ordering.
    REQUIRE(PdfReader::LoadCidToUnicodeTableFromFile(L"Adobe-Japan1",
                                                     testFilesDir + L"UniJIS-UTF16-H"));

    PdfReader reader;
    wxString text;
    REQUIRE_NOTHROW(text = reader.ReadFile(testFilesDir + L"vertical.pdf"));

    CHECK(text.Contains(L"あいうえお"));
    CHECK(text.Contains(L"日本語"));
    }
