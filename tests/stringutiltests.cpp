#include "../src/util/string_util.h"
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <iomanip>
#include <set>
#include <sstream>

// NOLINTBEGIN
using namespace string_util;
using namespace Catch::Matchers;

// clang-format off
TEST_CASE("find_unescaped_char", "[stringutil][search]")
    {
    std::wstring_view st{ L"Hello there!" };
    CHECK(nullptr == find_unescaped_char(st.data(), L'#'));
    st = L"";
    CHECK(nullptr == find_unescaped_char(st.data(), L'#'));
    st = LR"(\#)";
    CHECK(nullptr == find_unescaped_char(st.data(), L'#'));
    st = LR"(\\#)";
    CHECK(find_unescaped_char(st.data(), L'#') == st.data() + 2);
    st = LR"(\\\\\\\\#)";
    CHECK(find_unescaped_char(st.data(), L'#') == st.data() + 8);
    st = LR"(\#\#\\#)";
    CHECK(find_unescaped_char(st.data(), L'#') == st.data() + 6);
    st = LR"(  abc#)";
    CHECK(find_unescaped_char(st.data(), L'#') == st.data() + 5);
    st = LR"(#)";
    CHECK(find_unescaped_char(st.data(), L'#') == st.data());
    st = LR"(Hello there#world#)";
    CHECK(find_unescaped_char(st.data(), L'#') == st.data() + 11);
    st = L"Hello there\n\n#world#";
    CHECK(find_unescaped_char(st.data(), L'#') == st.data() + 13);
    }

TEST_CASE("find_unescaped_char_same_line_n", "[stringutil][search]")
    {
    SECTION("Full scan")
        {
        std::wstring_view st{ L"Hello there!" };
        CHECK(nullptr == find_unescaped_char_same_line_n(st.data(), L'#', st.size()));
        st = L"";
        CHECK(nullptr == find_unescaped_char_same_line_n(st.data(), L'#', st.size()));
        st = LR"(\#)";
        CHECK(nullptr == find_unescaped_char_same_line_n(st.data(), L'#', st.size()));
        st = LR"(\\#)";
        CHECK(find_unescaped_char_same_line_n(st.data(), L'#', st.size()) == st.data() + 2);
        st = LR"(\\\\\\\\#)";
        CHECK(find_unescaped_char_same_line_n(st.data(), L'#', st.size()) == st.data() + 8);
        st = LR"(\#\#\\#)";
        CHECK(find_unescaped_char_same_line_n(st.data(), L'#', st.size()) == st.data() + 6);
        st = LR"(  abc#)";
        CHECK(find_unescaped_char_same_line_n(st.data(), L'#', st.size()) == st.data() + 5);
        st = LR"(#)";
        CHECK(find_unescaped_char_same_line_n(st.data(), L'#', st.size()) == st.data());
        st = LR"(Hello there#world#)";
        CHECK(find_unescaped_char_same_line_n(st.data(), L'#', st.size()) == st.data() + 11);
        st = L"Hello there\n\n#world#";
        CHECK(nullptr == find_unescaped_char_same_line_n(st.data(), L'#', st.size()) );
        }
    SECTION("Partial scan")
        {
        std::wstring_view st = LR"(\\#)";
        CHECK(nullptr == find_unescaped_char_same_line_n(st.data(), L'#', 1));
        st = LR"(\\\\\\\\#)";
        CHECK(nullptr == find_unescaped_char_same_line_n(st.data(), L'#', 7));
        // too far
        st = LR"(\\\\\\\\)";
        CHECK(nullptr == find_unescaped_char_same_line_n(st.data(), L'#', 89));
        st = LR"(Hello there#world#)";
        CHECK(find_unescaped_char_same_line_n(st.data(), L'#', 89) == st.data() + 11);
        }
    }

TEST_CASE("find_unescaped_char_n", "[stringutil][search]")
    {
    SECTION("Full scan")
        {
        std::wstring_view st{ L"Hello there!" };
        CHECK(nullptr == find_unescaped_char_n(st.data(), L'#', st.size()));
        st = L"";
        CHECK(nullptr == find_unescaped_char_n(st.data(), L'#', st.size()));
        st = LR"(\#)";
        CHECK(nullptr == find_unescaped_char_n(st.data(), L'#', st.size()));
        st = LR"(\\#)";
        CHECK(find_unescaped_char_n(st.data(), L'#', st.size()) == st.data() + 2);
        st = LR"(\\\\\\\\#)";
        CHECK(find_unescaped_char_n(st.data(), L'#', st.size()) == st.data() + 8);
        st = LR"(\#\#\\#)";
        CHECK(find_unescaped_char_n(st.data(), L'#', st.size()) == st.data() + 6);
        st = LR"(  abc#)";
        CHECK(find_unescaped_char_n(st.data(), L'#', st.size()) == st.data() + 5);
        st = LR"(#)";
        CHECK(find_unescaped_char_n(st.data(), L'#', st.size()) == st.data());
        st = LR"(Hello there#world#)";
        CHECK(find_unescaped_char_n(st.data(), L'#', st.size()) == st.data() + 11);
        st = L"Hello there\n\n#world#";
        CHECK(find_unescaped_char_n(st.data(), L'#', st.size()) == st.data() + 13);
        }
    SECTION("Partial scan")
        {
        std::wstring_view st = LR"(\\#)";
        CHECK(nullptr == find_unescaped_char_n(st.data(), L'#', 1));
        st = LR"(\\#)";
        CHECK(nullptr == find_unescaped_char_n(st.data(), L'#', 2));
        st = LR"(\\#)";
        CHECK(st.data() + 2 == find_unescaped_char_n(st.data(), L'#', 3));
        st = LR"(\\\\\\\\#)";
        CHECK(nullptr == find_unescaped_char_n(st.data(), L'#', 7));
        // too far
        st = LR"(\\\\\\\\)";
        CHECK(nullptr == find_unescaped_char_n(st.data(), L'#', 89));
        st = LR"(Hello there#world#)";
        CHECK(find_unescaped_char_n(st.data(), L'#', 89) == st.data() + 11);
        }
    }

