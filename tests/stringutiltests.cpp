#include <iomanip>
#include <sstream>
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include "../src/util/stringutil.h"

using namespace string_util;

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
        CHECK(string_util::is_hex_digit<char>('0'));
        CHECK(string_util::is_hex_digit<char>('1'));
        CHECK(string_util::is_hex_digit<char>('2'));
        CHECK(string_util::is_hex_digit<char>('3'));
        CHECK(string_util::is_hex_digit<char>('4'));
        CHECK(string_util::is_hex_digit<char>('5'));
        CHECK(string_util::is_hex_digit<char>('6'));
        CHECK(string_util::is_hex_digit<char>('7'));
        CHECK(string_util::is_hex_digit<char>('8'));
        CHECK(string_util::is_hex_digit<char>('9'));
        CHECK(string_util::is_hex_digit<char>('a'));
        CHECK(string_util::is_hex_digit<char>('b'));
        CHECK(string_util::is_hex_digit<char>('c'));
        CHECK(string_util::is_hex_digit<char>('d'));
        CHECK(string_util::is_hex_digit<char>('e'));
        CHECK(string_util::is_hex_digit<char>('f'));
        CHECK(string_util::is_hex_digit<char>('A'));
        CHECK(string_util::is_hex_digit<char>('B'));
        CHECK(string_util::is_hex_digit<char>('C'));
        CHECK(string_util::is_hex_digit<char>('D'));
        CHECK(string_util::is_hex_digit<char>('E'));
        CHECK(string_util::is_hex_digit<char>('F'));
        CHECK_FALSE(string_util::is_hex_digit<char>('g'));
        CHECK_FALSE(string_util::is_hex_digit<char>('.'));

        CHECK(string_util::is_hex_digit<wchar_t>(L'0'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'1'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'2'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'3'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'4'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'5'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'6'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'7'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'8'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'9'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'a'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'b'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'c'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'd'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'e'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'f'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'A'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'B'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'C'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'D'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'E'));
        CHECK(string_util::is_hex_digit<wchar_t>(L'F'));
        CHECK_FALSE(string_util::is_hex_digit<wchar_t>(L'g'));
        CHECK_FALSE(string_util::is_hex_digit<wchar_t>(L'.'));
        }
    SECTION("Axtoi")
        {
        size_t length(static_cast<size_t>(-1));
        CHECK(string_util::axtoi(static_cast<wchar_t*>(nullptr), length) == 0);
        CHECK(0 == length);
        length = 4;
        CHECK(string_util::axtoi(static_cast<wchar_t*>(nullptr), length) == 0);
        CHECK(0 == length);

        length = static_cast<size_t>(-1);
        CHECK(5513 == string_util::axtoi(L"0x1589", length));
        CHECK(4 == length);

        length = 20;//bogus length
        CHECK(string_util::axtoi(L"0x1589", length) == 5513);
        CHECK(4 == length);

        length = 6;
        CHECK(7894 == string_util::axtoi(L"0x1ed6", length));
        length = 6;
        CHECK(7894 == string_util::axtoi(L"0x1ED6", length));

        length = 9;
        CHECK(32335749 == string_util::axtoi(L"0x1ed6785", length));

        length = 7;
        CHECK(32335749 == string_util::axtoi(L"1ed6785", length));
        CHECK(0 == string_util::axtoi(L"Hello", length));
        CHECK(0 == length);

        length = 5;
        CHECK(854756 == string_util::axtoi(L"d0Ae4", length));
        CHECK(5 == length);

        length = static_cast<size_t>(-1);
        CHECK(854756 == string_util::axtoi(L"d0Ae4", length));
        CHECK(5 == length);

        length = 10;
        CHECK(745269863 == string_util::axtoi(L"0x2c6bea67", length));
        CHECK(8 == length);

        CHECK(string_util::axtoi(L"2c6bea67", length) == 745269863);
        CHECK(string_util::axtoi(L"2c6bea67", length) == 745269863);

        length = 10;
        CHECK(string_util::axtoi(L"0x2c6bea67", length) == 745269863);

        length = static_cast<size_t>(-1);
        CHECK(string_util::axtoi(L"0x2c6bea67", length) == 745269863);

        length = 1;
        CHECK(string_util::axtoi(L"D", length) == 13);
        CHECK(string_util::axtoi(L"0", length) == 0);
        CHECK(string_util::axtoi(L"0", length) == 0);

        length = 2;
        CHECK(0 == string_util::axtoi(L"0", length));
        CHECK(1 == length);
        length = 2;
        CHECK(0 == string_util::axtoi(L"0x", length));
        CHECK(0 == length);
        length = static_cast<size_t>(-1);
        CHECK(0 == string_util::axtoi(L"0x", length));
        length = 0;
        CHECK(0 == string_util::axtoi(L"0x", length));
        length = 1;
        CHECK(0 == string_util::axtoi(L"0x", length));
        length = 2;
        CHECK(string_util::axtoi(L"0x", length) == 0);
        length = 3;
        CHECK(string_util::axtoi(L"0x0", length) == 0);
        CHECK(1 == length);
        length = static_cast<size_t>(-1);
        CHECK(string_util::axtoi(L"0x39", length) == 57);
        CHECK(2 == length);
        length = 4;
        CHECK(57 == string_util::axtoi(L"0x39", length));
        CHECK(2 == length);
        length = static_cast<size_t>(-1);
        CHECK(57 == string_util::axtoi(L"39", length));
        length = 2;
        CHECK(57 == string_util::axtoi(L"39", length));
        length = static_cast<size_t>(-1);
        CHECK(57 == string_util::axtoi(L"0x0039", length));
        CHECK(4 == length);
        length = 6;
        CHECK(57 == string_util::axtoi(L"0x0039", length));
        CHECK(4 == length);
        length = static_cast<size_t>(-1);
        CHECK(57 == string_util::axtoi(L"0039", length));
        CHECK(4 == length);
        length = 4;
        CHECK(57 == string_util::axtoi(L"0039", length));
        CHECK(4 == length);
        length = static_cast<size_t>(-1);
        CHECK(175 == string_util::axtoi(L"00AF", length));
        length = 4;
        CHECK(175 == string_util::axtoi(L"00AF", length));
        length = 2;
        CHECK(string_util::axtoi(L"", length) == 0);
        CHECK(0 == length);
        length = static_cast<size_t>(-1);
        CHECK(305419896 == string_util::axtoi(L"0x012345678", length));
        length = static_cast<size_t>(-1);
        CHECK(591751049 == string_util::axtoi(L"0x023456789", length));
        }
    SECTION("Axtoi Full Range")
        {
        for (int i = 0; i < 10000; ++i)
            {
            std::wstringstream stream;
            stream << L"0x" << std::hex << i;

            std::wstring lowerStr = stream.str();
            std::transform(lowerStr.cbegin(), lowerStr.cend(), lowerStr.begin(), ::tolower);
            std::wstring upperStr = stream.str();
            std::transform(upperStr.cbegin(), upperStr.cend(), upperStr.begin(), ::toupper);
            size_t length = lowerStr.length();
            CHECK(i == string_util::axtoi(lowerStr.c_str(), length));
            length = upperStr.length();
            CHECK(i == string_util::axtoi(upperStr.c_str(), length));
            length = lowerStr.length();
            }
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
