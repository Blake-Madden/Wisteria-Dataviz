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

TEST_CASE("atoi", "[stringutil][atoi]")
    {
    SECTION("NULLs")
        {
        CHECK(string_util::atoi(L"") == 0);
        wchar_t* val = nullptr;
        CHECK(string_util::atoi(val) == 0);
        CHECK(string_util::atoi(L"873") == 873);
        CHECK(string_util::atoi(L"7") == 7);
        CHECK(string_util::atoi(L"-10") == -10);
        }
    }

TEST_CASE("StrICmp", "[stringutil][StrICmp]")
    {
    SECTION("TestNulls")
        {
        CHECK(string_util::stricmp("", "") == 0);
        CHECK(string_util::stricmp<char>(nullptr, nullptr) == 0);
        CHECK(string_util::stricmp<char>(nullptr, "a") < 0);
        CHECK(string_util::stricmp<char>("a", nullptr) > 0);
        }
    SECTION("TestCaseComparisons")
        {
        CHECK(string_util::stricmp("some text", "SoMe TEXt") == 0);
        CHECK(string_util::stricmp("some text", "SoMe TEXt") == 0);
        CHECK(string_util::stricmp("SoMe TEXt", "some text") == 0);
        }
    SECTION("TestDifferentLengthComparison")
        {
        CHECK(string_util::stricmp("SoMe TEXt", "some") > 0);
        CHECK(string_util::stricmp("Text100", "Text0") > 0);
        CHECK(string_util::stricmp("Text100", "Text00100moretext") > 0);
        CHECK(string_util::stricmp("Text000", "Text10000moretext") < 0);
        }
    }

TEST_CASE("StrNICmp", "[stringutil][StrNICmp]")
    {
    SECTION("TestNulls")
        {
        CHECK(string_util::strnicmp("", "", 9) == 0);
        CHECK(string_util::strnicmp<char>(nullptr, nullptr, 9) == 0);
        CHECK(string_util::strnicmp<char>(nullptr, "a", 9) < 0);
        CHECK(string_util::strnicmp<char>("a", nullptr, 9) > 0);
        }
    SECTION("TestCaseComparisons")
        {
        CHECK(string_util::strnicmp("some text", "SoMe TEXt", 9) == 0);
        CHECK(string_util::strnicmp("some text", "SoMe TEXt", 9) == 0);
        CHECK(string_util::strnicmp("SoMe TEXt", "some text", 9) == 0);
        }
    SECTION("TestDifferentLengthComparison")
        {
        CHECK(string_util::strnicmp("SoMe TEXt", "some", 4) == 0);
        CHECK(string_util::strnicmp("Text100", "Text0", 4) == 0);
        CHECK(string_util::strnicmp("Text100", "Text00100moretext", 5) > 0);
        CHECK(string_util::strnicmp("Text000", "Text10000moretext", 5) < 0);
        }
    }

TEST_CASE("HasSuffix", "[stringutil][HasSuffix]")
    {
    SECTION("TestNulls")
        {
        CHECK(string_util::has_suffix<wchar_t>(nullptr, 5, L"es", 2) == false);
        CHECK(string_util::has_suffix<wchar_t>(L"hunted", 6, nullptr, 2) == false);
        CHECK(string_util::has_suffix<wchar_t>(L"e", 1, L"es", 2) == false);
        }
    SECTION("TestHasSuffix")
        {
        CHECK(string_util::has_suffix<wchar_t>(L"hunted", 6, L"ed", 2));
        CHECK(string_util::has_suffix<wchar_t>(L"ted", 3, L"ed", 2));
        }
    SECTION("TestNotHasSuffix")
        {
        CHECK(string_util::has_suffix<wchar_t>(L"hunted", 6, L"es", 2) == false);
        // text must be longer than suffix for it to have the suffix
        CHECK(string_util::has_suffix<wchar_t>(L"ed", 2, L"ed", 2) == false);
        }
    }