TEST_CASE("full_width_to_narrow", "[stringutil][fullwidth]")
	{
	SECTION("Punctuation")
		{
        CHECK(L'!' == full_width_to_narrow(L'!'));
        CHECK(L'!' == full_width_to_narrow(L'！'));
        CHECK(L'"' == full_width_to_narrow(L'＂'));
        CHECK(L'#' == full_width_to_narrow(L'＃'));
        CHECK(L'$' == full_width_to_narrow(L'＄'));
        CHECK(L'%' == full_width_to_narrow(L'％'));
        CHECK(L'&' == full_width_to_narrow(L'＆'));
        CHECK(L'\'' == full_width_to_narrow(L'＇'));
        CHECK(L'(' == full_width_to_narrow(L'（'));
        CHECK(L')' == full_width_to_narrow(L'）'));
        CHECK(L'*' == full_width_to_narrow(L'＊'));
        CHECK(L'+' == full_width_to_narrow(L'＋'));
        CHECK(L',' == full_width_to_narrow(L'，'));
        CHECK(L'-' == full_width_to_narrow(L'－'));
        CHECK(L'.' == full_width_to_narrow(L'．'));
        CHECK(L'/' == full_width_to_narrow(L'／'));
        CHECK(L':' == full_width_to_narrow(L'：'));
        CHECK(L';' == full_width_to_narrow(L'；'));
        CHECK(L'<' == full_width_to_narrow(L'＜'));
        CHECK(L'=' == full_width_to_narrow(L'＝'));
        CHECK(L'>' == full_width_to_narrow(L'＞'));
        CHECK(L'?' == full_width_to_narrow(L'？'));
        CHECK(L'@' == full_width_to_narrow(L'＠'));
        CHECK(L'[' == full_width_to_narrow(L'［'));
        CHECK(L'\\' == full_width_to_narrow(L'＼'));
        CHECK(L']' == full_width_to_narrow(L'］'));
        CHECK(L'^' == full_width_to_narrow(L'＾'));
        CHECK(L'_' == full_width_to_narrow(L'＿'));
        CHECK(L'`' == full_width_to_narrow(L'｀'));
        CHECK(L'{' == full_width_to_narrow(L'｛'));
        CHECK(L'|' == full_width_to_narrow(L'｜'));
        CHECK(L'}' == full_width_to_narrow(L'｝'));
        CHECK(L'~' == full_width_to_narrow(L'～'));
        CHECK(L'¢' == full_width_to_narrow(L'￠'));
        CHECK(L'£' == full_width_to_narrow(L'￡'));
        CHECK(L'¥' == full_width_to_narrow(L'￥'));
        CHECK(L'¬' == full_width_to_narrow(L'￢'));
        CHECK(L'¯' == full_width_to_narrow(L'￣'));
        CHECK(L'¦' == full_width_to_narrow(L'￤'));
		}
    SECTION("Numbers")
        {
        CHECK(L'0'==full_width_to_narrow(L'０'));
        CHECK(L'1'==full_width_to_narrow(L'１'));
        CHECK(L'2'==full_width_to_narrow(L'２'));
        CHECK(L'3'==full_width_to_narrow(L'３'));
        CHECK(L'4'==full_width_to_narrow(L'４'));
        CHECK(L'5'==full_width_to_narrow(L'５'));
        CHECK(L'6'==full_width_to_narrow(L'６'));
        CHECK(L'7'==full_width_to_narrow(L'７'));
        CHECK(L'8'==full_width_to_narrow(L'８'));
        CHECK(L'9'==full_width_to_narrow(L'９'));

        CHECK(L'0'==full_width_to_narrow(L'0'));
        CHECK(L'1'==full_width_to_narrow(L'1'));
        CHECK(L'2'==full_width_to_narrow(L'2'));
        CHECK(L'3'==full_width_to_narrow(L'3'));
        CHECK(L'4'==full_width_to_narrow(L'4'));
        CHECK(L'5'==full_width_to_narrow(L'5'));
        CHECK(L'6'==full_width_to_narrow(L'6'));
        CHECK(L'7'==full_width_to_narrow(L'7'));
        CHECK(L'8'==full_width_to_narrow(L'8'));
        CHECK(L'9'==full_width_to_narrow(L'9'));
        }
    SECTION("Letters")
        {
        CHECK(L'a'==full_width_to_narrow(L'a'));
        CHECK(L'b'==full_width_to_narrow(L'b'));
        CHECK(L'c'==full_width_to_narrow(L'c'));
        CHECK(L'd'==full_width_to_narrow(L'd'));
        CHECK(L'e'==full_width_to_narrow(L'e'));
        CHECK(L'f'==full_width_to_narrow(L'f'));
        CHECK(L'g'==full_width_to_narrow(L'g'));
        CHECK(L'h'==full_width_to_narrow(L'h'));
        CHECK(L'i'==full_width_to_narrow(L'i'));
        CHECK(L'j'==full_width_to_narrow(L'j'));
        CHECK(L'k'==full_width_to_narrow(L'k'));
        CHECK(L'l'==full_width_to_narrow(L'l'));
        CHECK(L'm'==full_width_to_narrow(L'm'));
        CHECK(L'n'==full_width_to_narrow(L'n'));
        CHECK(L'o'==full_width_to_narrow(L'o'));
        CHECK(L'p'==full_width_to_narrow(L'p'));
        CHECK(L'q'==full_width_to_narrow(L'q'));
        CHECK(L'r'==full_width_to_narrow(L'r'));
        CHECK(L's'==full_width_to_narrow(L's'));
        CHECK(L't'==full_width_to_narrow(L't'));
        CHECK(L'u'==full_width_to_narrow(L'u'));
        CHECK(L'v'==full_width_to_narrow(L'v'));
        CHECK(L'w'==full_width_to_narrow(L'w'));
        CHECK(L'x'==full_width_to_narrow(L'x'));
        CHECK(L'y'==full_width_to_narrow(L'y'));
        CHECK(L'z'==full_width_to_narrow(L'z'));

        CHECK(L'A'==full_width_to_narrow(L'A'));
        CHECK(L'B'==full_width_to_narrow(L'B'));
        CHECK(L'C'==full_width_to_narrow(L'C'));
        CHECK(L'D'==full_width_to_narrow(L'D'));
        CHECK(L'E'==full_width_to_narrow(L'E'));
        CHECK(L'F'==full_width_to_narrow(L'F'));
        CHECK(L'G'==full_width_to_narrow(L'G'));
        CHECK(L'H'==full_width_to_narrow(L'H'));
        CHECK(L'I'==full_width_to_narrow(L'I'));
        CHECK(L'J'==full_width_to_narrow(L'J'));
        CHECK(L'K'==full_width_to_narrow(L'K'));
        CHECK(L'L'==full_width_to_narrow(L'L'));
        CHECK(L'M'==full_width_to_narrow(L'M'));
        CHECK(L'N'==full_width_to_narrow(L'N'));
        CHECK(L'O'==full_width_to_narrow(L'O'));
        CHECK(L'P'==full_width_to_narrow(L'P'));
        CHECK(L'Q'==full_width_to_narrow(L'Q'));
        CHECK(L'R'==full_width_to_narrow(L'R'));
        CHECK(L'S'==full_width_to_narrow(L'S'));
        CHECK(L'T'==full_width_to_narrow(L'T'));
        CHECK(L'U'==full_width_to_narrow(L'U'));
        CHECK(L'V'==full_width_to_narrow(L'V'));
        CHECK(L'W'==full_width_to_narrow(L'W'));
        CHECK(L'X'==full_width_to_narrow(L'X'));
        CHECK(L'Y'==full_width_to_narrow(L'Y'));
        CHECK(L'Z'==full_width_to_narrow(L'Z'));

        CHECK(L'œ'==full_width_to_narrow(L'œ'));

        CHECK(L'a'==full_width_to_narrow(L'ａ'));
        CHECK(L'b'==full_width_to_narrow(L'ｂ'));
        CHECK(L'c'==full_width_to_narrow(L'ｃ'));
        CHECK(L'd'==full_width_to_narrow(L'ｄ'));
        CHECK(L'e'==full_width_to_narrow(L'ｅ'));
        CHECK(L'f'==full_width_to_narrow(L'ｆ'));
        CHECK(L'g'==full_width_to_narrow(L'ｇ'));
        CHECK(L'h'==full_width_to_narrow(L'ｈ'));
        CHECK(L'i'==full_width_to_narrow(L'ｉ'));
        CHECK(L'j'==full_width_to_narrow(L'ｊ'));
        CHECK(L'k'==full_width_to_narrow(L'ｋ'));
        CHECK(L'l'==full_width_to_narrow(L'ｌ'));
        CHECK(L'm'==full_width_to_narrow(L'ｍ'));
        CHECK(L'n'==full_width_to_narrow(L'ｎ'));
        CHECK(L'o'==full_width_to_narrow(L'ｏ'));
        CHECK(L'p'==full_width_to_narrow(L'ｐ'));
        CHECK(L'q'==full_width_to_narrow(L'ｑ'));
        CHECK(L'r'==full_width_to_narrow(L'ｒ'));
        CHECK(L's'==full_width_to_narrow(L'ｓ'));
        CHECK(L't'==full_width_to_narrow(L'ｔ'));
        CHECK(L'u'==full_width_to_narrow(L'ｕ'));
        CHECK(L'v'==full_width_to_narrow(L'ｖ'));
        CHECK(L'w'==full_width_to_narrow(L'ｗ'));
        CHECK(L'x'==full_width_to_narrow(L'ｘ'));
        CHECK(L'y'==full_width_to_narrow(L'ｙ'));
        CHECK(L'z'==full_width_to_narrow(L'ｚ'));

        CHECK(L'A'==full_width_to_narrow(L'Ａ'));
        CHECK(L'B'==full_width_to_narrow(L'Ｂ'));
        CHECK(L'C'==full_width_to_narrow(L'Ｃ'));
        CHECK(L'D'==full_width_to_narrow(L'Ｄ'));
        CHECK(L'E'==full_width_to_narrow(L'Ｅ'));
        CHECK(L'F'==full_width_to_narrow(L'Ｆ'));
        CHECK(L'G'==full_width_to_narrow(L'Ｇ'));
        CHECK(L'H'==full_width_to_narrow(L'Ｈ'));
        CHECK(L'I'==full_width_to_narrow(L'Ｉ'));
        CHECK(L'J'==full_width_to_narrow(L'Ｊ'));
        CHECK(L'K'==full_width_to_narrow(L'Ｋ'));
        CHECK(L'L'==full_width_to_narrow(L'Ｌ'));
        CHECK(L'M'==full_width_to_narrow(L'Ｍ'));
        CHECK(L'N'==full_width_to_narrow(L'Ｎ'));
        CHECK(L'O'==full_width_to_narrow(L'Ｏ'));
        CHECK(L'P'==full_width_to_narrow(L'Ｐ'));
        CHECK(L'Q'==full_width_to_narrow(L'Ｑ'));
        CHECK(L'R'==full_width_to_narrow(L'Ｒ'));
        CHECK(L'S'==full_width_to_narrow(L'Ｓ'));
        CHECK(L'T'==full_width_to_narrow(L'Ｔ'));
        CHECK(L'U'==full_width_to_narrow(L'Ｕ'));
        CHECK(L'V'==full_width_to_narrow(L'Ｖ'));
        CHECK(L'W'==full_width_to_narrow(L'Ｗ'));
        CHECK(L'X'==full_width_to_narrow(L'Ｘ'));
        CHECK(L'Y'==full_width_to_narrow(L'Ｙ'));
        CHECK(L'Z'==full_width_to_narrow(L'Ｚ'));
        }
	}

TEST_CASE("replace_all", "[stringutil][replace_all]")
    {
    SECTION("NULLs")
        {
        std::wstring str{ L"text" };
        string_util::replace_all(str, nullptr, 0, L"replace");
        CHECK(std::wstring(L"text") == str);

        string_util::replace_all(str, L"l", 1, nullptr);
        CHECK(std::wstring(L"text") == str);
        }
    SECTION("Replace All Char Ptrs")
        {
        std::wstring str{ L"Here is some text to edit. Some more text" };
        string_util::replace_all(str, L"ext", 3, L"EXT");
        CHECK(std::wstring(L"Here is some tEXT to edit. Some more tEXT") == str);

        string_util::replace_all(str, L"EXT", 3, L"e x t");
        CHECK(std::wstring(L"Here is some te x t to edit. Some more te x t") == str);

        string_util::replace_all(str, L"e x t", 5, L"ext");
        CHECK(std::wstring(L"Here is some text to edit. Some more text") == str);
        }
    SECTION("Replace All Chars")
        {
        std::wstring str{ L"Here is some text to edit. Some more text" };
        string_util::replace_all(str, L'e', L'E');
        CHECK(std::wstring(L"HErE is somE tExt to Edit. SomE morE tExt") == str);
        }
    SECTION("Replace All Strings")
        {
        std::wstring str{ L"Here is some text to edit. Some more text" };
        string_util::replace_all(str, std::wstring(L"ext"), std::wstring(L"EXT"));
        CHECK(std::wstring(L"Here is some tEXT to edit. Some more tEXT") == str);
        }
    }

