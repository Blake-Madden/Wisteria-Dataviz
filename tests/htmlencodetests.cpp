// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/html_encode.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("HTML Encode", "[html encode]")
    {
    SECTION("Null")
        {
        lily_of_the_valley::html_encode_text encode;
        CHECK_FALSE(encode.needs_to_be_encoded({ L"text", 0 }));
        CHECK(encode({ L"text", 0 }, true) == L"");
        }
    SECTION("Plain Text")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello, world";
        CHECK_FALSE(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello, world");
        }
    SECTION("Whitespace")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello\tworld";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello&nbsp;&nbsp;&nbsp;world");
        text = L"hello\nworld";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello<p></p>world");
        text = L"hello\n\rworld";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello<p></p>world");
        text = L"hello    world";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello &nbsp;&nbsp;&nbsp;world");
        text = L"hello  world";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello &nbsp;world");
        }
    SECTION("Illegal Symbols")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello&<>\"\'world";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"hello&#38;&#60;&#62;&#34;&#39;world");
        }
    SECTION("Unicode")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"heâllo\x432";
        CHECK(encode.needs_to_be_encoded(text));
        CHECK(encode(text, true) == L"he&#226;llo&#1074;");
        }
    }

TEST_CASE("HTML Simple Encode", "[html encode]")
    {
    SECTION("Null")
        {
        lily_of_the_valley::html_encode_text encode;
        CHECK_FALSE(encode.needs_to_be_simple_encoded({ L"text", 0 }));
        CHECK(encode({ L"text", 0 }, true) == L"");
        }
    SECTION("Plain Text")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello, world";
        CHECK_FALSE(encode.needs_to_be_simple_encoded(text));
        CHECK(encode(text, true) == L"hello, world");
        }
    SECTION("Whitespace")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello\tworld";
        CHECK_FALSE(encode.needs_to_be_simple_encoded(text));
        CHECK(encode(text, true) == L"hello&nbsp;&nbsp;&nbsp;world");
        text = L"hello\nworld";
        CHECK_FALSE(encode.needs_to_be_simple_encoded(text));
        CHECK(encode(text, true) == L"hello<p></p>world");
        text = L"hello\n\rworld";
        CHECK_FALSE(encode.needs_to_be_simple_encoded(text));
        CHECK(encode(text, true) == L"hello<p></p>world");
        text = L"hello    world";
        CHECK_FALSE(encode.needs_to_be_simple_encoded(text));
        CHECK(encode(text, true) == L"hello &nbsp;&nbsp;&nbsp;world");
        text = L"hello  world";
        CHECK_FALSE(encode.needs_to_be_simple_encoded(text));
        CHECK(encode(text, true) == L"hello &nbsp;world");
        }
    SECTION("Illegal Symbols")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"hello&<>\"\'world";
        CHECK(encode.needs_to_be_simple_encoded(text));
        CHECK(encode(text, true) == L"hello&#38;&#60;&#62;&#34;&#39;world");
        }
    SECTION("Unicode")
        {
        lily_of_the_valley::html_encode_text encode;
        const wchar_t* text = L"heâllo\x432";
        CHECK_FALSE(encode.needs_to_be_simple_encoded(text));
        CHECK(encode(text, true) == L"he&#226;llo&#1074;");
        }
    }

namespace
{
    // Small helper: strip in place and return the result (for terse assertions)
    static std::wstring Strip(std::wstring html, bool preserve = true)
        {
        lily_of_the_valley::html_format::strip_hyperlinks(html, preserve);
        return html;
        }
}

// -----------------------------------------------------------------------------
// Removes external links but keeps inner text
// -----------------------------------------------------------------------------
TEST_CASE("strip_hyperlinks removes external anchors", "[html][strip_hyperlinks]")
    {
    std::wstring html =
        L"Go to <a href=\"https://example.com\">example</a> and "
        L"<a href=\"mailto:test@example.com\">email us</a>.";
    const std::wstring expected =
        L"Go to example and email us.";

    CHECK(Strip(html) == expected);
    }

// -----------------------------------------------------------------------------
// Internal bookmark link with no corresponding bookmark in the page is removed
// -----------------------------------------------------------------------------
TEST_CASE("strip_hyperlinks removes missing in-page bookmark links", "[html][strip_hyperlinks]")
    {
    std::wstring html =
        L"<p>Jump <a href=\"#top\">to top</a> of the page.</p>";
    const std::wstring expected =
        L"<p>Jump to top of the page.</p>";

    CHECK(Strip(html, /*preserveInPageBookmarks*/ true) == expected);
    }

