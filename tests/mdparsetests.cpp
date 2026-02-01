// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/markdown_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("Markdown Parser Meta", "[md import]")
    {
    SECTION("Meta Sections")
        {
        lily_of_the_valley::markdown_extract_text md;
        // YAML
        auto mdText = L"---\n   title:my book\n\nHere is the *actual* \\*text to **review**.";
        CHECK(md.has_metadata_section(mdText));
        }

    SECTION("Meta Section End")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"---\ntitle:my book\n---\nHere is the *actual* text to **review**." }) } ==
            std::wstring{ L"Here is the actual text to review." });
        }

    SECTION("Newlines")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{
                  md({ L"---\ntitle:my book\n---\nHere is the *actual* text to **review**." }) } ==
              std::wstring{ L"Here is the actual text to review." });
        }
    }

TEST_CASE("Markdown Parser HTML", "[md import]")
    {
    SECTION("HTML List")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ LR"(following list:

<ul>
<li>Interval</li>
<li>Period</li>
<li>Int</li>
<li>More</li>
</ul>

The End.)" }) } ==
             std::wstring{ LR"(following list:



 
	Interval 
	Period 
	Int 
	More 


 The End.)" });
        }

    SECTION("HTML Table")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ LR"(following table:

<table>
<tr>
<th>Interval</th>
<th>Period</th>
<th>Int</th>
<th>More</th>
</tr>
<tr>
<td>0 to 20</td>
<td>8</td>
<td>Supported.</td>
<td>Not supported.</td>
</tr>
</table>

The End.)" }) } ==
             std::wstring{ LR"(following table:



 

 Interval Period Int More  

 	0 to 20 	8 	Supported. 	Not supported.  


 The End.)" });
        }
    SECTION("HTML angle")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"This is <em>bold</em>." }) } ==
              std::wstring{ L"This is bold." });
        CHECK(std::wstring{ md({ L"This is <a href=\"#bm\">bold</a>." }) } ==
            std::wstring{ L"This is bold." });
        CHECK(std::wstring{ md({ L"<dl> <dt>**PGF\\_HOT**</dt> </dl>" }) } ==
              std::wstring{ L" PGF_HOT " });
        CHECK(std::wstring{ md({ L"2 > 1" }) } ==
            std::wstring{ L"2 > 1" });
        CHECK(std::wstring{ md({ L"2 < 5" }) } ==
            std::wstring{ L"2 < 5" });
        CHECK(std::wstring{ md({ L"Go to <https://website> for more info." }) } ==
            std::wstring{ L"Go to <https://website> for more info." });
        }

    SECTION("HTML tags")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Hi &gt; there &amp; you &quot;." }) } ==
              std::wstring{ L"Hi > there & you \"." });
        CHECK(std::wstring{ md({ L"You &amp me." }) } ==
            std::wstring{ L"You &amp me." });
        CHECK(std::wstring{ md({ L"organization&#39;s" }) } ==
            std::wstring{ L"organization's" });
        CHECK(std::wstring{ md({ L"organization&#X27;s" }) } ==
            std::wstring{ L"organization's" });
        }
    }

TEST_CASE("Markdown Parser Code", "[md import]")
    {
    SECTION("Inline code")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"This `is code`." }) } ==
              std::wstring{ L"This is code." });
        CHECK(std::wstring{ md({ L"This ```is code```." }) } ==
            std::wstring{ L"This is code." });
        CHECK(std::wstring{ md({ L"Code `r 2+2`." }) } ==
              std::wstring{ L"Code 2+2." });
        CHECK(std::wstring{ md({ L"``2`2`` `shared_ptr`" }) } ==
              std::wstring{ L"2`2 shared_ptr" });
        CHECK(std::wstring{ md({ L"### `std::basic_istream::read` processing of `\\r\\n`` =>`\\n`\n `shared_ptr`" }) } ==
              std::wstring{ L"std::basic_istream::read processing of \\r\\n =>n\n shared_ptr" });
        }

    SECTION("Code block")
        {
        lily_of_the_valley::markdown_extract_text md;
        // inline (you aren't supposed to do this with ```, but people do)
        CHECK(std::wstring{ md({ L"This\n```\nis code\r\nhere```\n." }) } ==
              std::wstring{ L"This \n\tis code\r\n\there\n\n." });
        // remove lang info
        auto blah = std::wstring{ md({ L"This\n```cpp\nis code\r\nhere\n```\n." }) };
        CHECK(std::wstring{ md({ L"This\n```cpp\nis code\r\nhere\n```\n." }) } ==
            std::wstring{ L"This \n\tis code\r\n\there\n\t\n\n." });
        }
    }