TEST_CASE("Hex Strings", "[stringutil][hex]")
	{
    SECTION("IsHex")
        {
        CHECK(string_util::is_hex_digit(L'0'));
        CHECK(string_util::is_hex_digit(L'1'));
        CHECK(string_util::is_hex_digit(L'2'));
        CHECK(string_util::is_hex_digit(L'3'));
        CHECK(string_util::is_hex_digit(L'4'));
        CHECK(string_util::is_hex_digit(L'5'));
        CHECK(string_util::is_hex_digit(L'6'));
        CHECK(string_util::is_hex_digit(L'7'));
        CHECK(string_util::is_hex_digit(L'8'));
        CHECK(string_util::is_hex_digit(L'9'));
        CHECK(string_util::is_hex_digit(L'a'));
        CHECK(string_util::is_hex_digit(L'b'));
        CHECK(string_util::is_hex_digit(L'c'));
        CHECK(string_util::is_hex_digit(L'd'));
        CHECK(string_util::is_hex_digit(L'e'));
        CHECK(string_util::is_hex_digit(L'f'));
        CHECK(string_util::is_hex_digit(L'A'));
        CHECK(string_util::is_hex_digit(L'B'));
        CHECK(string_util::is_hex_digit(L'C'));
        CHECK(string_util::is_hex_digit(L'D'));
        CHECK(string_util::is_hex_digit(L'E'));
        CHECK(string_util::is_hex_digit(L'F'));
        CHECK_FALSE(string_util::is_hex_digit(L'g'));
        CHECK_FALSE(string_util::is_hex_digit(L'.'));
        }
	}

TEST_CASE("Trim", "[stringutil][trim]")
    {
    SECTION("LeftTrim")
        {
        std::wstring str = L"  \n\t Text";
        ltrim(str);
        CHECK(str == L"Text");

        str = L"Text";
        ltrim(str);
        CHECK(str == L"Text");

        str = L"Text   ";
        ltrim(str);
        CHECK(str == L"Text   ");

        str = L"";
        ltrim(str);
        CHECK(str == L"");
        }

    SECTION("RightTrim")
        {
        std::wstring str = L"Text  \n\t ";
        rtrim(str);
        CHECK(str == L"Text");

        str = L"Text";
        rtrim(str);
        CHECK(str == L"Text");

        str = L"   Text";
        rtrim(str);
        CHECK(str == L"   Text");

        str = L"";
        rtrim(str);
        CHECK(str == L"");
        }

    SECTION("Trim")
        {
        std::wstring str = L"   Text  \n\t ";
        trim(str);
        CHECK(str == L"Text");

        str = L"Text";
        trim(str);
        CHECK(str == L"Text");

        str = L"   Text";
        trim(str);
        CHECK(str == L"Text");

        str = L"";
        trim(str);
        CHECK(str == L"");
        }

    SECTION("LeftTrimPunct")
        {
        std::wstring str = L"::;Text";
        ltrim_punct(str);
        CHECK(str == L"Text");

        str = L"Text";
        ltrim_punct(str);
        CHECK(str == L"Text");

        str = L"Text,,\\.";
        ltrim_punct(str);
        CHECK(str == L"Text,,\\.");

        str = L"";
        ltrim_punct(str);
        CHECK(str == L"");
        }

    SECTION("RightTrimPunct")
        {
        std::wstring str = L"Text,,\\.";
        rtrim_punct(str);
        CHECK(str == L"Text");

        str = L"Text";
        rtrim_punct(str);
        CHECK(str == L"Text");

        str = L"::{.Text";
        rtrim_punct(str);
        CHECK(str == L"::{.Text");

        str = L"";
        rtrim_punct(str);
        CHECK(str == L"");
        }

    SECTION("TrimPunct")
        {
        std::wstring str = L"::{.[]Text!@#$";
        trim_punct(str);
        CHECK(str == L"Text");

        str = L"Text";
        trim_punct(str);
        CHECK(str == L"Text");

        str = L"::{.Text";
        trim_punct(str);
        CHECK(str == L"Text");

        str = L"";
        trim_punct(str);
        CHECK(str == L"");

        str = L"::{.!@#$";
        trim_punct(str);
        CHECK(str == L"");
        }
    }

TEST_CASE("Natural Order Cmp", "[stringutil][natural order]")
    {
    SECTION("Nulls")
        {
        CHECK(string_util::strnatordcmp(L"", L"", true) == 0);
        CHECK(string_util::strnatordcmp<wchar_t>(nullptr, nullptr, true) == 0);
        CHECK(string_util::strnatordcmp<wchar_t>(nullptr, nullptr, false) == 0);
        CHECK(string_util::strnatordcmp<wchar_t>(nullptr, L"word", true) < 0);
        CHECK(string_util::strnatordcmp<wchar_t>(nullptr, L"word", false) < 0);
        CHECK(string_util::strnatordcmp<wchar_t>(L"word", nullptr, true) > 0);
        CHECK(string_util::strnatordcmp<wchar_t>(L"word", nullptr, false) > 0);
        }

    SECTION("SuperScript")
        {
        CHECK(string_util::strnatordcmp(L"Ca²⁺", L"Ca²⁺", true) == 0);
        CHECK(string_util::strnatordcmp(L"Ca²⁺", L"Ca³⁺", true) < 0);
        CHECK(string_util::strnatordcmp(L"Ca³⁺", L"Ca²⁺", true) > 0);
        }

    SECTION("CaseComparisons")
        {
        CHECK(string_util::strnatordcmp(L"some text", L"SoMe TEXt", true) == 0);
        CHECK(string_util::strnatordcmp(L"some text", L"SoMe TEXt", false) > 0);
        CHECK(string_util::strnatordcmp(L"SoMe TEXt", L"some text", false) < 0);
        }

    SECTION("NumberComparison")
        {
        CHECK(string_util::strnatordcmp(L"10000", L"79", true) > 0);
        CHECK(string_util::strnatordcmp(L"0010000", L"0082", true) > 0);
        CHECK(string_util::strnatordcmp(L"Word100", L"Word002", true) > 0);
        CHECK(string_util::strnatordcmp(L"889Text", L"99Text", true) > 0);
        CHECK(string_util::strnatordcmp(L"Text100MoreText", L"Text79MoreText", true) > 0);
        }
    SECTION("DifferentLengthComparison")
        {
        CHECK(string_util::strnatordcmp(L"SoMe TEXt", L"some", false) < 0);
        CHECK(string_util::strnatordcmp(L"Text100", L"Text00100", true) == 0);
        CHECK(string_util::strnatordcmp(L"Text100", L"Text00100moretext", true) < 0);
        }

    SECTION("Decimal")
        {
        CHECK(string_util::strnatordcmp(L"1.58", L"1.9", true) < 0);
        CHECK(string_util::strnatordcmp(L"1.9", L"1.4", true) > 0);
        CHECK(string_util::strnatordcmp(L"1.9", L"1.9", true) == 0);
        }

    SECTION("Thousands")
        {
        CHECK(string_util::strnatordcmp(L"7,200", L"8", true) > 0);
        CHECK(string_util::strnatordcmp(L"8", L"5,000,250", true) < 0);
        CHECK(string_util::strnatordcmp(L"8,780", L"8,001,870", true) < 0);
        }
    }

TEST_CASE("string_no_case_less", "[stringutil][string_no_case_less]")
    {
    std::set<std::wstring, string_no_case_less> strMap =
        {
        L"ERNIE",
        L"BERT",
        L"Ernie",
        L"Gordan",
        L"Maria",
        L"bert"
        };
    CHECK(strMap.size() == 4);
    CHECK(strMap.find(L"BeRt") != strMap.cend());
    CHECK(strMap.find(L"Ernie") != strMap.cend());
    CHECK(strMap.find(L"Oscar") == strMap.cend());
    CHECK(strMap.find(L"ERNI") == strMap.cend());
    CHECK(strMap.find(L"ERNIE'S") == strMap.cend());
    CHECK(strMap.find(L"") == strMap.cend());
    }

TEST_CASE("StrICmp", "[stringutil][StrICmp]")
    {
    SECTION("Nulls")
        {
        CHECK(string_util::stricmp("", "") == 0);
        CHECK(string_util::stricmp<char>(nullptr, nullptr) == 0);
        CHECK(string_util::stricmp<char>(nullptr, "a") < 0);
        CHECK(string_util::stricmp<char>("a", nullptr) > 0);
        }
    SECTION("CaseComparisons")
        {
        CHECK(string_util::stricmp("some text", "SoMe TEXt") == 0);
        CHECK(string_util::stricmp("some text", "SoMe TEXt") == 0);
        CHECK(string_util::stricmp("SoMe TEXt", "some text") == 0);
        }
    SECTION("DifferentLengthComparison")
        {
        CHECK(string_util::stricmp("SoMe TEXt", "some") > 0);
        CHECK(string_util::stricmp("Text100", "Text0") > 0);
        CHECK(string_util::stricmp("Text100", "Text00100moretext") > 0);
        CHECK(string_util::stricmp("Text000", "Text10000moretext") < 0);
        }
    }

TEST_CASE("StrNICmp", "[stringutil][StrNICmp]")
    {
    SECTION("Nulls")
        {
        CHECK(string_util::strnicmp("", "", 9) == 0);
        CHECK(string_util::strnicmp<char>(nullptr, nullptr, 9) == 0);
        CHECK(string_util::strnicmp<char>(nullptr, "a", 9) < 0);
        CHECK(string_util::strnicmp<char>("a", nullptr, 9) > 0);
        }
    SECTION("CaseComparisons")
        {
        CHECK(string_util::strnicmp("some text", "SoMe TEXt", 9) == 0);
        CHECK(string_util::strnicmp("some text", "SoMe TEXt", 9) == 0);
        CHECK(string_util::strnicmp("SoMe TEXt", "some text", 9) == 0);
        }
    SECTION("DifferentLengthComparison")
        {
        CHECK(string_util::strnicmp("SoMe TEXt", "some", 4) == 0);
        CHECK(string_util::strnicmp("Text100", "Text0", 4) == 0);
        CHECK(string_util::strnicmp("Text100", "Text00100moretext", 5) > 0);
        CHECK(string_util::strnicmp("Text000", "Text10000moretext", 5) < 0);
        }
    }