// -----------------------------------------------------------------------------
// When asked not to preserve in-page bookmarks, all links are removed
// -----------------------------------------------------------------------------
TEST_CASE("strip_hyperlinks removes all links when preserve=false", "[html][strip_hyperlinks]")
    {
    std::wstring html =
        L"<div>"
        L"<a href=\"#bm\">bookmark link</a> and "
        L"<a href=\"/relative/page.html\">relative</a> and "
        L"<a href=\"https://example.com\">absolute</a>"
        L"</div>";

    const std::wstring expected =
        L"<div>"
        L"bookmark link and "
        L"relative and "
        L"absolute"
        L"</div>";

    CHECK(Strip(html, /*preserveInPageBookmarks*/ false) == expected);
    }

// -----------------------------------------------------------------------------
// If a matching bookmark exists in the same page and preserve=true, keep the link
// (We add both common bookmark forms: id= and name=)
// -----------------------------------------------------------------------------
TEST_CASE("strip_hyperlinks preserves in-page links to existing bookmarks", "[html][strip_hyperlinks]")
    {
    // include two common bookmark declarations; finder should notice at least one
    std::wstring html =
        L"<h1>Title</h1>"
        L"<a id=\"bm\"></a>"
        L"<a name=\"bm\"></a>"
        L"<p>Jump <a href=\"#bm\">back to title</a> here.</p>";

    // Expect unchanged when preserving and bookmark exists
    CHECK(Strip(html, /*preserveInPageBookmarks*/ true) == html);
    }

// -----------------------------------------------------------------------------
// Malformed anchor with no closing </a>: start tag is stripped; text retained
// -----------------------------------------------------------------------------
TEST_CASE("strip_hyperlinks handles missing closing tag gracefully", "[html][strip_hyperlinks]")
    {
    std::wstring html =
        L"Click <a href=\"https://example.com\">this broken link.";
    // The function erases the opening tag and leaves the text behind if </a> is missing
    const std::wstring expected =
        L"Click this broken link.";

    CHECK(Strip(html) == expected);
    }

// -----------------------------------------------------------------------------
// No anchors: input unchanged
// -----------------------------------------------------------------------------
TEST_CASE("strip_hyperlinks leaves text without anchors unchanged", "[html][strip_hyperlinks]")
    {
    std::wstring html = L"<p>No links here, just text.</p>";
    CHECK(Strip(html) == html);
    }

namespace
{
    // Helper: call strip_images() and return the modified string (for terse checks)
    static std::wstring StripImg(std::wstring html, bool removePadding = true)
        {
        lily_of_the_valley::html_format::strip_images(html, removePadding);
        return html;
        }
}

// -----------------------------------------------------------------------------
// Removes a single <img ...> tag and keeps surrounding text intact
// -----------------------------------------------------------------------------
TEST_CASE("strip_images removes a single image tag", "[html][strip_images]")
    {
    std::wstring html =
        L"<p>Before <img src=\"pic.png\" alt=\"pic\"> after.</p>";
    const std::wstring expected =
        L"<p>Before  after.</p>";

    CHECK(StripImg(html) == expected);
    }

// -----------------------------------------------------------------------------
// Removes &nbsp; padding on both sides when removePadding=true (default)
// -----------------------------------------------------------------------------
TEST_CASE("strip_images removes &nbsp; padding around images", "[html][strip_images]")
    {
    std::wstring html =
        L"<div>One&nbsp;<img src=\"a.png\">&nbsp;Two</div>";
    const std::wstring expected =
        L"<div>OneTwo</div>";

    CHECK(StripImg(html, /*removePadding*/ true) == expected);
    }

// -----------------------------------------------------------------------------
// Keeps &nbsp; padding when removePadding=false
// -----------------------------------------------------------------------------
TEST_CASE("strip_images preserves &nbsp; when removePadding is false", "[html][strip_images]")
    {
    std::wstring html =
        L"<div>Start&nbsp;<img src=\"a.png\">&nbsp;End</div>";
    const std::wstring expected =
        L"<div>Start&nbsp;&nbsp;End</div>";

    CHECK(StripImg(html, /*removePadding*/ false) == expected);
    }

// -----------------------------------------------------------------------------
// Handles multiple images (mixed with and without padding) across the string
// -----------------------------------------------------------------------------
TEST_CASE("strip_images removes multiple images throughout the text", "[html][strip_images]")
    {
    std::wstring html =
        L"Top <img src=\"x.png\"> mid&nbsp;<img src=\"y.png\">&nbsp;tail <img src=\"z.png\">";
    const std::wstring expected =
        L"Top  midtail ";

    CHECK(StripImg(html) == expected);
    }