TEST_CASE("StrCSpnPointer", "[stringutil][StrCSpnPointer]")
    {
    SECTION("TestTestNulls")
        {
        CHECK(string_util::strcspn_pointer<char>(nullptr, "hello", 5) == nullptr);
        CHECK(string_util::strcspn_pointer<char>("HelLo", nullptr, 0) == nullptr);
        CHECK(string_util::strcspn_pointer<char>(nullptr, nullptr, 0) == nullptr);
        }
    SECTION("TestFindFirst")
        {
        const char* buffer = "<blah blah>";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer);
        buffer = ">blah blah>";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer);
        }
    SECTION("TestFindMiddle")
        {
        const char* buffer = "blah <blah";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer+5);
        buffer = "blah >blah";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer+5);
        }
    SECTION("TestFindLast")
        {
        const char* buffer = "blah blah<";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer+9);
        buffer = "blah blah>";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == buffer+9);
        }
    SECTION("TestNotFind")
        {
        const char* buffer = "blah blah";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == nullptr);
        buffer = "blah blah";
        CHECK(string_util::strcspn_pointer<char>(buffer, "<>", 2) == nullptr);
        CHECK(string_util::strcspn_pointer<char>("", "<>", 2) == nullptr);
        }
    }

TEST_CASE("FindLastNotOf", "[stringutil][FindLastNotOf]")
    {
    SECTION("TestNulls")
        {
        const wchar_t* buffer = L"ABB9";
        CHECK(string_util::find_last_not_of<wchar_t>(nullptr, L"0123456789") == -1);
        CHECK(string_util::find_last_not_of<wchar_t>(buffer, nullptr) == -1);
        }
    SECTION("TestNotFound")
        {
        const wchar_t* buffer = L"124578";
        CHECK(string_util::find_last_not_of<wchar_t>(buffer, L"0123456789") == -1);
        }
    SECTION("TestFound")
        {
        const wchar_t* buffer = L"ABB8";
        CHECK(string_util::find_last_not_of<wchar_t>(buffer, L"0123456789") == 2);
        }
    SECTION("TestFoundAtEnd")
        {
        const wchar_t* buffer = L"5ABB";
        CHECK(string_util::find_last_not_of<wchar_t>(buffer, L"0123456789") == 3);
        }
    SECTION("TestFoundUsingOffset")
        {
        const wchar_t* buffer = L"A56BB5";
        CHECK(string_util::find_last_not_of<wchar_t>(buffer, L"0123456789", 2) == 0);
        CHECK(string_util::find_last_not_of<wchar_t>(buffer, L"0123456789") == 4); // not using offset
        }
    SECTION("TestEmptyString")
        {
        const wchar_t* buffer = L"";
        CHECK(string_util::find_last_not_of<wchar_t>(buffer, L"0123456789") == -1);
        buffer = L"A56BB5";
        CHECK(string_util::find_last_not_of<wchar_t>(buffer, L"") == -1);
        }
    }