TEST_CASE("HasSuffix", "[stringutil][HasSuffix]")
    {
    SECTION("Nulls")
        {
        CHECK(string_util::has_suffix<wchar_t>(nullptr, 5, L"es", 2) == false);
        CHECK(string_util::has_suffix<wchar_t>(L"hunted", 6, nullptr, 2) == false);
        CHECK(string_util::has_suffix<wchar_t>(L"e", 1, L"es", 2) == false);
        }
    SECTION("HasSuffix")
        {
        CHECK(string_util::has_suffix<wchar_t>(L"hunted", 6, L"ed", 2));
        CHECK(string_util::has_suffix<wchar_t>(L"ted", 3, L"ed", 2));
        }
    SECTION("NotHasSuffix")
        {
        CHECK(string_util::has_suffix<wchar_t>(L"hunted", 6, L"es", 2) == false);
        // text must be longer than suffix for it to have the suffix
        CHECK(string_util::has_suffix<wchar_t>(L"ed", 2, L"ed", 2) == false);
        }
    }

TEST_CASE("StrCSpnPointer", "[stringutil][StrCSpnPointer]")
    {
    SECTION("TestNulls")
        {
        CHECK(string_util::strcspn_pointer<char>(nullptr, "hello", 5) == nullptr);
        CHECK(string_util::strcspn_pointer<char>("HelLo", nullptr, 0) == nullptr);
        CHECK(string_util::strcspn_pointer<char>(nullptr, nullptr, 0) == nullptr);
        }
    SECTION("FindFirst")
        {
        const char* buffer = "<blah blah>";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer);
        buffer = ">blah blah>";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer);
        }
    SECTION("FindMiddle")
        {
        const char* buffer = "blah <blah";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer+5);
        buffer = "blah >blah";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer+5);
        }
    SECTION("FindLast")
        {
        const char* buffer = "blah blah<";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer+9);
        buffer = "blah blah>";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer+9);
        }
    SECTION("NotFind")
        {
        const char* buffer = "blah blah";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == nullptr);
        buffer = "blah blah";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == nullptr);
        CHECK(string_util::strcspn_pointer<char>("", "<>", 2) == nullptr);
        }
    }

TEST_CASE("Find Matching Tag", "[stringutil][find_matching_close_tag]")
    {
    SECTION("Closing With Open Tags Strings")
        {
        const wchar_t* buffer = L"[[img [[]]]hello]]], world";
        CHECK(string_util::find_matching_close_tag(buffer+1, L"[[", L"]]]") == buffer+16);
        }
    SECTION("Closing With Open Tags Strings2")
        {
        const wchar_t* buffer = L"[[img [[]]]h[[e]]]llo]]], world";
        CHECK(string_util::find_matching_close_tag(buffer+1, L"[[", L"]]]") == buffer+21);
        }
    SECTION("Closing With Open Tags Strings Start With Same Char")
        {
        const wchar_t* buffer = L"[[img [[[]]hello[]], world";
        CHECK(string_util::find_matching_close_tag(buffer+1, L"[[", L"[]]") == buffer+16);
        }
    SECTION("Test Nulls Strings")
        {
        CHECK(string_util::find_matching_close_tag(L"", L"[[", L"[]]") == nullptr);
        CHECK(string_util::find_matching_close_tag(L"text", L"", L"[]]") == nullptr);
        CHECK(string_util::find_matching_close_tag(L"text", L"[]]", L"") == nullptr);
        CHECK(string_util::find_matching_close_tag(L"[[img [[[]]hello[]], world", L"", L"[]]") == nullptr);
        CHECK(string_util::find_matching_close_tag(L"[[img [[[]]hello[]], world", L"[[", L"") == nullptr);
        }
    SECTION("No Closing Tags Strings")
        {
        const wchar_t* buffer = L"[[img hello, world";
        CHECK(string_util::find_matching_close_tag(buffer+1, L"[[", L"[]]") == nullptr);
        }
    SECTION("Closing With Trailing Open Tag Strings")
        {
        const wchar_t* buffer = L"[[img [[ihello[]], world";
        CHECK(string_util::find_matching_close_tag(buffer+1, L"[[", L"[]]") == nullptr);
        }
    // single characters
    SECTION(" Nulls")
        {
        CHECK(string_util::find_matching_close_tag(nullptr, L'<', L'>', false) == nullptr);
        }
    SECTION("No Closing Tags")
        {
        const wchar_t* buffer = L"<img hello, world";
        CHECK(string_util::find_matching_close_tag(buffer+1, L'<', L'>', false) == nullptr);
        }
    SECTION("Closing Tags")
        {
        const wchar_t* buffer = L"<img hello>, world";
        CHECK(string_util::find_matching_close_tag(buffer+1, L'<', L'>', false) == buffer+10);
        }
    SECTION("Closing With Open Tags")
        {
        const wchar_t* buffer = L"<img <i>hello</i>>, world";
        CHECK(string_util::find_matching_close_tag(buffer+1, L'<', L'>', false) == buffer+17);
        }
    SECTION("Closing With Trailing Open Tag")
        {
        const wchar_t* buffer = L"<img <ihello>, world";
        CHECK(string_util::find_matching_close_tag(buffer+1, L'<', L'>', false) == nullptr);
        }
    }

TEST_CASE("Find Matching Tag Unescaped", "[stringutil][find_unescaped_matching_close_tag]")
    {
    SECTION("No Closing Tags Strings")
        {
        const wchar_t* buffer = L"[img hello, world";
        CHECK(string_util::find_unescaped_matching_close_tag<wchar_t>(buffer+1, L'[', L']') == nullptr);
        }
    SECTION("Closing With Trailing Open Tag Strings")
        {
        const wchar_t* buffer = L"[img [ihello[], world";
        CHECK(string_util::find_unescaped_matching_close_tag<wchar_t>(buffer+1, L'[', L']') == nullptr);
        }
    SECTION("Nulls")
        {
        CHECK(string_util::find_unescaped_matching_close_tag<wchar_t>(nullptr, L'<', L'>') == nullptr);
        }
    SECTION("No Closing Tags")
        {
        const wchar_t* buffer = L"<img hello, world";
        CHECK(string_util::find_unescaped_matching_close_tag<wchar_t>(buffer+1, L'<', L'>') == nullptr);
        }
    SECTION("No Closing Tags Escaped")
        {
        const wchar_t* buffer = LR"(<img hello, world\>)";
        CHECK(string_util::find_unescaped_matching_close_tag<wchar_t>(buffer+1, L'<', L'>') == nullptr);
        }
    SECTION("Closing Tags")
        {
        const wchar_t* buffer = L"<\n\nimg hello>, world";
        CHECK(string_util::find_unescaped_matching_close_tag<wchar_t>(buffer+1, L'<', L'>') == buffer+12);
        }
    SECTION("Closing Tags Escaped")
        {
        const wchar_t* buffer = LR"(<img \>hello>, world)";
        CHECK(string_util::find_unescaped_matching_close_tag<wchar_t>(buffer+1, L'<', L'>') == buffer+12);
        }
    SECTION("Closing With Open Tags")
        {
        const wchar_t* buffer = L"<img \n<i>hello</i>>, world";
        CHECK(string_util::find_unescaped_matching_close_tag<wchar_t>(buffer+1, L'<', L'>') == buffer+18);
        }
    SECTION("Closing With Open Tags Escaped")
        {
        const wchar_t* buffer = LR"(<img \<<i>hello</i>>, world)";
        CHECK(string_util::find_unescaped_matching_close_tag<wchar_t>(buffer+1, L'<', L'>') == buffer+19);
        }
    SECTION("Closing With Trailing Open Tag")
        {
        const wchar_t* buffer = L"<img <ihello>, world";
        CHECK(string_util::find_unescaped_matching_close_tag<wchar_t>(buffer+1, L'<', L'>') == nullptr);
        }
    }

TEST_CASE("Find Matching Tag Unescaped", "[stringutil][find_unescaped_matching_close_tag_same_line]")
    {
    SECTION("No Closing Tags Strings")
        {
        const wchar_t* buffer = L"[img hello, world";
        CHECK(string_util::find_unescaped_matching_close_tag_same_line<wchar_t>(buffer+1, L'[', L']') == nullptr);
        }
    SECTION("Closing With Trailing Open Tag Strings")
        {
        const wchar_t* buffer = L"[img [ihello[], world";
        CHECK(string_util::find_unescaped_matching_close_tag_same_line<wchar_t>(buffer+1, L'[', L']') == nullptr);
        }
    SECTION("Nulls")
        {
        CHECK(string_util::find_unescaped_matching_close_tag_same_line<wchar_t>(nullptr, L'<', L'>') == nullptr);
        }
    SECTION("No Closing Tags")
        {
        const wchar_t* buffer = L"<img hello, world";
        CHECK(string_util::find_unescaped_matching_close_tag_same_line<wchar_t>(buffer+1, L'<', L'>') == nullptr);
        }
    SECTION("No Closing Tags Escaped")
        {
        const wchar_t* buffer = LR"(<img hello, world\>)";
        CHECK(string_util::find_unescaped_matching_close_tag_same_line<wchar_t>(buffer+1, L'<', L'>') == nullptr);
        }
    SECTION("Closing Tags")
        {
        const wchar_t* buffer = L"<\n\nimg hello>, world";
        CHECK(string_util::find_unescaped_matching_close_tag_same_line<wchar_t>(buffer+1, L'<', L'>') == nullptr);
        }
    SECTION("Closing Tags Escaped")
        {
        const wchar_t* buffer = LR"(<img \>hello>, world)";
        CHECK(string_util::find_unescaped_matching_close_tag_same_line<wchar_t>(buffer+1, L'<', L'>') == buffer+12);
        }
    SECTION("Closing With Open Tags")
        {
        const wchar_t* buffer = L"<img \n<i>hello</i>>, world";
        CHECK(string_util::find_unescaped_matching_close_tag_same_line<wchar_t>(buffer+1, L'<', L'>') == nullptr);
        }
    SECTION("Closing With Open Tags Escaped")
        {
        const wchar_t* buffer = LR"(<img \<<i>hello</i>>, world)";
        CHECK(string_util::find_unescaped_matching_close_tag_same_line<wchar_t>(buffer+1, L'<', L'>') == buffer+19);
        }
    SECTION("Closing With Trailing Open Tag")
        {
        const wchar_t* buffer = L"<img <ihello>, world";
        CHECK(string_util::find_unescaped_matching_close_tag_same_line<wchar_t>(buffer+1, L'<', L'>') == nullptr);
        }
    }