TEST_CASE("Markdown Parser Lists", "[md import]")
    {
    SECTION("Unordered Lists")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"- list one\nhere\n- Item 2\n\nSome -text." }) } ==
              std::wstring{ L"- list one here\n- Item 2\n\nSome -text." });
        }

    SECTION("Unordered Nested Lists")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"- list one\n  - Item 2" }) } ==
              std::wstring{ L"- list one\n  - Item 2" });
        }

    SECTION("Ordered Lists")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"1. list one\nhere\n256. Item\n2" }) } ==
              std::wstring{ L"1. list one here\n256. Item 2" });
        }
    }

TEST_CASE("Markdown Parser Links", "[md import]")
    {
    SECTION("Links")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"go to [https://visualstudio.microsoft.com/vs/pricing/](https://visualstudio.microsoft.com/vs/pricing/) to explore." }) } ==
              std::wstring{ L"go to https://visualstudio.microsoft.com/vs/pricing/ to explore." });
        CHECK(std::wstring{ md({ L"Tux [the Linux mascot](/assets/tux.png) the penguin." }) } ==
              std::wstring{ L"Tux the Linux mascot the penguin." });
        CHECK(std::wstring{ md({ L"Tux [the **Linux** mascot](/assets/tux.png) the penguin." }) } ==
            std::wstring{ L"Tux the Linux mascot the penguin." });
        // malformed
        CHECK(std::wstring{ md({ L"Tux [the Linux mascot" }) } ==
            std::wstring{ L"Tux [the Linux mascot" });
        CHECK(std::wstring{ md({ L"Tux [the Linux mascot](/assets/tux.png" }) } ==
            std::wstring{ L"Tux [the Linux mascot](/assets/tux.png" });
        CHECK(std::wstring{ md({ L"The third member function inserts the sequence [`first`, `last`). You use it" }) } ==
            std::wstring{ L"The third member function inserts the sequence [first, last). You use it" });
        // missing link
        CHECK(std::wstring{ md({ L"as an **[out]** parameter." }) } ==
            std::wstring{ L"as an [out] parameter." });
        }

    SECTION("Formatted link")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"`shared_ptr`" }) } ==
            std::wstring{ L"shared_ptr" });
        CHECK(std::wstring{ md({ L"[`shared_ptr`](www.website)" }) } ==
              std::wstring{ L"shared_ptr" });
        }

    SECTION("Images")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Tux ![Tux, the Linux mascot](/assets/tux.png) the penguin." }) } ==
              std::wstring{ L"Tux  the penguin." });
        // malformed
        CHECK(std::wstring{ md({ L"Tux ![Tux, the Linux mascot" }) } ==
            std::wstring{ L"Tux [Tux, the Linux mascot" });
        CHECK(std::wstring{ md({ L"Tux ![Tux, the Linux mascot](/assets/tux.png" }) } ==
            std::wstring{ L"Tux (/assets/tux.png" });
        }
    }