TEST_CASE("FindMatchingTag", "[stringutil][FindMatchingTag]")
    {
    SECTION("TestClosingWithOpenTagsStrings")
        {
        const wchar_t* buffer = L"[[img [[]]]hello]]], world";
        CHECK(string_util::find_matching_close_tag<wchar_t>(buffer+1, L"[[", L"]]]") == buffer+16);
        }
    SECTION("TestClosingWithOpenTagsStrings2")
        {
        const wchar_t* buffer = L"[[img [[]]]h[[e]]]llo]]], world";
        CHECK(string_util::find_matching_close_tag<wchar_t>(buffer+1, L"[[", L"]]]") == buffer+21);
        }
    SECTION("TestClosingWithOpenTagsStringsStartWithSameChar")
        {
        const wchar_t* buffer = L"[[img [[[]]hello[]], world";
        CHECK(string_util::find_matching_close_tag<wchar_t>(buffer+1, L"[[", L"[]]") == buffer+16);
        }
    SECTION("TestTestNullsStrings")
        {
        CHECK(string_util::find_matching_close_tag<wchar_t>(nullptr, L"[[", L"[]]") == nullptr);
        CHECK(string_util::find_matching_close_tag<wchar_t>(L"text", nullptr, L"[]]") == nullptr);
        CHECK(string_util::find_matching_close_tag<wchar_t>(L"text", L"[]]", nullptr) == nullptr);
        CHECK(string_util::find_matching_close_tag<wchar_t>(L"[[img [[[]]hello[]], world", L"", L"[]]") == nullptr);
        CHECK(string_util::find_matching_close_tag<wchar_t>(L"[[img [[[]]hello[]], world", L"[[", L"") == nullptr);
        }
    SECTION("TestNoClosingTagsStrings")
        {
        const wchar_t* buffer = L"[[img hello, world";
        CHECK(string_util::find_matching_close_tag<wchar_t>(buffer+1, L"[[", L"[]]") == nullptr);
        }
    SECTION("TestClosingWithStrayOpenTagStrings")
        {
        const wchar_t* buffer = L"[[img [[ihello[]], world";
        CHECK(string_util::find_matching_close_tag<wchar_t>(buffer+1, L"[[", L"[]]") == nullptr);
        }
    // single characters
    SECTION("TestTestNulls")
        {
        CHECK(string_util::find_matching_close_tag<wchar_t>(nullptr, L'<', L'>') == nullptr);
        }
    SECTION("TestNoClosingTags")
        {
        const wchar_t* buffer = L"<img hello, world";
        CHECK(string_util::find_matching_close_tag<wchar_t>(buffer+1, L'<', L'>') == nullptr);
        }
    SECTION("TestClosingTags")
        {
        const wchar_t* buffer = L"<img hello>, world";
        CHECK(string_util::find_matching_close_tag<wchar_t>(buffer+1, L'<', L'>') == buffer+10);
        }
    SECTION("TestClosingWithOpenTags")
        {
        const wchar_t* buffer = L"<img <i>hello</i>>, world";
        CHECK(string_util::find_matching_close_tag<wchar_t>(buffer+1, L'<', L'>') == buffer+17);
        }
    SECTION("TestClosingWithStrayOpenTag")
        {
        const wchar_t* buffer = L"<img <ihello>, world";
        CHECK(string_util::find_matching_close_tag<wchar_t>(buffer+1, L'<', L'>') == nullptr);
        }
    }

TEST_CASE("ItoaStr", "[stringutil][ItoaStr]")
    {
    SECTION("TestItoaiNULL")
        {
        wchar_t result[5];
        CHECK(string_util::itoa(548, (wchar_t*)nullptr, 5) == -1);
        CHECK(string_util::itoa(548, result, 0) == -1);

        CHECK(string_util::itoa(12345, result, 5) == -1); // no space for nullptr terminator, should fail
        CHECK(std::wmemcmp(result, L"\0\0\0\0\0", 5) == 0);

        CHECK(string_util::itoa(1234, result, 5) == 0); // just enough space
        CHECK(std::wcscmp(result, L"1234") == 0);
        }

    SECTION("TestItoai")
        {
        wchar_t result[12];
        CHECK(string_util::itoa(-112993, result, 12) == 0);
        CHECK(std::wcscmp(result, L"-112993") == 0);

        CHECK(string_util::itoa(-9, result, 12) == 0);
        CHECK(std::wcscmp(result, L"-9") == 0);

        CHECK(string_util::itoa(9, result, 12) == 0);
        CHECK(std::wcscmp(result, L"9") == 0);

        CHECK(string_util::itoa(98, result, 12) == 0);
        CHECK(std::wcscmp(result, L"98") == 0);

        CHECK(string_util::itoa(978, result, 12) == 0);
        CHECK(std::wcscmp(result, L"978") == 0);

        CHECK(string_util::itoa(0, result, 12) == 0);
        CHECK(std::wcscmp(result, L"0") == 0);

        CHECK(string_util::itoa(84579541, result, 12) == 0);
        CHECK(std::wcscmp(result, L"84579541") == 0);
        }
    };