TEST_CASE("Find Matching Tag Unescaped", "[stringutil][find_unescaped_matching_close_tag_same_line_n]")
    {
    SECTION("Full scan")
        {
        SECTION("No Closing Tags Strings")
            {
            const wchar_t* buffer = L"[img hello, world";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'[', L']', std::wcslen(buffer)) == nullptr);
            }
        SECTION("Closing With Trailing Open Tag Strings")
            {
            const wchar_t* buffer = L"[img [ihello[], world";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'[', L']', std::wcslen(buffer)) == nullptr);
            }
        SECTION("Nulls")
            {
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(nullptr, L'<', L'>', 0) == nullptr);
            }
        SECTION("No Closing Tags")
            {
            const wchar_t* buffer = L"<img hello, world";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'<', L'>', std::wcslen(buffer)) == nullptr);
            }
        SECTION("No Closing Tags Escaped")
            {
            const wchar_t* buffer = LR"(<img hello, world\>)";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'<', L'>', std::wcslen(buffer)) == nullptr);
            }
        SECTION("Closing Tags")
            {
            const wchar_t* buffer = L"<\n\nimg hello>, world";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'<', L'>', std::wcslen(buffer)) == nullptr);
            }
        SECTION("Closing Tags Escaped")
            {
            const wchar_t* buffer = LR"(<img \>hello>, world)";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'<', L'>', std::wcslen(buffer)) == buffer+12);
            }
        SECTION("Closing With Open Tags")
            {
            const wchar_t* buffer = L"<img \n<i>hello</i>>, world";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'<', L'>', std::wcslen(buffer)) == nullptr);
            }
        SECTION("Closing With Open Tags Escaped")
            {
            const wchar_t* buffer = LR"(<img \<<i>hello</i>>, world)";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'<', L'>', std::wcslen(buffer)) == buffer+19);
            }
        SECTION("Closing With Trailing Open Tag")
            {
            const wchar_t* buffer = L"<img <ihello>, world";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'<', L'>', std::wcslen(buffer)) == nullptr);
            }
        }
    SECTION("Partial scan")
        {
        SECTION("Closing Tags Escaped")
            {
            const wchar_t* buffer = LR"(<img \>hello>, world)";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'<', L'>', 4) == nullptr);
            buffer = LR"(<img \>hello>, world)";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer + 1, L'<', L'>', 0) == nullptr);
            }
        SECTION("Closing With Open Tags Escaped")
            {
            const wchar_t* buffer = LR"(<img \<<i>hello</i>>, world)";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'<', L'>', 4) == nullptr);
            }
        SECTION("Too far")
            {
            const wchar_t* buffer = LR"(<img \<<i>hello</i>>, world)";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer+1, L'<', L'>', 89) == buffer + 19);

            buffer = LR"(<img \<<i>hello</i>>, world)";
            CHECK(string_util::find_unescaped_matching_close_tag_same_line_n(buffer + 1, L'<', L'>', 89) == buffer + 19);
            }
        }
    }

TEST_CASE("RemoveSpaces", "[stringutil][RemoveSpaces]")
    {
    SECTION("RemoveBlankLinesEmpty")
        {
        std::wstring text(L"");
        CHECK(string_util::remove_blank_lines(text) == 0);
        CHECK(text == L"");
        }
    SECTION("RemoveBlankLines")
        {
        std::wstring text(L"Blah\n\nLine2");
        CHECK(string_util::remove_blank_lines(text) == 1);
        CHECK(text == L"Blah\nLine2");
        }
    SECTION("RemoveBlankLines2")
        {
        std::wstring text(L"Blah\n\nLine2\n\n");
        CHECK(string_util::remove_blank_lines(text) == 2);
        CHECK(text == L"Blah\nLine2\n");
        }
    SECTION("RemoveBlankLines3")
        {
        std::wstring text(L"Blah\r\n\r\nLine2\n\n");
        CHECK(string_util::remove_blank_lines(text) == 3);
        CHECK(text == L"Blah\r\nLine2\n");
        }
    SECTION("RemoveSpacesEmpty")
        {
        std::wstring text(L"");
        CHECK(string_util::remove_extra_spaces(text) == 0);
        CHECK(text == L"");
        }
    SECTION("RemoveNoSpaces")
        {
        std::wstring text(L"hellothereworld!newline");
        CHECK(string_util::remove_extra_spaces(text) == 0);
        CHECK(text == L"hellothereworld!newline");
        }
    SECTION("RemoveSpaces")
        {
        std::wstring text(L"hello  there    \t \r\n world !\r\nnew line");
        CHECK(string_util::remove_extra_spaces(text) == 9);
        CHECK(text == L"hello there world !\r\nnew line");
        }
    SECTION("RemoveSpacesNoExtraSpaces")
        {
        std::wstring text(L"hello there\tworld!\r\nnew line");
        CHECK(string_util::remove_extra_spaces(text) == 0);
        CHECK(text == L"hello there\tworld!\r\nnew line");
        }
    };

TEST_CASE("StrIStr", "[stringutil][StrIStr]")
    {
    SECTION("TestNulls")
        {
        CHECK(string_util::stristr<char>(nullptr, "HelLo") == nullptr);
        CHECK(string_util::stristr<char>("HelLo", nullptr) == nullptr);
        }
    SECTION("FindFirstItem")
        {
        char buffer[] = "hello, world";
        // should find at the beginning
        CHECK(string_util::stristr(buffer, "HelLo") == buffer);
        }
    SECTION("LastItemInSequenceString")
        {
        char buffer[] = "hello, world";
        // should find last item in sequence
        CHECK(string_util::stristr(buffer, "WORLD") == buffer+7);
        }
    SECTION("MiddleItemInSequenceString")
        {
        char buffer[] = "hello, world!!! Goodbye, cruel world!";
        // should find last item in sequence
        CHECK(string_util::stristr(buffer, "WORLD") == buffer+7);
        }
    SECTION("FindNothing")
        {
        char buffer[] = "hello, world";
        // should find nothing and return nullptr
        CHECK(string_util::stristr(buffer, "Help") == nullptr);
        }
    SECTION("FindNothingEmptySearchString")
        {
        char buffer[] = "hello, world";
        // should find nothing and return nullptr
        CHECK(string_util::stristr(buffer, "") == nullptr);
        }
    SECTION("EmptyString")
        {
        // should find nothing and return nullptr
        CHECK(string_util::stristr("", "Hello") == nullptr);
        }
    SECTION("SubStringTooBig")
        {
        CHECK(string_util::stristr("Hello", "Hello World") == nullptr);
        CHECK(string_util::stristr("Hello", "StringLongerThanMainString") == nullptr);
        }
    };

TEST_CASE("StrNChr", "[stringutil][StrNChr]")
    {
    SECTION("Null")
        {
        CHECK(string_util::strnchr<char>(nullptr, ',', 5) == nullptr);
        }
    SECTION("NotSearchFarEnough")
        {
        char buffer[] = "hello, world";
        // should only search "hello" and not find ,
        CHECK(string_util::strnchr(buffer, ',', 5) == nullptr);
        }
    SECTION("FindFirstItem")
        {
        char buffer[] = "hello, world";
        // should find first ,
        CHECK(string_util::strnchr(buffer, ',', 6) == buffer+5);
        }
    SECTION("LastItemInSequenceString")
        {
        char buffer[] = "hello, world";
        // should find last item in sequence
        CHECK(string_util::strnchr(buffer, 'd', 12) == buffer+11);
        }
    SECTION("Find nothing")
        {
        char buffer[] = "hello, world";
        // should find nothing and return size argument
        CHECK(string_util::strnchr(buffer, 'z', 12) == nullptr);
        }
    SECTION("Empty string")
        {
        // should find nothing and nullptr
        CHECK(string_util::strnchr("", 'z', 0) == nullptr);
        }
    SECTION("Boundary error")
        {
        wchar_t text[10];
        std::wmemset(text, 0, 10);
        text[0] = L'a';
        text[1] = L'b';
        text[2] = L'c';
        // will really only scan first three letter, see the embedded NULLs, and return
        // what the caller thought was the length of the string to know that it failed.
        CHECK(string_util::strnchr<wchar_t>(text, L'x', 100) == nullptr);
        }
    };

TEST_CASE("StrtodEx", "[stringutil][StrtodEx]")
    {
    SECTION("Null")
        {
        wchar_t* end;
        CHECK(string_util::strtod_ex<wchar_t>(nullptr, &end) == 0);
        CHECK(end == nullptr);
        }
    // make sure your system's region is set to English or won't work
    // (this test case assumes US English decimal)
    SECTION("Double")
        {
        wchar_t* end;
        const wchar_t* value = L"5.27";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 5.27);
        // end should point to the value's null terminator
        CHECK(end == (value+4));
        CHECK(*end == 0);
        }
    SECTION("EndingHyphen")
        {
        wchar_t* end;
        const wchar_t* value = L"5.27-";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 5.27);
        // end should point to the dash at the end
        CHECK(end == (value+4));
        CHECK(*end == L'-');
        }
    SECTION("Hyphen")
        {
        wchar_t* end;
        const wchar_t* value = L"5.5-6";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 5.75);
        // end should point to the value's null terminator
        CHECK(end == (value+5));
        CHECK(*end == 0);
        }
    SECTION("Colon")
        {
        wchar_t* end;
        const wchar_t* value = L"5.5:6";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 5.75);
        // end should point to the value's null terminator
        CHECK(end == (value+5));
        CHECK(*end == 0);
        }
    SECTION("NonDoubles")
        {
        wchar_t* end;
        const wchar_t* value = L"5:8";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 6.5);
        // end should point to the value's null terminator
        CHECK(end == (value+3));
        CHECK(*end == 0);
        }
    SECTION("Zeros")
        {
        wchar_t* end;
        const wchar_t* value = L"0:0.0";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 0);
        // end should point to the value's null terminator
        CHECK(end == (value+5));
        CHECK(*end == 0);
        }
    };

