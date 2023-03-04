#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/import/idl_extract_text.h"

using namespace Catch::Matchers;
using namespace lily_of_the_valley;

TEST_CASE("IDL Parser", "[idl import]")
    {
    SECTION("Null")
        {
        idl_extract_text filter_idl;
        const wchar_t* p = filter_idl({ L"hello", 0 });
        CHECK(p == nullptr);

        CHECK(filter_idl.get_filtered_text_length() == 0);
        CHECK(filter_idl({ L"/*Interface attributes go here. */helpstring(\"do something\")", 0 }) == nullptr);
        CHECK(filter_idl.get_filtered_text_length() == 0);
        }
    SECTION("Strings")
        {
        idl_extract_text filter_idl;
        std::wstring value = L"[\nuuid(1e196b20-1f3c-1069-996b-00dd010fe676),\nhelpstring(\"Lines 1.0 Type Library\"),\nversion(1.0)\n]\n"
            "library Lines\n{\n[\nuuid(1e123456-1f3c-1069-996b-00dd010fe676),\nhelpstring(\"Line object.\"),\noleautomation,\ndual\n]\n"
            "interface ILine : IDispatch\n{\n[propget, helpstring(\"Returns and sets RGB color.\")]\nHRESULT Color([out, retval] long* ReturnVal);\n"
            "[propput, helpstring(\"Returns and sets RGB color.\")]\nHRESULT Color([in] long rgb);\n}\n};";
        const wchar_t* p = filter_idl({ value.c_str(), value.length() });
        CHECK(std::wcscmp(p, L"Lines 1.0 Type Library\n\nLine object.\n\nReturns and sets RGB color.\n\nReturns and sets RGB color.\n\n") == 0);
        }
    SECTION("Simple")
        {
        const wchar_t* text = L"helpstring(\"function\")/*Interface attributes go here. */helpstring(\"do something\")";
        idl_extract_text ext;
        const wchar_t* output = ext({ text });
        CHECK(std::wcscmp(output, L"function\n\ndo something\n\n") == 0);
        CHECK(ext.get_filtered_text_length() == 24);
        }
    SECTION("Bouds check")
        {
        const wchar_t* text = L"helpstring(\"";
        idl_extract_text ext;
        const wchar_t* output = ext({ text });
        CHECK(std::wcscmp(output, L"") == 0);
        CHECK(ext.get_filtered_text_length() == 0);
        }
    }
