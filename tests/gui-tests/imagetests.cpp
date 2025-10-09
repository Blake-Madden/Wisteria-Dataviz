#include "../../src/base/image.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/init.h>

using namespace Wisteria::GraphItems;

// Helper: write SVG content to a temp file and return the path (left on disk).
static wxString WriteTempSVG(const wxString& content)
    {
    wxString path = wxFileName::CreateTempFileName(L"svgsize_") + L".svg";
    wxFFile f(path, L"wb");
    CHECK(f.IsOpened());
    if (f.IsOpened())
        {
        CHECK(f.Write(content.utf8_string()));
        f.Close();
        }
    return path;
    }

// -----------------------------
// Leading-node robustness test
// -----------------------------
TEST_CASE("GetSVGSize parses SVG files with leading nodes", "[image][svg][filepath]")
    {
    const wxString cleanSVG = L"<svg width=\"200\" height=\"100\" viewBox=\"0 0 200 100\" "
                              L"xmlns=\"http://www.w3.org/2000/svg\">"
                              L"<rect width=\"200\" height=\"100\" fill=\"red\" />"
                              L"</svg>";

    const wxString leadingCommentSVG = L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                       L"<!-- Created with Inkscape -->\n"
                                       L"<svg width=\"200\" height=\"100\" viewBox=\"0 0 200 100\" "
                                       L"xmlns=\"http://www.w3.org/2000/svg\">"
                                       L"<rect width=\"200\" height=\"100\" fill=\"red\" />"
                                       L"</svg>";

    const wxString leadingWhitespaceSVG =
        L"\n \t\n"
        L"<svg width=\"200\" height=\"100\" viewBox=\"0 0 200 100\" "
        L"xmlns=\"http://www.w3.org/2000/svg\">"
        L"<rect width=\"200\" height=\"100\" fill=\"red\" />"
        L"</svg>";

    const wxString leadingDoctypeSVG = L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                       L"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
                                       L"\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
                                       L"<svg width=\"200\" height=\"100\" viewBox=\"0 0 200 100\" "
                                       L"xmlns=\"http://www.w3.org/2000/svg\">"
                                       L"<rect width=\"200\" height=\"100\" fill=\"red\" />"
                                       L"</svg>";

    struct Case
        {
        const char* name;
        const wxString* svg;
        } cases[] = {
            { "clean", &cleanSVG },
            { "leading_comment", &leadingCommentSVG },
            { "leading_whitespace", &leadingWhitespaceSVG },
            { "leading_doctype", &leadingDoctypeSVG },
        };

    for (const auto& c : cases)
        {
        DYNAMIC_SECTION("leading nodes: " << c.name)
            {
            const wxString path = WriteTempSVG(*c.svg);
            INFO("Temp SVG (kept): " << path);

            const wxSize sz = Image::GetSVGSize(path);

            CHECK(sz.GetWidth() == 200);
            CHECK(sz.GetHeight() == 100);
            }
        }
    }

// ---------------------------------------
// Regex robustness: trigger current bug
// ---------------------------------------
TEST_CASE("GetSVGSize viewBox regex handles commas/whitespace/decimals/exponents",
          "[image][svg][regex]")
    {
    // These variants should all resolve to width=200, height=100.
    // If your current regex only matches space-separated integers, these will fail.
    const wxString viewBox_commas = L"<svg width=\"200\" height=\"100\" viewBox=\"0,0,200,100\" "
                                    L"xmlns=\"http://www.w3.org/2000/svg\"></svg>";

    const wxString viewBox_mixed =
        L"<svg viewBox=\"0,0 200,100\" xmlns=\"http://www.w3.org/2000/svg\"></svg>";

    const wxString viewBox_decimals_ws =
        L"<svg viewBox=\"0.0 0.0   200.0   100.0\" xmlns=\"http://www.w3.org/2000/svg\"></svg>";

    const wxString viewBox_newlines =
        L"<svg viewBox=\"\n 0  \n 0 \n 200 \n 100 \n\" xmlns=\"http://www.w3.org/2000/svg\"></svg>";

    struct Case
        {
        const char* name;
        const wxString* svg;
        } cases[] = {
            { "commas", &viewBox_commas },        { "mixed", &viewBox_mixed },
            { "decimals", &viewBox_decimals_ws },
            { "newlines", &viewBox_newlines },
        };

    for (const auto& c : cases)
        {
        DYNAMIC_SECTION("regex case: " << c.name)
            {
            const wxString path = WriteTempSVG(*c.svg);
            INFO("Temp SVG (kept): " << path);

            const wxSize sz = Image::GetSVGSize(path);

            CHECK(sz.GetWidth() == 200);
            CHECK(sz.GetHeight() == 100);
            }
        }
    }