TEST_CASE("RemoveAllWhitespace", "[stringutil][RemoveAllWhitespace]")
    {
    SECTION("HasWhitespaces")
        {
        std::wstring theWord = L"\nWords\rMore\n\rEnd.\n";
        theWord = string_util::remove_all_whitespace<std::wstring>(theWord);
        CHECK(theWord == L"WordsMoreEnd.");
        }
    SECTION("AllWhitespaces")
        {
        std::wstring theWord = L"\n\r\n\r\n";
        theWord = string_util::remove_all_whitespace<std::wstring>(theWord);
        CHECK(theWord == L"");
        }
    SECTION("HasNoWhitespaces")
        {
        std::wstring theWord = L"WordsMoreEnd.";
        theWord = string_util::remove_all_whitespace<std::wstring>(theWord);
        CHECK(theWord == L"WordsMoreEnd.");
        }
    };

TEST_CASE("Strnlen", "[stringutil][Strnlen]")
    {
    SECTION("Nulls")
        {
        CHECK(string_util::strnlen<wchar_t>(nullptr, 5) == 0);
        }
    SECTION("Normal")
        {
        CHECK(string_util::strnlen(L"hello", 5) == 5);
        CHECK(string_util::strnlen(L"longer string here. ", 20) == 20);
        }
    SECTION("NotScanningWholeText")
        {
        CHECK(string_util::strnlen(L"hello", 3) == 3);
        CHECK(string_util::strnlen(L"longer string here. ", 15) == 15);
        }
    SECTION("MaxValueTooBig")
        {
        CHECK(string_util::strnlen(L"hello", 10) == 5);
        CHECK(string_util::strnlen(L"longer string here. ", 999) == 20);
        }
    SECTION("BadString")
        {
        const wchar_t text[5] = { L'h', L'e', L'l', L'l', L'o' }; // no nullptr terminator
        CHECK(string_util::strnlen(text, 5) == 5);
        }
    };

TEST_CASE("StrNIStr", "[stringutil][StrNIStr]")
    {
    SECTION("NotSearchFarEnough")
        {
        char buffer[] = "hello, world";
        // should only search "hell" and not find "hello"
        CHECK(string_util::strnistr(buffer, "HeLlO", 4) == nullptr);
        }
    SECTION("FindItemBufferAndSearchAreTheSame")
        {
        char buffer[] = "hello";
        // should find first ,
        CHECK(string_util::strnistr(buffer, "HelLo", std::strlen(buffer)) == buffer);
        }
    SECTION("FindFirstItem")
        {
        char buffer[] = "hello, world";
        // should find first ,
        CHECK(string_util::strnistr(buffer, "HelLo", 5) == buffer);
        }
    SECTION("FindItemInMiddle")
        {
        char buffer[] = "hello, there world";
        // should find first ,
        CHECK(string_util::strnistr(buffer, "THErE", std::strlen(buffer)) == buffer+7);
        }
    SECTION("LastItemInSequenceString")
        {
        char buffer[] = "hello, world";
        // should find last item in sequence
        CHECK(string_util::strnistr(buffer, "WORLD", std::strlen(buffer) ) == buffer+7);
        }
    SECTION("LastItemInSequenceStringCharacter")
        {
        char buffer[] = "hello, world";
        //should find last item in sequence
        CHECK(string_util::strnistr(buffer, "d", std::strlen(buffer) ) == buffer+11);
        }
    SECTION("FindNothing")
        {
        char buffer[] = "hello, world";
        // should find nothing and return nullptr
        CHECK(string_util::strnistr(buffer, "Help", std::strlen(buffer)) == nullptr);
        }
    SECTION("FindNothingPartialMatchAtEnd")
        {
        char buffer[] = "hello, world";
        // should find nothing and return nullptr
        CHECK(string_util::strnistr(buffer, "WORLDs", std::strlen(buffer)) == nullptr);
        }
    SECTION("FindNothingNonTerminatedBuffer")
        {
        char buffer[] = { 'h', 'e', 'l', 'l', 'o' };
        // should find nothing and return nullptr
        CHECK(string_util::strnistr(buffer, "hello there", 5) == nullptr);
        }
    SECTION("FindNothingEmptyString")
        {
        char buffer[] = "hello, world";
        // should find nothing and return nullptr
        CHECK(string_util::strnistr(buffer, "", std::strlen(buffer)) == nullptr);
        }
    SECTION("EmptyString")
        {
        // should find nothing and nullptr
        CHECK(string_util::strnistr("", "Hello", 0) == nullptr);
        }
    SECTION("BoundaryError")
        {
        // pass in buffersize that is incorrect
        CHECK(string_util::strnistr("", "Hello", 5) == nullptr);
        }
    SECTION("BoundaryError2")
        {
        // pass in buffersize that is incorrect
        CHECK(string_util::strnistr("Hell", "Hello", 5) == nullptr);
        }
    SECTION("SubStringTooBig")
        {
        CHECK(string_util::strnistr("Hello", "Hello World", 11) == nullptr);
        }
    };

TEST_CASE("wcstod_thousands_separator", "[stringutil][wcstod_thousands_separator]")
    {
    setlocale(LC_NUMERIC, "");

    SECTION("Null")
        {
        CHECK(0 == wcstod_thousands_separator(nullptr, nullptr));
        }

    SECTION("Skip Spaces")
        {
        setlocale(LC_NUMERIC, "de-DE");
	    const wchar_t* buffer = L"    8.080.287.890,47 ml";
	    wchar_t* end;
	    CHECK_THAT(8080287890.47, WithinRel(wcstod_thousands_separator(buffer, &end), 1e-4));
        CHECK(0 == wcsncmp(end, L" ml", 3));
        }

    SECTION("Plus")
        {
        setlocale(LC_NUMERIC, "en-US");
	    const wchar_t* buffer = L"+8,080,287,890.47 ml";
	    wchar_t* end;
	    CHECK_THAT(8080287890.47, WithinRel(wcstod_thousands_separator(buffer, &end), 1e-4));
        CHECK(0 == wcsncmp(end, L" ml", 3));
        }

    SECTION("Minus")
        {
        setlocale(LC_NUMERIC, "en-US");
	    const wchar_t* buffer = L"-8,080,287,890.47 ml";
	    wchar_t* end;
	    CHECK_THAT(-8080287890.47, WithinRel(wcstod_thousands_separator(buffer, &end), 1e-4));
        CHECK(0 == wcsncmp(end, L" ml", 3));
        }

    SECTION("Long Number")
        {
        setlocale(LC_NUMERIC, "en-US");
	    const wchar_t* buffer =
            L"-8,080,287,890.457425548545785245742554854578524574255485457852457425548545785245742554854578524574255485457852";
	    wchar_t* end;
	    CHECK_THAT(-8080287890.45743, WithinRel(wcstod_thousands_separator(buffer, &end), 1e-4));
        // some of the ridiculously long number string will not be read and converted
        CHECK(0 == wcsncmp(end, L"425548545785245742554854578524574255485457852", 45));
        }

    SECTION("Short Number")
        {
        setlocale(LC_NUMERIC, "en-US");
	    const wchar_t* buffer = L"-8ml";
	    wchar_t* end;
	    CHECK_THAT(-8, WithinRel(wcstod_thousands_separator(buffer, &end), 1e-4));
        CHECK(0 == wcsncmp(end, L"ml", 2));
        }

    SECTION("Not A Number")
        {
        setlocale(LC_NUMERIC, "en-US");
	    const wchar_t* buffer = L",ml";
	    wchar_t* end;
	    CHECK_THAT(0, WithinRel(wcstod_thousands_separator(buffer, &end), 1e-4));
        CHECK(0 == wcsncmp(end, L",ml", 3));
        }

    SECTION("German")
        {
        setlocale(LC_NUMERIC, "de-DE");
	    const wchar_t* buffer = L"8.080.287.890,47 ml";
	    wchar_t* end;
	    CHECK_THAT(8080287890.47, WithinRel(wcstod_thousands_separator(buffer, &end), 1e-4));
        CHECK(0 == wcsncmp(end, L" ml", 3));
        }

    SECTION("English")
        {
        setlocale(LC_NUMERIC, "en-US");
	    const wchar_t* buffer = L"8,080,287,890.47 ml";
	    wchar_t* end;
	    CHECK_THAT(8080287890.47, WithinRel(wcstod_thousands_separator(buffer, &end), 1e-4));
        CHECK(0 == wcsncmp(end, L" ml", 3));
        }

    SECTION("Ignore End")
        {
        setlocale(LC_NUMERIC, "en-US");
	    const wchar_t* buffer = L"8,080,287,890.47 ml";
	    CHECK_THAT(8080287890.47, WithinRel(wcstod_thousands_separator(buffer, nullptr), 1e-4));
        }

    setlocale(LC_NUMERIC, "");
    }

