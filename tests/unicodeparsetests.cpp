#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/unicode_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("Unicode Parser", "[unicode import]")
    {
    SECTION("Little Endian")
        {
        char buffer[] = { -1, -2, 0x54, 0, -23, 0, 0x6c, 0, -23, 0, 0x63, 0, 0x68, 0, 0x61, 0, 0x72, 0, 0x67, 0, 0x65, 0, 0x72, 0, 0x20, 0, 0x6c, 0, 0x61, 0, 0x20, 0, 0x56, 0, 0x65, 0, 0x72, 0, 0x73, 0, 0x69, 0, 0x6f, 0, 0x6e, 0, 0x20, 0, 0x64, 0, 0x27, 0, -55, 0, 0x76, 0, 0x61, 0, 0x6c, 0, 0x75, 0, 0x61, 0, 0x74, 0, 0x69, 0, 0x6f, 0, 0x6e, 0, 0, 0 };
        unicode_extract_text ext;
        const wchar_t* out = ext(buffer, 72);
        CHECK(std::wcscmp(out, L"Télécharger la Version d'Évaluation") == 0);
        CHECK(ext.get_filtered_text_length() == 35);
        }
    SECTION("Big Endian")
        {
        char buffer[] = { -2, -1, 0, 0x54, 0, -23, 0, 0x6c, 0, -23, 0, 0x63, 0, 0x68, 0, 0x61, 0, 0x72, 0, 0x67, 0, 0x65, 0, 0x72, 0, 0x20, 0, 0x6c, 0, 0x61, 0, 0x20, 0, 0x56, 0, 0x65, 0, 0x72, 0, 0x73, 0, 0x69, 0, 0x6f, 0, 0x6e, 0, 0x20, 0, 0x64, 0, 0x27, 0, -55, 0, 0x76, 0, 0x61, 0, 0x6c, 0, 0x75, 0, 0x61, 0, 0x74, 0, 0x69, 0, 0x6f, 0, 0x6e, 0, 0 };
        unicode_extract_text ext;
        const wchar_t* out = ext(buffer, 72);
        CHECK(std::wcscmp(out, L"Télécharger la Version d'Évaluation") == 0);
        CHECK(ext.get_filtered_text_length() == 35);
        }
    SECTION("Not Unicode")
        {
        char buffer[] = "Télécharger la Version d'Évaluation";
        unicode_extract_text ext;
        CHECK(ext(buffer, 72) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        }
    SECTION("Null")
        {
        char buffer[] = { -1, -2, 0x54, 0, -23, 0, 0x6c, 0, -23, 0, 0x63, 0, 0x68, 0, 0x61, 0, 0x72, 0, 0x67, 0, 0x65, 0, 0x72, 0, 0x20, 0, 0x6c, 0, 0x61, 0, 0x20, 0, 0x56, 0, 0x65, 0, 0x72, 0, 0x73, 0, 0x69, 0, 0x6f, 0, 0x6e, 0, 0x20, 0, 0x64, 0, 0x27, 0, -55, 0, 0x76, 0, 0x61, 0, 0x6c, 0, 0x75, 0, 0x61, 0, 0x74, 0, 0x69, 0, 0x6f, 0, 0x6e, 0, 0, 0 };
        unicode_extract_text ext;
        CHECK(ext(nullptr, 72) == nullptr);
        CHECK(ext(buffer, 0) == nullptr);
        CHECK(ext.get_filtered_text_length() == 0);
        }
    }