TEST_CASE("Markdown Parser Styling", "[md import]")
    {
    SECTION("Emphasis")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"This is *italic* and **bold** and also __italic__. 2 \\* 2." }) } ==
              std::wstring{ L"This is italic and bold and also italic. 2 * 2." });
        CHECK(std::wstring{ md({ L"This is _italic and **bold** text_." }) } ==
            std::wstring{ L"This is italic and bold text." });
        CHECK(std::wstring{ md({ L"**PGF\\_HOT**" }) } ==
            std::wstring{ L"PGF_HOT" });
        CHECK(std::wstring{ md({ L"TIFF _spe_ci**f**i_c_ *options*" }) } ==
            std::wstring{ L"TIFF specific options" });
        CHECK(std::wstring{ md({ L"2 * 2" }) } ==
            std::wstring{ L"2 * 2" });
        CHECK(std::wstring{ md({ L"A **_variant_\\_t** _object_" }) } ==
            std::wstring{ L"A variant_t object" });
        }
    
    SECTION("Emphasis (overlapping)")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"**Formalize the routine, use a *top-down* approach.**" }) } ==
              std::wstring{ L"Formalize the routine, use a top-down approach." });
        CHECK(std::wstring{ md({ L"*Formalize the routine, use a **top-down** approach.*" }) } ==
              std::wstring{ L"Formalize the routine, use a top-down approach." });
        // malformed
        CHECK(std::wstring{ md({ L"*Formalize the routine, use a **top-down approach.*" }) } ==
              std::wstring{ L"Formalize the routine, use a top-down approach." });
        CHECK(std::wstring{ md({ L"**Formalize the routine, use a *top-down* approach." }) } ==
              std::wstring{ L"Formalize the routine, use a top-down approach." });
        CHECK(std::wstring{ md({ L"**Formalize the routine, use a *top-down approach." }) } ==
              std::wstring{ L"Formalize the routine, use a top-down approach." });
        }

    SECTION("Block quotes")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"> This is a quote\n\n>\n\n> End of Quote" }) } ==
              std::wstring{ L"\tThis is a quote\n\n\t\n\n\tEnd of Quote" });
        // nested
        CHECK(std::wstring{ md({ L"> This is a quote\n\n>\n\n>> End of Quote" }) } ==
            std::wstring{ L"\tThis is a quote\n\n\t\n\n\t\tEnd of Quote" });
        // with header
        CHECK(std::wstring{ md({ L"> # This is a quote header\n\n>\n\n>> End of Quote" }) } ==
            std::wstring{ L"\tThis is a quote header\n\n\t\n\n\t\tEnd of Quote" });
        // indented
        CHECK(std::wstring{ md({ L"    This is a quote\n    End of Quote" }) } ==
            std::wstring{ L"    This is a quote\n    End of Quote" });
        }

    SECTION("Superscript")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"5^th^ edition \\^5" }) } ==
              std::wstring{ L"5th edition ^5" });
        }
    }

TEST_CASE("Markdown Parser Table", "[md import]")
    {
    SECTION("Table")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"| Syntax | Description |\n| --- | ----------- |\n| Header | Title |" }) } ==
              std::wstring{ L"\t| Syntax \t| Description \t|\n\t| Header \t| Title \t|" });
        CHECK(std::wstring{ md({ L"| Syntax | Description |\n| :-- | ----------: |\n| Header | Title |" }) } ==
              std::wstring{ L"\t| Syntax \t| Description \t|\n\t| Header \t| Title \t|" });
        // boundary check
        CHECK(std::wstring{ md({ L"| Syntax | Description |\n| :-- | ----------: |" }) } ==
              std::wstring{ L"\t| Syntax \t| Description \t|\n" });
        }
    }

TEST_CASE("Markdown Parser Header", "[md import]")
    {
    SECTION("Null")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK_FALSE(md.has_metadata_section({L""}));
        CHECK(md({ L"some MD text", 0 }) == nullptr);
        }

    SECTION("Header")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"# Header1\n### Header2\n Not a #header" }) } ==
              std::wstring{ L"Header1\n\nHeader2\n\n Not a #header" });
        CHECK(std::wstring{ md({ L"Header1\n=========\nHeader2\n--\nNot a =header" }) } ==
            std::wstring{ L"Header1\n\nHeader2\n\nNot a =header" });
        CHECK(std::wstring{ md({ L"# Header1 {.unnumbered}\nText" }) } ==
            std::wstring{ L"Header1 \n\nText" });
        CHECK(std::wstring{ md({ L"\n\n# Header1\n### Header2\n Not a #header" }) } ==
              std::wstring{ L"\n\nHeader1\n\nHeader2\n\n Not a #header" });
        CHECK(std::wstring{ md({ L"\n# Header1\n### Header2\n Not a #header" }) } ==
              std::wstring{ L"\nHeader1\n\nHeader2\n\n Not a #header" });
        CHECK(std::wstring{ md({ L"Some content\n# Header1\n### Header2\n Not a #header" }) } ==
              std::wstring{ L"Some content\nHeader1\n\nHeader2\n\n Not a #header" });
        }
    }