TEST_CASE("RemoveSpaces", "[stringutil][RemoveSpaces]")
    {
    SECTION("TestRemoveBlankLinesEmpty")
        {
        std::wstring text(L"");
        CHECK(string_util::remove_blank_lines(text) == 0);
        CHECK(text == L"");
        }
    SECTION("TestRemoveBlankLines")
        {
        std::wstring text(L"Blah\n\nLine2");
        CHECK(string_util::remove_blank_lines(text) == 1);
        CHECK(text == L"Blah\nLine2");
        }
    SECTION("TestRemoveBlankLines2")
        {
        std::wstring text(L"Blah\n\nLine2\n\n");
        CHECK(string_util::remove_blank_lines(text) == 2);
        CHECK(text == L"Blah\nLine2\n");
        }
    SECTION("TestRemoveBlankLines3")
        {
        std::wstring text(L"Blah\r\n\r\nLine2\n\n");
        CHECK(string_util::remove_blank_lines(text) == 3);
        CHECK(text == L"Blah\r\nLine2\n");
        }
    SECTION("TestRemoveSpacesEmpty")
        {
        std::wstring text(L"");
        CHECK(string_util::remove_extra_spaces(text) == 0);
        CHECK(text == L"");
        }
    SECTION("TestRemoveNoSpaces")
        {
        std::wstring text(L"hellothereworld!newline");
        CHECK(string_util::remove_extra_spaces(text) == 0);
        CHECK(text == L"hellothereworld!newline");
        }
    SECTION("TestRemoveSpaces")
        {
        std::wstring text(L"hello  there    \t \r\n world !\r\nnew line");
        CHECK(string_util::remove_extra_spaces(text) == 9);
        CHECK(text == L"hello there world !\r\nnew line");
        }
    SECTION("TestRemoveSpacesNoExtraSpaces")
        {
        std::wstring text(L"hello there\tworld!\r\nnew line");
        CHECK(string_util::remove_extra_spaces(text) == 0);
        CHECK(text == L"hello there\tworld!\r\nnew line");
        }
    };

TEST_CASE("StrIStr", "[stringutil][StrIStr]")
    {
    SECTION("TestTestNulls")
        {
        CHECK(string_util::stristr<char>(nullptr, "HelLo") == nullptr);
        CHECK(string_util::stristr<char>("HelLo", nullptr) == nullptr);
        }
    SECTION("TestFindFirstItem")
        {
        char buffer[] = "hello, world";
        // should find at the beginning
        CHECK(string_util::stristr(buffer, "HelLo") == buffer);
        }
    SECTION("TestLastItemInSequenceString")
        {
        char buffer[] = "hello, world";
        // should find last item in sequence
        CHECK(string_util::stristr(buffer, "WORLD") == buffer+7);
        }
    SECTION("TestMiddleItemInSequenceString")
        {
        char buffer[] = "hello, world!!! Goodbye, cruel world!";
        // should find last item in sequence
        CHECK(string_util::stristr(buffer, "WORLD") == buffer+7);
        }
    SECTION("TestFindNothing")
        {
        char buffer[] = "hello, world";
        // should find nothing and return nullptr
        CHECK(string_util::stristr(buffer, "Help") == nullptr);
        }
    SECTION("TestFindNothingEmptySearchString")
        {
        char buffer[] = "hello, world";
        // should find nothing and return nullptr
        CHECK(string_util::stristr(buffer, "") == nullptr);
        }
    SECTION("TestEmptyString")
        {
        // should find nothing and return nullptr
        CHECK(string_util::stristr("", "Hello") == nullptr);
        }
    SECTION("TestSubStringTooBig")
        {
        CHECK(string_util::stristr("Hello", "Hello World") == nullptr);
        CHECK(string_util::stristr("Hello", "StringLongerThanMainString") == nullptr);
        }
    };