// -----------------------------------------------------------------------------
// Graceful handling when an <img ...> tag is malformed (no closing '>'):
// nothing after the broken tag is erased by this function
// -----------------------------------------------------------------------------
TEST_CASE("strip_images leaves malformed image tag (no '>') untouched", "[html][strip_images]")
    {
    std::wstring html =
        L"Text <img src=\"broken.png\" and more text";
    // The function finds the start, fails to find '>' and breaks; input remains unchanged.
    CHECK(StripImg(html) == html);
    }

// -----------------------------------------------------------------------------
// If image is at the very start, only right-side &nbsp; is removed (left check uses start>6)
// -----------------------------------------------------------------------------
TEST_CASE("strip_images: image at start only removes right padding", "[html][strip_images]")
    {
    std::wstring html =
        L"<img src=\"hero.png\">&nbsp;Heading";
    const std::wstring expected =
        L"Heading";

    CHECK(StripImg(html) == expected);
    }

// -----------------------------------------------------------------------------
// Non-image tags remain intact
// -----------------------------------------------------------------------------
TEST_CASE("strip_images does not affect other tags", "[html][strip_images]")
    {
    std::wstring html =
        L"<p><strong>Bold</strong> and <em>italic</em> text.</p>";
    CHECK(StripImg(html) == html);
    }

namespace
{
    static std::wstring StripBody(std::wstring html)
        {
        lily_of_the_valley::html_format::strip_body_attributes(html);
        return html;
        }
}

// -----------------------------------------------------------------------------
// Removes simple attribute (e.g. bgcolor) from <body>
// -----------------------------------------------------------------------------
TEST_CASE("strip_body_attributes removes simple attribute", "[html][strip_body_attributes]")
    {
    std::wstring html =
        L"<html><body bgcolor=\"white\">Hello</body></html>";
    const std::wstring expected =
        L"<html><body>Hello</body></html>";

    CHECK(StripBody(html) == expected);
    }

// -----------------------------------------------------------------------------
// Removes multiple attributes and preserves tag structure
// -----------------------------------------------------------------------------
TEST_CASE("strip_body_attributes removes multiple attributes", "[html][strip_body_attributes]")
    {
    std::wstring html =
        L"<body class=\"main\" id=\"page\" style=\"color:red\">Content</body>";
    const std::wstring expected =
        L"<body>Content</body>";

    CHECK(StripBody(html) == expected);
    }

// -----------------------------------------------------------------------------
// Leaves <body> with no attributes untouched
// -----------------------------------------------------------------------------
TEST_CASE("strip_body_attributes leaves bare <body> alone", "[html][strip_body_attributes]")
    {
    std::wstring html =
        L"<html><body>Plain text</body></html>";
    CHECK(StripBody(html) == html);
    }

// -----------------------------------------------------------------------------
// Handles uppercase/lowercase mismatch: only lowercase <body > is recognized
// -----------------------------------------------------------------------------
TEST_CASE("strip_body_attributes only matches lowercase '<body '", "[html][strip_body_attributes]")
    {
    std::wstring html =
        L"<HTML><BODY BGCOLOR=\"red\">X</BODY></HTML>";
    // function won't find "<BODY ", so string is unchanged
    CHECK(StripBody(html) == html);
    }

// -----------------------------------------------------------------------------
// Graceful handling: <body ...> with no closing '>' is left unchanged
// -----------------------------------------------------------------------------
TEST_CASE("strip_body_attributes handles missing closing bracket", "[html][strip_body_attributes]")
    {
    std::wstring html =
        L"<html><body bgcolor=\"red\" Text still here";
    // no '>', function returns without erase
    CHECK(StripBody(html) == html);
    }

// -----------------------------------------------------------------------------
// Only first <body ...> is affected
// -----------------------------------------------------------------------------
TEST_CASE("strip_body_attributes only processes first <body>", "[html][strip_body_attributes]")
    {
    std::wstring html =
        L"<body bgcolor=\"red\">first</body><body bgcolor=\"blue\">second</body>";
    const std::wstring expected =
        L"<body>first</body><body bgcolor=\"blue\">second</body>";

    CHECK(StripBody(html) == expected);
    }

namespace
{
    // helpers to call the functions under test and return the mutated string
    static std::wstring SetTitle(std::wstring html, const std::wstring& title)
        {
        lily_of_the_valley::html_format::set_title(html, title);
        return html;
        }

    static std::wstring SetEncoding(std::wstring html, const std::wstring& enc = L"UTF-8")
        {
        lily_of_the_valley::html_format::set_encoding(html, enc);
        return html;
        }
}