TEST_CASE("Superscript/Subscrpt", "[stringutil][superscript][subscript]")
    {
    SECTION("ToSuperscript")
        {
        CHECK(L'⁰' == string_util::to_superscript(L'0'));
        CHECK(L'¹' == string_util::to_superscript(L'1'));
        CHECK(L'²' == string_util::to_superscript(L'2'));
        CHECK(L'³' == string_util::to_superscript(L'3'));
        CHECK(L'⁴' == string_util::to_superscript(L'4'));
        CHECK(L'⁵' == string_util::to_superscript(L'5'));
        CHECK(L'⁶' == string_util::to_superscript(L'6'));
        CHECK(L'⁷' == string_util::to_superscript(L'7'));
        CHECK(L'⁸' == string_util::to_superscript(L'8'));
        CHECK(L'⁹' == string_util::to_superscript(L'9'));
        CHECK(L'⁺' == string_util::to_superscript(L'+'));
        CHECK(L'⁻' == string_util::to_superscript(L'-'));
        CHECK(L'⁼' == string_util::to_superscript(L'='));
        CHECK(L'⁽' == string_util::to_superscript(L'('));
        CHECK(L'⁾' == string_util::to_superscript(L')'));
        CHECK(L'ᵃ' == string_util::to_superscript(L'a'));
        CHECK(L'ᵇ' == string_util::to_superscript(L'b'));
        CHECK(L'ᶜ' == string_util::to_superscript(L'c'));
        CHECK(L'ᵈ' == string_util::to_superscript(L'd'));
        CHECK(L'ᵉ' == string_util::to_superscript(L'e'));
        CHECK(L'ᶠ' == string_util::to_superscript(L'f'));
        CHECK(L'ᵍ' == string_util::to_superscript(L'g'));
        CHECK(L'ʰ' == string_util::to_superscript(L'h'));
        CHECK(L'ʲ' == string_util::to_superscript(L'j'));
        CHECK(L'ᵏ' == string_util::to_superscript(L'k'));
        CHECK(L'ˡ' == string_util::to_superscript(L'l'));
        CHECK(L'ᵐ' == string_util::to_superscript(L'm'));
        CHECK(L'ⁿ' == string_util::to_superscript(L'n'));
        CHECK(L'ᵒ' == string_util::to_superscript(L'o'));
        CHECK(L'ᵖ' == string_util::to_superscript(L'p'));
        CHECK(L'ʳ' == string_util::to_superscript(L'r'));
        CHECK(L'ˢ' == string_util::to_superscript(L's'));
        CHECK(L'ᵗ' == string_util::to_superscript(L't'));
        CHECK(L'ᵘ' == string_util::to_superscript(L'u'));
        CHECK(L'ᵛ' == string_util::to_superscript(L'v'));
        CHECK(L'ʷ' == string_util::to_superscript(L'w'));
        CHECK(L'ʸ' == string_util::to_superscript(L'y'));
        CHECK(L'ˣ' == string_util::to_superscript(L'x'));
        CHECK(L'ᶻ' == string_util::to_superscript(L'z'));
        CHECK(L'ⁱ' == string_util::to_superscript(L'i'));
        CHECK(L'ⁿ' == string_util::to_superscript(L'n'));
        CHECK(L'⁰' == string_util::to_superscript(L'０'));
        CHECK(L'¹' == string_util::to_superscript(L'１'));
        CHECK(L'²' == string_util::to_superscript(L'２'));
        CHECK(L'³' == string_util::to_superscript(L'３'));
        CHECK(L'⁴' == string_util::to_superscript(L'４'));
        CHECK(L'⁵' == string_util::to_superscript(L'５'));
        CHECK(L'⁶' == string_util::to_superscript(L'６'));
        CHECK(L'⁷' == string_util::to_superscript(L'７'));
        CHECK(L'⁸' == string_util::to_superscript(L'８'));
        CHECK(L'⁹' == string_util::to_superscript(L'９'));
        CHECK(L'*' == string_util::to_superscript(L'*'));                            

        CHECK(string_util::is_superscript_number(L'⁰'));
        CHECK(string_util::is_superscript_number(L'¹'));
        CHECK(string_util::is_superscript_number(L'²'));
        CHECK(string_util::is_superscript_number(L'³'));
        CHECK(string_util::is_superscript_number(L'⁴'));
        CHECK(string_util::is_superscript_number(L'⁵'));
        CHECK(string_util::is_superscript_number(L'⁶'));
        CHECK(string_util::is_superscript_number(L'⁷'));
        CHECK(string_util::is_superscript_number(L'⁸'));
        CHECK(string_util::is_superscript_number(L'⁹'));
        CHECK(!string_util::is_superscript_number(L'2'));
        CHECK(!string_util::is_superscript_number(L'₀'));
        CHECK(!string_util::is_superscript_number(L'a'));
        CHECK(!string_util::is_superscript_number(L'⁺'));
        CHECK(!string_util::is_superscript_number(L'ⁿ'));
        // Roman numerals
        CHECK(string_util::is_superscript_number(L'ᶜ'));
        CHECK(string_util::is_superscript_number(L'ᵈ'));
        CHECK(string_util::is_superscript_number(L'ⁱ'));
        CHECK(string_util::is_superscript_number(L'ᵐ'));
        CHECK(string_util::is_superscript_number(L'ᵛ'));
        CHECK(string_util::is_superscript_number(L'ˣ'));
        }

    SECTION("ToSubscript")
        {
        CHECK(L'₀' == string_util::to_subscript(L'0'));
        CHECK(L'₁' == string_util::to_subscript(L'1'));
        CHECK(L'₂' == string_util::to_subscript(L'2'));
        CHECK(L'₃' == string_util::to_subscript(L'3'));
        CHECK(L'₄' == string_util::to_subscript(L'4'));
        CHECK(L'₅' == string_util::to_subscript(L'5'));
        CHECK(L'₆' == string_util::to_subscript(L'6'));
        CHECK(L'₇' == string_util::to_subscript(L'7'));
        CHECK(L'₈' == string_util::to_subscript(L'8'));
        CHECK(L'₉' == string_util::to_subscript(L'9'));
        CHECK(L'₊' == string_util::to_subscript(L'+'));
        CHECK(L'₋' == string_util::to_subscript(L'-'));
        CHECK(L'₌' == string_util::to_subscript(L'='));
        CHECK(L'₍' == string_util::to_subscript(L'('));
        CHECK(L'₎' == string_util::to_subscript(L')'));
        CHECK(L'ₐ' == string_util::to_subscript(L'a'));
        CHECK(L'ₑ' == string_util::to_subscript(L'e'));
        CHECK(L'ₒ' == string_util::to_subscript(L'o'));
        CHECK(L'ₕ' == string_util::to_subscript(L'h'));
        CHECK(L'ₖ' == string_util::to_subscript(L'k'));
        CHECK(L'ₗ' == string_util::to_subscript(L'l'));
        CHECK(L'ₘ' == string_util::to_subscript(L'm'));
        CHECK(L'ₙ' == string_util::to_subscript(L'n'));
        CHECK(L'ₚ' == string_util::to_subscript(L'p'));
        CHECK(L'ₛ' == string_util::to_subscript(L's'));
        CHECK(L'ₜ' == string_util::to_subscript(L't'));
        CHECK(L'₀' == string_util::to_subscript(L'０'));
        CHECK(L'₁' == string_util::to_subscript(L'１'));
        CHECK(L'₂' == string_util::to_subscript(L'２'));
        CHECK(L'₃' == string_util::to_subscript(L'３'));
        CHECK(L'₄' == string_util::to_subscript(L'４'));
        CHECK(L'₅' == string_util::to_subscript(L'５'));
        CHECK(L'₆' == string_util::to_subscript(L'６'));
        CHECK(L'₇' == string_util::to_subscript(L'７'));
        CHECK(L'₈' == string_util::to_subscript(L'８'));
        CHECK(L'₉' == string_util::to_subscript(L'９'));
        CHECK(L'z' == string_util::to_subscript(L'z'));

        CHECK(string_util::is_subscript_number(L'₀'));
        CHECK(string_util::is_subscript_number(L'₁'));
        CHECK(string_util::is_subscript_number(L'₂'));
        CHECK(string_util::is_subscript_number(L'₃'));
        CHECK(string_util::is_subscript_number(L'₄'));
        CHECK(string_util::is_subscript_number(L'₅'));
        CHECK(string_util::is_subscript_number(L'₆'));
        CHECK(string_util::is_subscript_number(L'₇'));
        CHECK(string_util::is_subscript_number(L'₈'));
        CHECK(string_util::is_subscript_number(L'₉'));
        }
    }

TEST_CASE("Find Whole Word", "[stringutil][whole word]")
    {
    SECTION("Find")
        {
        std::wstring needle{L"needle"};
        std::wstring haystack{ L"needle in the haystack. There are needles in the haystack, including knittingneedles."
                                "knitting-needle? Anyway, just find needle" };
        CHECK(0 == string_util::find_whole_word(haystack, needle));
        CHECK(94 == string_util::find_whole_word(haystack, needle, 1));
        CHECK(94 == string_util::find_whole_word(haystack, needle, 94));
        CHECK(120 == string_util::find_whole_word(haystack, needle, 95));
        CHECK(-1 == string_util::find_whole_word(haystack, needle, 121));
        CHECK(-1 == string_util::find_whole_word(haystack, needle, 1000)); // out of bounds
        }

    SECTION("Find with newlines")
        {
        std::wstring needle{L"needle"};
        std::wstring haystack{ L"needle in the haystack. There are needles in the haystack, including knittingneedles."
                                "knitting\nneedle? Anyway, just find\nneedle" };
        CHECK(0 == string_util::find_whole_word(haystack, needle));
        CHECK(94 == string_util::find_whole_word(haystack, needle, 1));
        CHECK(94 == string_util::find_whole_word(haystack, needle, 94));
        CHECK(120 == string_util::find_whole_word(haystack, needle, 95));
        CHECK(-1 == string_util::find_whole_word(haystack, needle, 121));
        }

    SECTION("No Find")
        {
        std::wstring needle{L"pin"};
        std::wstring haystack{ L"needle in the haystack. There are needles in the haystack, including knittingneedles."
                                 "knitting-needle? Anyway, just find needle" };
        CHECK(-1 == string_util::find_whole_word(haystack, needle));
        CHECK(-1 == string_util::find_whole_word(haystack, needle, 500));
        CHECK(-1 == string_util::find_whole_word(haystack, std::wstring(L"")));
        CHECK(-1 == string_util::find_whole_word(std::wstring(L""), needle));
        }
    }