TEST_CASE("Markdown Parser Quarto Shortcodes", "[md import]")
    {
    SECTION("Kbd")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Press {{< kbd Shift-Ctrl-P >}} to open." }) } ==
              std::wstring{ L"Press SHIFT-CTRL-P to open." });
        }

    SECTION("Kbd multi-platform")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Use {{< kbd mac=Shift-Command-O win=Shift-Control-O >}} here." }) } ==
              std::wstring{ L"Use MAC=SHIFT-COMMAND-O WIN=SHIFT-CONTROL-O here." });
        }

    SECTION("Meta")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"The {{< meta title >}} is shown." }) } ==
              std::wstring{ L"The TITLE is shown." });
        }

    SECTION("Var")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Version {{< var version >}} released." }) } ==
              std::wstring{ L"Version VERSION released." });
        }

    SECTION("Env")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Home is {{< env HOME >}} here." }) } ==
              std::wstring{ L"Home is HOME here." });
        }

    SECTION("Pagebreak")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Before{{< pagebreak >}}After" }) } ==
              std::wstring{ L"Before\n\nAfter" });
        }

    SECTION("Video")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"See {{< video https://example.com >}} here." }) } ==
              std::wstring{ L"See https://example.com here." });
        }

    SECTION("Unknown shortcode stripped")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"See {{< lipsum 3 >}} here." }) } ==
              std::wstring{ L"See  here." });
        CHECK(std::wstring{ md({ L"See {{< bogus some args >}} here." }) } ==
              std::wstring{ L"See  here." });
        }

    SECTION("Empty shortcode")
        {
        lily_of_the_valley::markdown_extract_text md;
        // {{<  >}} with just whitespace inside
        CHECK(std::wstring{ md({ L"See {{<  >}} here." }) } ==
              std::wstring{ L"See  here." });
        }

    SECTION("Shortcode name only, no value")
        {
        lily_of_the_valley::markdown_extract_text md;
        // kbd with no arguments
        CHECK(std::wstring{ md({ L"Press {{< kbd >}} now." }) } ==
              std::wstring{ L"Press  now." });
        CHECK(std::wstring{ md({ L"The {{< meta>}} value." }) } ==
              std::wstring{ L"The  value." });
        CHECK(std::wstring{ md({ L"The {{<var>}} value." }) } ==
              std::wstring{ L"The  value." });
        CHECK(std::wstring{ md({ L"The {{< env >}} value." }) } ==
              std::wstring{ L"The  value." });
        CHECK(std::wstring{ md({ L"See {{< video >}} here." }) } ==
              std::wstring{ L"See  here." });
        }

    SECTION("Malformed shortcode, missing closing")
        {
        lily_of_the_valley::markdown_extract_text md;
        // missing >}}, parser should log error and stop
        const auto* result = md({ L"See {{< kbd Ctrl-C here." });
        CHECK(result != nullptr);
        CHECK(md.get_log().find(L"Bad Quarto shortcode") != std::wstring::npos);
        }

    SECTION("Multiple shortcodes in one line")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Press {{< kbd Ctrl-C >}} then {{< kbd Ctrl-V >}} to paste." }) } ==
              std::wstring{ L"Press CTRL-C then CTRL-V to paste." });
        }
    }