TEST_CASE("StrNChr", "[stringutil][StrNChr]")
    {
    SECTION("TestNull")
        {
        CHECK(string_util::strnchr<char>(nullptr, ',', 5) == nullptr);
        }
    SECTION("TestNotSearchFarEnough")
        {
        char buffer[] = "hello, world";
        // should only search "hello" and not find ,
        CHECK(string_util::strnchr(buffer, ',', 5) == nullptr);
        }
    SECTION("TestFindFirstItem")
        {
        char buffer[] = "hello, world";
        // should find first ,
        CHECK(string_util::strnchr(buffer, ',', 6) == buffer+5);
        }
    SECTION("TestLastItemInSequenceString")
        {
        char buffer[] = "hello, world";
        // should find last item in sequence
        CHECK(string_util::strnchr(buffer, 'd', 12) == buffer+11);
        }
    SECTION("TestFindNothing")
        {
        char buffer[] = "hello, world";
        // should find nothing and return size argument
        CHECK(string_util::strnchr(buffer, 'z', 12) == nullptr);
        }
    SECTION("TestEmptyString")
        {
        // should find nothing and nullptr
        CHECK(string_util::strnchr("", 'z', 0) == nullptr);
        }
    SECTION("TestBoundaryError")
        {
        wchar_t text[100];
        std::wmemset(text, 0, 100);
        std::wcscpy(text, L"abc");
        // will really only scan first three letter, see the embedded NULLs, and return
        // what the caller thought was the length of the string to know that it failed.
        CHECK(string_util::strnchr<wchar_t>(L"abc", L'x', 100) == nullptr);
        }
    };

TEST_CASE("StrtodEx", "[stringutil][StrtodEx]")
    {
    SECTION("TestNull")
        {
        wchar_t* end;
        CHECK(string_util::strtod_ex<wchar_t>(nullptr, &end) == 0);
        CHECK(end == nullptr);
        }
    // make sure your system's region is set to English or won't work
    // (this test case assumes US English decimal)
    SECTION("TestDouble")
        {
        wchar_t* end;
        const wchar_t* value = L"5.27";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 5.27);
        // end should point to the value's null terminator
        CHECK(end == (value+4));
        CHECK(*end == 0);
        }
    SECTION("TestEndingHyphen")
        {
        wchar_t* end;
        const wchar_t* value = L"5.27-";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 5.27);
        // end should point to the dash at the end
        CHECK(end == (value+4));
        CHECK(*end == L'-');
        }
    SECTION("TestHyphen")
        {
        wchar_t* end;
        const wchar_t* value = L"5.5-6";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 5.75);
        // end should point to the value's null terminator
        CHECK(end == (value+5));
        CHECK(*end == 0);
        }
    SECTION("TestColon")
        {
        wchar_t* end;
        const wchar_t* value = L"5.5:6";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 5.75);
        // end should point to the value's null terminator
        CHECK(end == (value+5));
        CHECK(*end == 0);
        }
    SECTION("TestNonDoubles")
        {
        wchar_t* end;
        const wchar_t* value = L"5:8";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 6.5);
        // end should point to the value's null terminator
        CHECK(end == (value+3));
        CHECK(*end == 0);
        }
    SECTION("TestZeros")
        {
        wchar_t* end;
        const wchar_t* value = L"0:0.0";
        CHECK(string_util::strtod_ex<wchar_t>(value, &end) == 0);
        // end should point to the value's null terminator
        CHECK(end == (value+5));
        CHECK(*end == 0);
        }
    };

