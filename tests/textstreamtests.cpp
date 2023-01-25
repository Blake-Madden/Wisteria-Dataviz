#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/util/textstream.h"

// Assigning a char* to a wxString in a UTF-8 encoded file behaves differently between Windows and UNIX.
// On Windows, MSVC converts the char* to the current locale, whereas UNIX leaves it in the raw UTF-8 data.
#ifdef __WXMSW__
    #define CONVERT_STR wxConvLocal
#else
    #define CONVERT_STR wxConvUTF8
#endif

TEST_CAST("Char Stream To Unicode", "[text stream]")
    {
    SECTION("Simple Buffer")
        {
        const char* text = "Hello, world! Here is some simple text.";
        wchar_t* dest = new wchar_t[std::strlen(text)+1];
        std::unique_ptr<wchar_t> buffDelete(dest);
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, (std::strlen(text)+1), text, std::strlen(text)));
        CHECK(std::wstring(L"Hello, world! Here is some simple text.") == std::wstring(dest));
        }

    SECTION("XML Buffer")
        {
        const char* text = "<?xml encoding=\"windows-1252\">HÉllo, world! Here is some simple text.";
        wchar_t* dest = new wchar_t[std::strlen(text)+1];
        std::unique_ptr<wchar_t> buffDelete(dest);
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, (std::strlen(text)+1), text, std::strlen(text)));
        CHECK(std::wstring(L"<?xml encoding=\"windows-1252\">HÉllo, world! Here is some simple text.") == std::wstring(dest));
        }

    SECTION("Wrong Charset Buffer")
        {
        const char* text = "HÉllo, world! Here is some simple text.";
        wchar_t* dest = new wchar_t[std::strlen(text)+1];
        std::unique_ptr<wchar_t> buffDelete(dest);
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, (std::strlen(text)+1), text, std::strlen(text), "utf-16"));
        CHECK(std::wstring(L"HÉllo, world! Here is some simple text.") == std::wstring(dest));
        }

    SECTION("Ansi Buffer")
        {
        const char* text = "T\x00E9l\x00E9 charger la Version d'\x00C9 valuation";
        wchar_t* dest = new wchar_t[std::strlen(text)+1];
        std::unique_ptr<wchar_t> buffDelete(dest);
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, (std::strlen(text)+1), text, std::strlen(text)));
        // should figure it out
        CHECK(std::wstring(L"Télé charger la Version d'É valuation") == std::wstring(dest));
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, (std::strlen(text)+1), text, std::strlen(text), "windows-1252"));
        CHECK(std::wstring(L"Télé charger la Version d'É valuation") == std::wstring(dest));
        }

    SECTION("Broken Encoding Buffer")
        {
        const char* text = "T\xE9\x6C\xE9\xE9 charger la Version d'\xC9 valuation";
        const size_t BuffSize = std::strlen(text)+1;
        wchar_t* dest = new wchar_t[BuffSize];
        std::unique_ptr<wchar_t> buffDelete(dest);
        // not really utf-8, so "bogus" characters get skipped over
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, (BuffSize), text, std::strlen(text), "utf-8"));
        CHECK(std::wstring(L"Tl charger la Version d' valuation") == std::wstring(dest));
        // bad character at start of stream
        text = "\xE9\x6C\xE9 charger la Version d'\xC9 valuation";
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, (BuffSize), text, std::strlen(text), "utf-8"));
        CHECK(std::wstring(L"l charger la Version d' valuation") == std::wstring(dest));
        // bad character at end of stream
        text = "T\xE9\x6C\xE9 charger la Version d'\xC9 valuation\xE9";
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, (BuffSize), text, std::strlen(text), "utf-8"));
        CHECK(std::wstring(L"Tl charger la Version d' valuation") == std::wstring(dest));
        // with BOM
        text = "ï»¿TélÃ©charger la Version d'Ã‰valuation";
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, (BuffSize), text, std::strlen(text), "utf-8"));
        // bogus "é" will be stripped out
        CHECK(std::wstring(L"Tlécharger la Version d'Évaluation") == std::wstring(dest));
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, (BuffSize), text, std::strlen(text)));
        CHECK(std::wstring(L"Tlécharger la Version d'Évaluation") == std::wstring(dest));
        }

    SECTION("UTF8 Encoding Buffer")
        {
        const char* text = "ï»¿TÃ©lÃ©charger la Version d'Ã‰valuation";
        const size_t BuffSize = std::strlen(text)+1;
        wchar_t* dest = new wchar_t[BuffSize];
        std::unique_ptr<wchar_t> buffDelete(dest);

        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, BuffSize, text, std::strlen(text)));
        CHECK(std::wstring(L"Télécharger la Version d'Évaluation") == std::wstring(dest));
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, BuffSize, text, std::strlen(text), "utf-8"));
        CHECK(std::wstring(L"Télécharger la Version d'Évaluation") == std::wstring(dest));
        // not really windows-1252
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, BuffSize, text, std::strlen(text), "windows-1252"));
        CHECK(std::wstring(L"Télécharger la Version d'Évaluation") == std::wstring(dest));

        // without the BOM
        text = "TÃ©lÃ©charger la Version d'Ã‰valuation";
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, BuffSize, text, std::strlen(text)));
        CHECK(std::wstring(L"Télécharger la Version d'Évaluation") == std::wstring(dest));
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, BuffSize, text, std::strlen(text), "utf-8"));
        CHECK(std::wstring(L"Télécharger la Version d'Évaluation") == std::wstring(dest));
        // not really windows-1252
        CHECK(Wisteria::TextStream::CharStreamToUnicode(dest, BuffSize, text, std::strlen(text), "windows-1252"));
        CHECK(std::wstring(L"Télécharger la Version d'Évaluation") == std::wstring(dest));
        }

    SECTION("Simple")
        {
        const char* text = "Hello, world! Here is some simple text.";
        CHECK(wxString(text) == Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text)));
        }

    SECTION("Ansi")
        {
        const char* text = "Télécharger la Version d'Évaluation";
        // should figure it out
        CHECK(wxString(text, CONVERT_STR) == Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text)));
        CHECK(wxString(text, CONVERT_STR) == Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text), "windows-1252"));
        }

    SECTION("Broken Encoding")
        {
        const char* text = "Téélécharger la Version d'Évaluation";
        // not really utf-8, so "bogus" characters get skipped over
        CHECK(wxString(L"Tlcharger la Version d'valuation") == Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text), "utf-8"));
        text = "élécharger la Version d'Évaluation";
        // bad character at start of stream
        CHECK(wxString(L"lcharger la Version d'valuation") == Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text), "utf-8"));
        text = "Télécharger la Version d'Évaluationé";
        // bad character at end of stream
        CHECK(wxString(L"Tlcharger la Version d'valuation") == Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text), "utf-8"));
        // with BOM
        text = "ï»¿TélÃ©charger la Version d'Ã‰valuation";
        //bogus "é" will be stripped out
        CHECK(wxString(L"Tlécharger la Version d'Évaluation") == Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text), "utf-8"));
        CHECK(wxString(L"Tlécharger la Version d'Évaluation") == Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text)));
        }

    SECTION("UTF8 Encoding")
        {
        const char* text = "ï»¿TÃ©lÃ©charger la Version d'Ã‰valuation";
        CHECK(wxString(L"Télécharger la Version d'Évaluation") ==
            Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text)));
        CHECK(wxString(L"Télécharger la Version d'Évaluation") ==
            Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text), "utf-8"));
        // not really windows-1252
        CHECK(wxString(L"Télécharger la Version d'Évaluation") ==
            Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text), "windows-1252"));

        // without the BOM
        text = "TÃ©lÃ©charger la Version d'Ã‰valuation";
        CHECK(wxString(L"Télécharger la Version d'Évaluation") == Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text)));
        CHECK(wxString(L"Télécharger la Version d'Évaluation") == Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text), "utf-8"));
        // not really windows-1252
        CHECK(wxString(L"Télécharger la Version d'Évaluation") == Wisteria::TextStream::CharStreamToUnicode(text, std::strlen(text), "windows-1252"));
        }

    SECTION("Embedded Nulls")
        {
        const char* text = "Hello, world!\0\0\0 Here is\0 some simple\0 text0.";
        CHECK(wxString(L"Hello, world! Here is some simple text0.") == Wisteria::TextStream::CharStreamWithEmbeddedNullsToUnicode(text, 45));
        }
    }