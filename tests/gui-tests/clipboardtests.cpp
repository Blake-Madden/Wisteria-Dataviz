#include "../../src/util/clipboard_rtf.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace
    {
    wxString MakeSampleRtf()
        {
        return L"{\\rtf1\\ansi\\deff0 "
               L"H\\'e9llo \\u8212 ? \\u119070 ?"
               L"}";
        }

    std::vector<char> ToUtf8Bytes(const wxString& s)
        {
        const wxScopedCharBuffer b = s.utf8_str();
        return std::vector<char>(b.data(), b.data() + b.length());
        }
    } // namespace

TEST_CASE("wxRtfDataObject: format id is platform-correct", "[rtf][format]")
    {
    wxRtfDataObject obj{};

    const wxDataFormat fmt = obj.GetFormat();
    const wxString id = fmt.GetId();

#ifdef __WXMSW__
    CHECK(id == L"Rich Text Format");
#elif defined __WXOSX__
    CHECK(id == L"public.rtf");
#else
    CHECK(id == L"text/rtf");
#endif
    }

TEST_CASE("wxRtfDataObject: size reflects UTF-8 byte length", "[rtf][size]")
    {
    const wxString rtf = MakeSampleRtf();
    wxRtfDataObject obj{ rtf };

    const size_t expected = rtf.utf8_str().length();

    SECTION("GetDataSize()") { CHECK(obj.GetDataSize() == expected); }

    SECTION("GetDataSize(format passthrough)")
        {
        CHECK(obj.GetDataSize(obj.GetFormat()) == expected);
        }
    }

TEST_CASE("wxRtfDataObject: GetDataHere copies exact bytes", "[rtf][copy]")
    {
    const wxString rtf = MakeSampleRtf();
    wxRtfDataObject obj{ rtf };

    const std::vector<char> expected = ToUtf8Bytes(rtf);
    std::vector<char> buf(expected.size(), '\0');

    // We need these to proceed to memcmp; use REQUIRE here.
    REQUIRE(obj.GetDataHere(static_cast<void*>(buf.data())));
    REQUIRE(buf.size() == expected.size());

    CHECK(std::memcmp(buf.data(), expected.data(), expected.size()) == 0);
    }

TEST_CASE("wxRtfDataObject: GetDataHere nullptr is rejected", "[rtf][safety]")
    {
    wxRtfDataObject obj{ MakeSampleRtf() };

    CHECK_FALSE(obj.GetDataHere(nullptr));
    }

TEST_CASE("wxRtfDataObject: SetData stores bytes as text (UTF-8)", "[rtf][setdata]")
    {
    wxRtfDataObject obj{};

    const wxString rtfIn = MakeSampleRtf();
    const std::vector<char> bytes = ToUtf8Bytes(rtfIn);

    // We need SetData to succeed to continue the round-trip; REQUIRE.
    REQUIRE(obj.SetData(obj.GetFormat(), bytes.size(), static_cast<const void*>(bytes.data())));

    CHECK(obj.GetDataSize() == bytes.size());

    std::vector<char> out(bytes.size(), '\0');
    REQUIRE(obj.GetDataHere(out.data()));

    CHECK(std::memcmp(out.data(), bytes.data(), bytes.size()) == 0);
    }

TEST_CASE("wxRtfDataObject: SetData with nullptr or zero length is rejected", "[rtf][safety]")
    {
    wxRtfDataObject obj{ L"{\\rtf1 foo}" };

    SECTION("nullptr")
        {
        CHECK_FALSE(obj.SetData(obj.GetFormat(), 10, nullptr));
        CHECK(obj.GetDataSize() >= 0);
        }

    SECTION("zero length")
        {
        const char dummy = 0;
        CHECK_FALSE(obj.SetData(obj.GetFormat(), 0, &dummy));
        CHECK(obj.GetDataSize() >= 0);
        }
    }