TEST_CASE("StrNCsp", "[stringutil][StrNCsp]")
    {
    SECTION("TestNulls")
        {
        CHECK(string_util::strncspn<char>(nullptr, 5, ", w", 3) == 5);
        CHECK(string_util::strncspn<char>("hello, world", 5, nullptr, 0) == 5);
        CHECK(string_util::strncspn<char>("hello, world", 0, ", w", 3) == 0);
        }
    SECTION("TestNotSearchFarEnough")
        {
        // should only search "hello" and not find , or ' '
        CHECK(string_util::strncspn("hello, world", 5, ", w", 3) == 5);
        }
    SECTION("TestFindFirstItem")
        {
        // should find first ,
        CHECK(string_util::strncspn("hello, world", 6, ", ", 2) == 5);
        }
    SECTION("TestOneItemSequenceString")
        {
        // should find first ,
        CHECK(string_util::strncspn("hello, world", 6, ",", 1) == 5);
        }
    SECTION("TestFirstItemEarly")
        {
        // should find first l before reaching last character in string being scanned
        CHECK(string_util::strncspn("hello, world", 9, "l ", 2) == 2);
        }
    SECTION("TestLastItemInSequenceString")
        {
        // should find last item in sequence
        CHECK(string_util::strncspn("hello, world", 9, "zy ", 3) == 6);
        }
    SECTION("TestFindNothing")
        {
        // should find nothing and return size argument
        CHECK(string_util::strncspn("hello, world", 12, "zy", 2) == 12);
        }
    SECTION("TestLargeSequenceStringFindNothing")
        {
        // should find nothing and return size argument
        CHECK(string_util::strncspn("hello, world", 12, "abcfgijkmnpqstuvxyz", 19) == 12);
        }
    SECTION("TestEmptySequenceString")
        {
        // should find nothing and return size argument
        CHECK(string_util::strncspn("hello, world", 12, "", 0) == 12);
        }
    SECTION("TestEmptyString")
        {
        // should find nothing and return size argument
        CHECK(string_util::strncspn("", 0, "abcdef", 6) == 0);
        }
    SECTION("TestBoundaryError")
        {
        wchar_t text[100];
        std::wmemset(text, 0, 100);
        std::wcscpy(text, L"abc");
        // will really only scan first three letter, see the embedded NULLs, and return
        // what the caller thought was the length of the string to know that it failed.
        CHECK(string_util::strncspn(L"abc", 100, L"x", 1) == 100);
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
    SECTION("TestNulls")
        {
        CHECK(string_util::strnlen<wchar_t>(nullptr, 5) == 0);
        }
    SECTION("TestNormal")
        {
        CHECK(string_util::strnlen(L"hello", 5) == 5);
        CHECK(string_util::strnlen(L"longer string here. ", 20) == 20);
        }
    SECTION("TestNotScanningWholeText")
        {
        CHECK(string_util::strnlen(L"hello", 3) == 3);
        CHECK(string_util::strnlen(L"longer string here. ", 15) == 15);
        }
    SECTION("TestMaxValueTooBig")
        {
        CHECK(string_util::strnlen(L"hello", 10) == 5);
        CHECK(string_util::strnlen(L"longer string here. ", 999) == 20);
        }
    SECTION("TestBadString")
        {
        const wchar_t text[5] = { L'h', L'e', L'l', L'l', L'o' }; // no nullptr terminator
        CHECK(string_util::strnlen(text, 5) == 5);
        }
    };

TEST_CASE("FindLastOf", "[stringutil][FindLastOf]")
    {
    SECTION("TestNulls")
        {
        CHECK(string_util::find_last_of<wchar_t>(nullptr, L'/', 1) == -1);
        CHECK(string_util::find_last_of(L"", L'/') == -1);
        }
    SECTION("TestFind")
        {
        CHECK(string_util::find_last_of(L"fire/dept.", L'/') == 4);
        CHECK(string_util::find_last_of(L"fire/dept.", L'/', 9) == 4);
        CHECK(string_util::find_last_of(L"fire/dept.", L'/', 5) == 4);
        }
    SECTION("TestNotFind")
        {
        CHECK(string_util::find_last_of(L"fire/dept.", L'/', 3) == -1);
        CHECK(string_util::find_last_of(L"fire/dept.", L'z', 4) == -1);
        CHECK(string_util::find_last_of(L"fire/dept.", L'z', (size_t)-1) == -1);
        }
    };

TEST_CASE("FindLastOfSequence", "[stringutil][FindLastOfSequence]")
    {
    SECTION("TestNulls")
        {
        CHECK(string_util::find_last_of<wchar_t>(nullptr, L"/", 1) == -1);
        CHECK(string_util::find_last_of(L"", L"/") == -1);
        }
    SECTION("TestFind")
        {
        CHECK(string_util::find_last_of(L"fire/*dept.", L"/") == 4);
        CHECK(string_util::find_last_of(L"fire/*dept.", L"/*") == 5);
        CHECK(string_util::find_last_of(L"fire/dept.", L"/*", 9) == 4);
        CHECK(string_util::find_last_of(L"fire/dept.", L"/*", 4) == 4);
        CHECK(string_util::find_last_of(L"%fire/dept.", L"*%", 10) == 0);
        }
    SECTION("TestNotFind")
        {
        CHECK(string_util::find_last_of(L"fire/dept.", L"/*%", 3) == -1);
        CHECK(string_util::find_last_of(L"fire/dept.", L"*%", 4) == -1);
        CHECK(string_util::find_last_of(L"fire/dept.", L"*%", (size_t)-1) == -1);
        }
    };

TEST_CASE("StrNIStr", "[stringutil][StrNIStr]")
    {
    SECTION("TestNotSearchFarEnough")
        {
        char buffer[] = "hello, world";
        // should only search "hell" and not find "hello"
        CHECK(string_util::strnistr(buffer, "HeLlO", 4) == nullptr);
        }
    SECTION("TestFindItemBufferAndSearchAreTheSame")
        {
        char buffer[] = "hello";
        // should find first ,
        CHECK(string_util::strnistr(buffer, "HelLo", std::strlen(buffer)) == buffer);
        }
    SECTION("TestFindFirstItem")
        {
        char buffer[] = "hello, world";
        // should find first ,
        CHECK(string_util::strnistr(buffer, "HelLo", 5) == buffer);
        }
    SECTION("TestFindItemInMiddle")
        {
        char buffer[] = "hello, there world";
        // should find first ,
        CHECK(string_util::strnistr(buffer, "THErE", std::strlen(buffer)) == buffer+7);
        }
    SECTION("TestLastItemInSequenceString")
        {
        char buffer[] = "hello, world";
        // should find last item in sequence
        CHECK(string_util::strnistr(buffer, "WORLD", std::strlen(buffer) ) == buffer+7);
        }
    SECTION("TestLastItemInSequenceStringCharacter")
        {
        char buffer[] = "hello, world";
        //should find last item in sequence
        CHECK(string_util::strnistr(buffer, "d", std::strlen(buffer) ) == buffer+11);
        }
    SECTION("TestFindNothing")
        {
        char buffer[] = "hello, world";
        // should find nothing and return nullptr
        CHECK(string_util::strnistr(buffer, "Help", std::strlen(buffer)) == nullptr);
        }
    SECTION("TestFindNothingPartialMatchAtEnd")
        {
        char buffer[] = "hello, world";
        // should find nothing and return nullptr
        CHECK(string_util::strnistr(buffer, "WORLDs", std::strlen(buffer)) == nullptr);
        }
    SECTION("TestFindNothingNonTerminatedBuffer")
        {
        char buffer[] = { 'h', 'e', 'l', 'l', 'o' };
        // should find nothing and return nullptr
        CHECK(string_util::strnistr(buffer, "hello there", 5) == nullptr);
        }
    SECTION("TestFindNothingEmptyString")
        {
        char buffer[] = "hello, world";
        // should find nothing and return nullptr
        CHECK(string_util::strnistr(buffer, "", std::strlen(buffer)) == nullptr);
        }
    SECTION("TestEmptyString")
        {
        // should find nothing and nullptr
        CHECK(string_util::strnistr("", "Hello", 0) == nullptr);
        }
    SECTION("TestBoundaryError")
        {
        // pass in buffersize that is incorrect
        CHECK(string_util::strnistr("", "Hello", 5) == nullptr);
        }
    SECTION("TestBoundaryError2")
        {
        // pass in buffersize that is incorrect
        CHECK(string_util::strnistr("Hell", "Hello", 5) == nullptr);
        }
    SECTION("TestSubStringTooBig")
        {
        CHECK(string_util::strnistr("Hello", "Hello World", 11) == nullptr);
        }
    };