TEST_CASE("Markdown Parser Math", "[md import]")
    {
    SECTION("Inline math")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"The equation $x+y=z$ is simple." }) } ==
              std::wstring{ L"The equation x+y=z is simple." });
        CHECK(std::wstring{ md({ L"Where $a^2 + b^2 = c^2$ holds." }) } ==
              std::wstring{ L"Where a^2 + b^2 = c^2 holds." });
        // LaTeX relational operators
        CHECK(std::wstring{ md({ L"$x \\leq y$" }) } ==
              std::wstring{ L"x \u2264 y" });
        CHECK(std::wstring{ md({ L"$x \\geq y$" }) } ==
              std::wstring{ L"x \u2265 y" });
        CHECK(std::wstring{ md({ L"$a \\neq b$" }) } ==
              std::wstring{ L"a \u2260 b" });
        CHECK(std::wstring{ md({ L"$a \\approx b$" }) } ==
              std::wstring{ L"a \u2248 b" });
        CHECK(std::wstring{ md({ L"$a \\equiv b$" }) } ==
              std::wstring{ L"a \u2261 b" });
        // Greek letters
        CHECK(std::wstring{ md({ L"$\\alpha + \\beta$" }) } ==
              std::wstring{ L"\u03B1 + \u03B2" });
        CHECK(std::wstring{ md({ L"$\\Sigma$" }) } ==
              std::wstring{ L"\u03A3" });
        CHECK(std::wstring{ md({ L"$\\pi r^2$" }) } ==
              std::wstring{ L"\u03C0 r^2" });
        CHECK(std::wstring{ md({ L"$\\theta + \\phi$" }) } ==
              std::wstring{ L"\u03B8 + \u03C6" });
        CHECK(std::wstring{ md({ L"$\\Omega$" }) } ==
              std::wstring{ L"\u03A9" });
        CHECK(std::wstring{ md({ L"$\\Delta x$" }) } ==
              std::wstring{ L"\u0394 x" });
        // Large operators
        CHECK(std::wstring{ md({ L"$\\sum_{i=0}^{n} x$" }) } ==
              std::wstring{ L"\u2211_{i=0}^{n} x" });
        CHECK(std::wstring{ md({ L"$\\prod_{i=1}^{n} x_i$" }) } ==
              std::wstring{ L"\u220F_{i=1}^{n} x_i" });
        CHECK(std::wstring{ md({ L"$\\int_0^1 f(x) dx$" }) } ==
              std::wstring{ L"\u222B_0^1 f(x) dx" });
        CHECK(std::wstring{ md({ L"$\\partial f$" }) } ==
              std::wstring{ L"\u2202 f" });
        // Arrows
        CHECK(std::wstring{ md({ L"$x \\to y$" }) } ==
              std::wstring{ L"x \u2192 y" });
        CHECK(std::wstring{ md({ L"$A \\Rightarrow B$" }) } ==
              std::wstring{ L"A \u21D2 B" });
        CHECK(std::wstring{ md({ L"$A \\Leftrightarrow B$" }) } ==
              std::wstring{ L"A \u21D4 B" });
        CHECK(std::wstring{ md({ L"$f: X \\mapsto Y$" }) } ==
              std::wstring{ L"f: X \u21A6 Y" });
        // Set/logic operators
        CHECK(std::wstring{ md({ L"$x \\in S$" }) } ==
              std::wstring{ L"x \u2208 S" });
        CHECK(std::wstring{ md({ L"$A \\cup B$" }) } ==
              std::wstring{ L"A \u222A B" });
        CHECK(std::wstring{ md({ L"$A \\cap B$" }) } ==
              std::wstring{ L"A \u2229 B" });
        CHECK(std::wstring{ md({ L"$A \\subseteq B$" }) } ==
              std::wstring{ L"A \u2286 B" });
        CHECK(std::wstring{ md({ L"$\\forall x \\exists y$" }) } ==
              std::wstring{ L"\u2200 x \u2203 y" });
        CHECK(std::wstring{ md({ L"$\\emptyset$" }) } ==
              std::wstring{ L"\u2205" });
        // Misc symbols
        CHECK(std::wstring{ md({ L"$\\infty$" }) } ==
              std::wstring{ L"\u221E" });
        CHECK(std::wstring{ md({ L"$a \\pm b$" }) } ==
              std::wstring{ L"a \u00B1 b" });
        CHECK(std::wstring{ md({ L"$a \\times b$" }) } ==
              std::wstring{ L"a \u00D7 b" });
        CHECK(std::wstring{ md({ L"$a \\cdot b$" }) } ==
              std::wstring{ L"a \u00B7 b" });
        CHECK(std::wstring{ md({ L"$\\sqrt{x}$" }) } ==
              std::wstring{ L"\u221A{x}" });
        CHECK(std::wstring{ md({ L"$\\nabla f$" }) } ==
              std::wstring{ L"\u2207 f" });
        CHECK(std::wstring{ md({ L"$\\ldots$" }) } ==
              std::wstring{ L"\u2026" });
        // Multiple commands in one equation
        CHECK(std::wstring{ md({ L"$\\alpha \\leq \\beta \\to \\infty$" }) } ==
              std::wstring{ L"\u03B1 \u2264 \u03B2 \u2192 \u221E" });
        // Unknown command left as-is
        CHECK(std::wstring{ md({ L"$\\frac{a}{b}$" }) } ==
              std::wstring{ L"\\frac{a}{b}" });
        // Backslash not followed by letters left as-is
        CHECK(std::wstring{ md({ L"$a \\+ b$" }) } ==
              std::wstring{ L"a \\+ b" });
        CHECK(std::wstring{ md({ L"$a \\ b$" }) } ==
              std::wstring{ L"a \\ b" });
        }

    SECTION("Display math")
        {
        lily_of_the_valley::markdown_extract_text md;
        CHECK(std::wstring{ md({ L"Before\n$$\nx = 5\n$$\nAfter" }) } ==
              std::wstring{ L"Before x = 5 After" });
        // display math with LaTeX commands
        CHECK(std::wstring{ md({ L"$$\\sum_{i=0}^{\\infty} \\alpha_i$$" }) } ==
              std::wstring{ L"\u2211_{i=0}^{\u221E} \u03B1_i" });
        }

    SECTION("Dollar sign not math")
        {
        lily_of_the_valley::markdown_extract_text md;
        // space after opening $ means not math
        CHECK(std::wstring{ md({ L"The price is $ 5.00 today." }) } ==
              std::wstring{ L"The price is $ 5.00 today." });
        // no closing $, not math
        CHECK(std::wstring{ md({ L"I have $5 in my pocket." }) } ==
              std::wstring{ L"I have $5 in my pocket." });
        // closing $ followed by digit, not math
        CHECK(std::wstring{ md({ L"between $5 and $10 range" }) } ==
              std::wstring{ L"between $5 and $10 range" });
        // tab after opening $
        CHECK(std::wstring{ md({ L"cost $\t50$ here" }) } ==
              std::wstring{ L"cost $\t50$ here" });
        // space before closing $
        CHECK(std::wstring{ md({ L"the $value $ is odd" }) } ==
              std::wstring{ L"the $value $ is odd" });
        }

    SECTION("Inline math boundary")
        {
        lily_of_the_valley::markdown_extract_text md;
        // equation at very start of input
        CHECK(std::wstring{ md({ L"$x$" }) } ==
              std::wstring{ L"x" });
        // equation at very end of input
        CHECK(std::wstring{ md({ L"see $x$" }) } ==
              std::wstring{ L"see x" });
        // back-to-back equations
        CHECK(std::wstring{ md({ L"$a$$b$" }) } ==
              std::wstring{ L"ab" });
        // single character equation
        CHECK(std::wstring{ md({ L"the $x$ axis" }) } ==
              std::wstring{ L"the x axis" });
        // equation with special markdown chars inside
        CHECK(std::wstring{ md({ L"$a*b*c$" }) } ==
              std::wstring{ L"a*b*c" });
        }

    SECTION("Display math boundary")
        {
        lily_of_the_valley::markdown_extract_text md;
        // inline style (no newlines)
        CHECK(std::wstring{ md({ L"$$x = 5$$" }) } ==
              std::wstring{ L"x = 5" });
        // empty display block
        CHECK(std::wstring{ md({ L"$$$$" }) } ==
              std::wstring{ L"" });
        // display math at end of input
        CHECK(std::wstring{ md({ L"see\n$$\nx+1\n$$" }) } ==
              std::wstring{ L"see x+1" });
        }

    SECTION("Malformed math")
        {
        lily_of_the_valley::markdown_extract_text md;
        // unclosed display math, reads $$ as literal
        CHECK(std::wstring{ md({ L"Before $$x = 5 and no close" }) } ==
              std::wstring{ L"Before $$x = 5 and no close" });
        CHECK(md.get_log().find(L"Bad display math") != std::wstring::npos);
        // lone $ at end of input
        CHECK(std::wstring{ md({ L"trailing $" }) } ==
              std::wstring{ L"trailing $" });
        // $$ without closing, reads as literal
        CHECK(std::wstring{ md({ L"a $$ b" }) } ==
              std::wstring{ L"a $$ b" });
        CHECK(md.get_log().find(L"Bad display math") != std::wstring::npos);
        }
    }

// NOLINTEND
// clang-format on