// -----------------------------------------------------------------------------
// set_title: replaces existing <title>...</title> content
// -----------------------------------------------------------------------------
TEST_CASE("set_title replaces existing title", "[html][set_title]")
    {
    const std::wstring html =
        L"<html><head><title>Old</title></head><body>Body</body></html>";
    const std::wstring expected =
        L"<html><head><title>New Title</title></head><body>Body</body></html>";

    CHECK(SetTitle(html, L"New Title") == expected);
    }

// -----------------------------------------------------------------------------
// set_title: inserts <title> into existing <head> when none present
// -----------------------------------------------------------------------------
TEST_CASE("set_title inserts title into existing head", "[html][set_title]")
    {
    const std::wstring html =
        L"<html><head></head><body>Body</body></html>";
    const std::wstring expected =
        L"<html><head>\n<title>New Title</title></head><body>Body</body></html>";

    CHECK(SetTitle(html, L"New Title") == expected);
    }

// -----------------------------------------------------------------------------
// set_title: inserts <head> (after <html ...>) and then <title> when no head exists
// -----------------------------------------------------------------------------
TEST_CASE("set_title inserts head and title if head is missing", "[html][set_title]")
    {
    const std::wstring html =
        L"<html lang=\"en\">\n<body>Hi</body></html>";
    // After inserting "\n<head></head>\n" right after the closing '>' of <html ...>,
    // set_title then inserts "\n<title></title>" after <head>, and writes the text.
    const std::wstring expected =
        L"<html lang=\"en\">\n<head>\n<title>New</title></head>\n\n<body>Hi</body></html>";

    CHECK(SetTitle(html, L"New") == expected);
    }

// -----------------------------------------------------------------------------
// set_title: bogus HTML (no <html>) → unchanged
// -----------------------------------------------------------------------------
TEST_CASE("set_title returns unchanged for bogus HTML (no <html>)", "[html][set_title]")
    {
    const std::wstring html = L"<head><title>Y</title></head>";
    CHECK(SetTitle(html, L"Y") == html);
    }

// -----------------------------------------------------------------------------
// set_title: malformed <html ... without closing '>'
// -----------------------------------------------------------------------------
TEST_CASE("set_title when <html> has no closing bracket", "[html][set_title]")
    {
    const std::wstring html = L"<html lang=\"en\" <head><title>Z</title>\n</head><body/>";
    CHECK(SetTitle(html, L"Z") == html);
    }

// -----------------------------------------------------------------------------
// set_encoding: inserts <meta http-equiv... charset=ENC> into existing <head> (no meta)
// -----------------------------------------------------------------------------
TEST_CASE("set_encoding inserts meta in existing head when missing", "[html][set_encoding]")
    {
    const std::wstring html =
        L"<html><head></head><body>Body</body></html>";
    const std::wstring expected =
        L"<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\" /></head><body>Body</body></html>";

    CHECK(SetEncoding(html) == expected);
    }

// -----------------------------------------------------------------------------
// set_encoding: inserts <head> and then meta when head is missing
// -----------------------------------------------------------------------------
TEST_CASE("set_encoding inserts head and meta when head missing", "[html][set_encoding]")
    {
    const std::wstring html =
        L"<html>\n<body>Body</body></html>";
    const std::wstring expected =
        L"<html>\n<head><meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\" /></head>\n\n<body>Body</body></html>";

    CHECK(SetEncoding(html, L"ISO-8859-1") == expected);
    }

// -----------------------------------------------------------------------------
// set_encoding: bogus HTML (no <html>)
// -----------------------------------------------------------------------------
TEST_CASE("set_encoding (no <html>)", "[html][set_encoding]")
    {
    const std::wstring html = L"<head><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\" />\n</head><body></body>";
    CHECK(SetEncoding(html) == html);
    }

// -----------------------------------------------------------------------------
// set_encoding: malformed <head ... without closing '>'
// -----------------------------------------------------------------------------
TEST_CASE("set_encoding when <head> has no closing bracket", "[html][set_encoding]")
    {
    const std::wstring html = L"<html><head lang=\"en\" <body><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-16\" /></body></html>";
    CHECK(SetEncoding(html, L"UTF-16") == html);
    }

TEST_CASE("set_title overshoots with nested tags in <title>", "[html][set_title]")
    {
    std::wstring html =
        L"<html><head><title>Old <b>stuff</b></title></head><body></body></html>";

    // What we *want* is the title replaced cleanly:
    std::wstring expected =
        L"<html><head><title>NewTitle</title></head><body></body></html>";

    lily_of_the_valley::html_format::set_title(html, L"NewTitle");

    CHECK(html == expected);
    }

// NOLINTEND
// clang-format on