TEST_CASE("Tokenize", "[stringutil][tokenize]")
	{
	SECTION("DelimAtFrontWord")
		{
        string_util::string_tokenize<std::wstring> tok(L"-vanilla", L"-", false);
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"");
        CHECK(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"vanilla");
        CHECK_FALSE(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
		}
    SECTION("DelimAtEndWord")
		{
        string_util::string_tokenize<std::wstring> tok(L"vanilla-", L"-", true);
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"vanilla");
        CHECK(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"");
        CHECK_FALSE(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
		}
    SECTION("HyphenWord")
		{
        string_util::string_tokenize<std::wstring> tok(L"-", L"-", false);
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"");
        CHECK(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"");
        CHECK_FALSE(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
		}
    SECTION("BlankWord")
		{
        string_util::string_tokenize<std::wstring> tok(L"", L"-", true);
        CHECK_FALSE(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"");
        CHECK_FALSE(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
		}
    SECTION("TwoDelims")
		{
        string_util::string_tokenize<std::wstring> tok(L"vanilla-cake/frosting", L"-/", true);
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"vanilla");
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"cake");
        CHECK(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"frosting");
        CHECK_FALSE(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"");
		}
    SECTION("SkipEmptyTokens")
		{
        string_util::string_tokenize<std::wstring> tok(L"the--end", L"-", true);
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"the");
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"end");
        CHECK_FALSE(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
		}
    SECTION("RepeatedDelimns")
		{
        string_util::string_tokenize<std::wstring> tok(L"the--end", L"-", false);
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"the");
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"");
        CHECK(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"end");
        CHECK_FALSE(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
		}
    SECTION("HyphenTriWord")
		{
        string_util::string_tokenize<std::wstring> tok(L"vanilla-cake-frosting", L"-", true);
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"vanilla");
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"cake");
        CHECK(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"frosting");
        CHECK_FALSE(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"");
		}
    SECTION("HyphenatedWord")
		{
        string_util::string_tokenize<std::wstring> tok(L"vanilla-cake", L"-", true);
        CHECK(tok.has_more_tokens());
        CHECK(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"vanilla");
        CHECK(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"cake");
        CHECK_FALSE(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"");
		}
    SECTION("NoDelimiters")
		{
        string_util::string_tokenize<std::wstring> tok(L"vanilla", L"-", true);
        CHECK(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"vanilla");
        CHECK_FALSE(tok.has_more_tokens());
        CHECK_FALSE(tok.has_more_delimiters());
        CHECK(tok.get_next_token() == L"");
		}
    }

TEST_CASE("String Equal Functors", "[stringutil][equal]")
    {
    SECTION("NoCaseStringMap")
        {
        CHECK_FALSE((equal_basic_string_i_compare_map<std::wstring,size_t>(L"bob").operator()(std::make_pair(L"Fred",2))));
        CHECK((equal_basic_string_i_compare_map<std::wstring,size_t>(L"fred").operator()(std::make_pair(L"Fred",2))));
        CHECK((equal_basic_string_i_compare_map<std::wstring,size_t>(L"bob").operator()(std::make_pair(L"bob",2))));
        CHECK_FALSE((equal_basic_string_i_compare_map<std::wstring,size_t>(L"bob").operator()(std::make_pair(L"bobby",2))));
        }
    SECTION("StringCompare")
        {
        CHECK_FALSE((less_basic_string_compare<std::wstring>{}(L"bob", L"Fred")));
        CHECK((less_basic_string_compare<std::wstring>{}(L"Bob", L"Fred")));
        CHECK((less_basic_string_compare<std::wstring>{}(L"Fred", L"bob")));
        CHECK((less_basic_string_compare<std::wstring>{}(L"bob", L"bobby")));
        CHECK((less_basic_string_compare<std::wstring>{}(L"Bob", L"bobby")));
        CHECK_FALSE((less_basic_string_compare<std::wstring>{}(L"bob", L"Bobby")));
        CHECK_FALSE((less_basic_string_compare<std::wstring>{}(L"bobby", L"bob")));
        CHECK_FALSE((less_basic_string_compare<std::wstring>{}(L"fred", L"Fred")));
        CHECK((less_basic_string_compare<std::wstring>{}(L"Fred", L"fred")));
        CHECK_FALSE((less_basic_string_compare<std::wstring>{}(L"Fred", L"Fred")));
        CHECK_FALSE((less_basic_string_compare<std::wstring>{}(L"bob", L"bob")));
        }
    SECTION("StringICompare")
        {
        CHECK((less_basic_string_i_compare<std::wstring>{}(L"bob", L"Fred")));
        CHECK((less_basic_string_i_compare<std::wstring>{}(L"Bob", L"Fred")));
        CHECK_FALSE((less_basic_string_i_compare<std::wstring>{}(L"Fred", L"bob")));
        CHECK_FALSE((less_basic_string_i_compare<std::wstring>{}(L"fred", L"Bob")));
        CHECK((less_basic_string_i_compare<std::wstring>{}(L"bob", L"bobby")));
        CHECK((less_basic_string_i_compare<std::wstring>{}(L"Bob", L"bobby")));
        CHECK((less_basic_string_i_compare<std::wstring>{}(L"bob", L"Bobby")));
        CHECK_FALSE((less_basic_string_i_compare<std::wstring>{}(L"bobby", L"bob")));
        CHECK_FALSE((less_basic_string_i_compare<std::wstring>{}(L"fred", L"Fred")));
        CHECK_FALSE((less_basic_string_i_compare<std::wstring>{}(L"Fred", L"fred")));
        CHECK_FALSE((less_basic_string_i_compare<std::wstring>{}(L"Fred", L"Fred")));
        CHECK_FALSE((less_basic_string_i_compare<std::wstring>{}(L"bob", L"bob")));
        }
    }

TEST_CASE("Remove all", "[stringutil][remove]")
    {
    SECTION("Empty")
        {
        std::wstring blah(L"");
        string_util::remove_all(blah, L'/');
        CHECK(blah.empty());
        }

    SECTION("NothingRemoved")
        {
        std::wstring blah(L"Some text here");
        string_util::remove_all(blah, L'&');
        CHECK(blah == L"Some text here");
        }

    SECTION("Removed")
        {
        std::wstring blah(L"&Some &te&&xt here&");
        string_util::remove_all(blah, L'&');
        CHECK(blah == L"Some text here");
        }

    SECTION("AllRemoved")
        {
        std::wstring blah(L"&");
        string_util::remove_all(blah, L'&');
        CHECK(blah.empty());
        blah = L"&&&";
        string_util::remove_all(blah, L'&');
        CHECK(blah.empty());
        blah = L"& &&";
        string_util::remove_all(blah, L'&');
        CHECK(blah == L" ");
        }
    }

TEST_CASE("Replace whole word", "[stringutil][whole word]")
    {
    SECTION("Find")
        {
        std::wstring needle{L"needle"};
        std::wstring haystack{ L"needle in the haystack. There are needles in the haystack, including knittingneedles."
                                 "knitting-needle? Anyway, just find needle" };
        string_util::replace_all_whole_word<std::wstring>(haystack, needle, L"pin");
        CHECK(std::wstring(L"pin in the haystack. There are needles in the haystack, including knittingneedles."
                               "knitting-pin? Anyway, just find pin") == haystack);
        string_util::replace_all_whole_word<std::wstring>(haystack, L"pin", L"needle");
        CHECK(std::wstring(L"needle in the haystack. There are needles in the haystack, including knittingneedles."
                                 "knitting-needle? Anyway, just find needle") == haystack);
        }

    SECTION("No Find")
        {
        std::wstring needle{L"pin"};
        std::wstring haystack{ L"needle in the haystack. There are needles in the haystack, including knittingneedles."
                                 "knitting-needle? Anyway, just find needle" };
        string_util::replace_all_whole_word<std::wstring>(haystack, needle, L"pin");
        CHECK(std::wstring(L"needle in the haystack. There are needles in the haystack, including knittingneedles."
                                 "knitting-needle? Anyway, just find needle") == haystack);
        }
    }

TEST_CASE("Is neither", "[isneither]")
    {
    SECTION("Is Either")
        {
        CHECK(is_either(5.1, 5.1, 0.9));
        CHECK(is_either(L'a', L'a', L'z'));
        CHECK(is_either(std::wstring(L"the"), std::wstring(L"there"), std::wstring(L"the")));

        CHECK(is_either(5.1, 5.12, 0.9) == false);
        CHECK(is_either(L'a', L'b', L'z') == false);
        CHECK(is_either(std::wstring(L"the"), std::wstring(L"there"), std::wstring(L"The")) == false);
        }
    SECTION("Is Neither")
        {
        CHECK(is_neither(5.1, 5.1, 0.9) == false);
        CHECK(is_neither(L'a', L'a', L'z') == false);
        CHECK(is_neither(std::wstring(L"the"), std::wstring(L"there"), std::wstring(L"the")) == false);

        CHECK(is_neither(5.1, 5.12, 0.9));
        CHECK(is_neither(L'a', L'b', L'z'));
        CHECK(is_neither(std::wstring(L"the"), std::wstring(L"there"), std::wstring(L"The")));
        }
    }

TEST_CASE("Is trademark", "[trademark]")
    {
    SECTION("Is Either")
        {
        CHECK(is_trademark_or_registration(L'℗'));
        CHECK(is_trademark_or_registration(L'Ⓒ'));
        CHECK(is_trademark_or_registration(L'©'));
        CHECK(is_trademark_or_registration(L'™'));
        CHECK(is_trademark_or_registration(L'®'));
        CHECK(is_trademark_or_registration(L'℠'));
        CHECK(is_trademark_or_registration(L'Ⓜ'));
        CHECK_FALSE(is_trademark_or_registration(L'.'));
        CHECK_FALSE(is_trademark_or_registration(L'!'));
        CHECK_FALSE(is_trademark_or_registration(L','));
        CHECK_FALSE(is_trademark_or_registration(L'@'));
        CHECK_FALSE(is_trademark_or_registration(L' '));
        }
    }

// NOLINTEND
// clang-format on
