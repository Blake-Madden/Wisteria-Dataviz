#include "../../src/util/xml_format.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace Catch::Matchers;
using Catch::Matchers::WithinAbs;

static const wxString kTag = L"enabled";

[[nodiscard]]
static auto MakeDoc(const wxString& inner) -> wxString
    {
    wxString xml;
    xml << L"<" << kTag << L">" << inner << L"</" << kTag << L">";
    return xml;
    }

[[nodiscard]]
static wxString MakeSingle(const wxString& tag, const wxString& inner)
    {
    wxString xml;
    xml << L"<" << tag << L">" << inner << L"</" << tag << L">";
    return xml;
    }

[[nodiscard]]
static wxString MakeMany(const wxString& tag, std::initializer_list<wxString> inners)
    {
    wxString xml;
    for (const auto& s : inners)
        {
        xml << L"<" << tag << L">" << s << L"</" << tag << L">";
        }
    return xml;
    }

// Small helpers
[[nodiscard]]
static const wchar_t* Beg(const wxString& s)
    {
    return s.wc_str();
    }

[[nodiscard]]
static const wchar_t* End(const wxString& s)
    {
    return s.wc_str() + s.length();
    }

TEST_CASE("XML format", "[xml format]")
    {
    SECTION("Format Xml Color")
        {
        CHECK(XmlFormat::FormatColorAttributes(wxColor(100, 120, 150)) ==
              wxString(LR"( red="100" green="120" blue="150")"));
        CHECK(XmlFormat::FormatColorAttributeWithInclusionTag(wxColor(100, 120, 150), true) ==
              wxString(LR"( red="100" green="120" blue="150" include="1")"));
        CHECK(XmlFormat::FormatColorAttributeWithInclusionTag(wxColor(100, 120, 150), false) ==
              wxString(LR"( red="100" green="120" blue="150" include="0")"));

        std::wstring_view colStr{ LR"(<data red="100" green="120" blue="150" include="0">)" };
        wxColor retCol =
            XmlFormat::GetColor(colStr.data(), colStr.data() + colStr.length(), L"data", *wxBLACK);
        CHECK(retCol.Red() == 100);
        CHECK(retCol.Green() == 120);
        CHECK(retCol.Blue() == 150);

        bool inc;
        wxColor retCol2 = XmlFormat::GetColorWithInclusionTag(
            colStr.data(), colStr.data() + colStr.length(), L"data", inc, *wxBLACK, false);
        CHECK(retCol2.Red() == 100);
        CHECK(retCol2.Green() == 120);
        CHECK(retCol2.Blue() == 150);
        }

    SECTION("Format Xml Font")
        {
        CHECK(
            XmlFormat::FormatFontAttributes(
                wxFontInfo(12).Bold(true).Italic(true).Underlined(true).FaceName(L"Arial")) ==
            wxString(
                LR"( font-point-size="12" font-style="93" font-weight="700" font-underline="1" font-face-name="Arial")"));

        std::wstring fontStr{
            LR"(<data font-point-size="12" font-style="93" font-weight="700" font-underline="1" font-face-name="Arial">)"
        };
        wxFont retFont =
            XmlFormat::GetFont(fontStr.data(), fontStr.data() + fontStr.length(), L"data");
        CHECK((int)retFont.GetPointSize() == 12);
        CHECK((int)retFont.GetStyle() == 93);
        CHECK((int)retFont.GetWeight() == 700);
        CHECK((int)retFont.GetUnderlined() == 1);
        }

    SECTION("Format Xml Section With Attribute")
        {
        wxString bufferText;
        bufferText += XmlFormat::FormatSectionWithAttribute(L"path", L"My File & Other Stúff.txt",
                                                            L"description", L"Other Stúff", 2);
        bufferText += XmlFormat::FormatSectionWithAttribute(
            L"path", L"File   with      spaces.txt", L"description", L"Lots    of   spaces", 2);
        bufferText +=
            XmlFormat::FormatSectionWithAttribute(L"path", L"Weird 'characters\"&@;1?<>.txt",
                                                  L"description", L"Weird chars '\"&@;1?<>", 2);
        bufferText += XmlFormat::FormatSectionWithAttribute(
            L"path", L"サーバコンピュータで構成され.txt", L"description",
            L"Japanese text: サーバコンピュータで構成され", 2);
        bufferText += XmlFormat::FormatSectionWithAttribute(
            L"path",
            L"   "
            L"abcdefghijklmnopqrstuvwxyz0123456789`~!@#$%^&*()_-+=[]{}|\\:;\"'<,>.?/"
            L"ÇüéƒäàåçêëïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒá¬.txt",
            L"description", L"ÇüéƒäàåçêëïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒá¬", 2);
        bufferText += XmlFormat::FormatSectionWithAttribute(
            L"path", L"//somewebsite\\%32%index.html", L"description",
            L"//somewebsite\\%32%index.html", 2);
        std::vector<comparable_first_pair<wxString, wxString>> strings;
        XmlFormat::GetStringsWithExtraInfo(bufferText.wc_str(),
                                           bufferText.wc_str() + bufferText.length(), L"path",
                                           "description", strings);

        CHECK(wxString(L"My File & Other Stúff.txt") == strings[0].first);
        CHECK(wxString(L"Other Stúff") == strings[0].second);

        CHECK(wxString(L"File   with      spaces.txt") == strings[1].first);
        CHECK(wxString(L"Lots    of   spaces") == strings[1].second);

        CHECK(wxString(L"Weird 'characters\"&@;1?<>.txt") == strings[2].first);
        CHECK(wxString(L"Weird chars '\"&@;1?<>") == strings[2].second);

        CHECK(wxString(L"サーバコンピュータで構成され.txt") == strings[3].first);
        CHECK(wxString(L"Japanese text: サーバコンピュータで構成され") == strings[3].second);

        CHECK(wxString(L"   "
                       L"abcdefghijklmnopqrstuvwxyz0123456789`~!@#$%^&*()_-+=[]{}|\\:;\"'<,>.?/"
                       L"ÇüéƒäàåçêëïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒá¬.txt") == strings[4].first);
        CHECK(wxString(L"ÇüéƒäàåçêëïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒá¬") == strings[4].second);

        CHECK(wxString(L"//somewebsite\\%32%index.html") == strings[5].first);
        CHECK(wxString(L"//somewebsite\\%32%index.html") == strings[5].second);
        }
    }

TEST_CASE("XmlFormat::GetBoolean basic true/false", "[xml][boolean]")
    {
        // true case: <enabled>1</enabled>
        {
        const wxString xml = MakeDoc(L"1");
        const wchar_t* start = xml.wc_str();
        const wchar_t* end = start + xml.length();
        CHECK(XmlFormat::GetBoolean(start, end, kTag, /*default*/ false) == true);
        }

        // false case: <enabled>0</enabled>
        {
        const wxString xml = MakeDoc(L"0");
        const wchar_t* start = xml.wc_str();
        const wchar_t* end = start + xml.length();
        CHECK(XmlFormat::GetBoolean(start, end, kTag, /*default*/ true) == false);
        }
    }

TEST_CASE("XmlFormat::GetBoolean non-numeric content treated as false", "[xml][boolean]")
    {
    const wxString xml = MakeDoc(L"true"); // not "1"
    const wchar_t* start = xml.wc_str();
    const wchar_t* end = start + xml.length();
    CHECK(XmlFormat::GetBoolean(start, end, kTag, /*default*/ true) == false);
    }

TEST_CASE("XmlFormat::GetBoolean empty tag returns default", "[xml][boolean]")
    {
    const wxString xml = MakeDoc(L"");
    const wchar_t* start = xml.wc_str();
    const wchar_t* end = start + xml.length();

    CHECK(XmlFormat::GetBoolean(start, end, kTag, /*default*/ false) == false);
    CHECK(XmlFormat::GetBoolean(start, end, kTag, /*default*/ true) == true);
    }

TEST_CASE("XmlFormat::GetBoolean missing tag returns default", "[xml][boolean]")
    {
    const wxString xml = L"<other>1</other>";
    const wchar_t* start = xml.wc_str();
    const wchar_t* end = start + xml.length();

    CHECK(XmlFormat::GetBoolean(start, end, kTag, /*default*/ false) == false);
    CHECK(XmlFormat::GetBoolean(start, end, kTag, /*default*/ true) == true);
    }

// ------------------------ GetString ------------------------

TEST_CASE("XmlFormat::GetString returns inner text for one tag", "[xml][getstring]")
    {
    const wxString tag = L"name";
    const wxString xml = MakeSingle(tag, L"Alice");

    const wchar_t* start = xml.wc_str();
    const wchar_t* end = start + xml.length();

    const wxString out = XmlFormat::GetString(start, end, tag, /*default*/ wxString{});
    CHECK(out == L"Alice");
    }

TEST_CASE("XmlFormat::GetString returns default when tag missing", "[xml][getstring]")
    {
    const wxString xml = L"<other>Bob</other>";

    const wchar_t* start = xml.wc_str();
    const wchar_t* end = start + xml.length();

    CHECK(XmlFormat::GetString(start, end, L"name", /*default*/ L"default") == L"default");
    }

TEST_CASE("XmlFormat::GetString respects empty content", "[xml][getstring]")
    {
    const wxString tag = L"empty";
    const wxString xml = MakeSingle(tag, L"");

    const wchar_t* start = xml.wc_str();
    const wchar_t* end = start + xml.length();

    CHECK(XmlFormat::GetString(start, end, tag, /*default*/ L"default") == L"");
    }

// ------------------------ GetStrings ------------------------

TEST_CASE("XmlFormat::GetStrings collects multiple occurrences", "[xml][getstrings]")
    {
    const wxString tag = L"item";
    const wxString xml = MakeMany(tag, { L"red", L"green", L"blue" });

    const wchar_t* start = xml.wc_str();
    const wchar_t* end = start + xml.length();

    std::vector<wxString> out;
    XmlFormat::GetStrings(start, end, tag, out);

    CHECK(out.size() == 3);
    CHECK(out[0] == L"red");
    CHECK(out[1] == L"green");
    CHECK(out[2] == L"blue");
    }

TEST_CASE("XmlFormat::GetStrings clears output and returns empty when tag missing",
          "[xml][getstrings]")
    {
    const wxString xml = L"<other>x</other>";

    const wchar_t* start = xml.wc_str();
    const wchar_t* end = start + xml.length();

    std::vector<wxString> out{ L"junk", L"data" }; // ensure it gets cleared
    XmlFormat::GetStrings(start, end, L"name", out);

    CHECK(out.empty());
    }

TEST_CASE("XmlFormat::GetStrings handles empty inner text entries", "[xml][getstrings]")
    {
    const wxString tag = L"opt";
    const wxString xml = MakeMany(tag, { L"", L"A", L"" });

    const wchar_t* start = xml.wc_str();
    const wchar_t* end = start + xml.length();

    std::vector<wxString> out;
    XmlFormat::GetStrings(start, end, tag, out);

    CHECK(out.size() == 3);
    CHECK(out[0] == L"");
    CHECK(out[1] == L"A");
    CHECK(out[2] == L"");
    }

constexpr static double kEps = 1e-9;

// ------------------------ GetLong ------------------------

TEST_CASE("XmlFormat::GetLong parses integer content", "[xml][getlong]")
    {
    const wxString xml = L"<count>42</count>";
    CHECK(XmlFormat::GetLong(Beg(xml), End(xml), L"count", /*default*/ -1) == 42);
    }

TEST_CASE("XmlFormat::GetLong returns default when tag missing", "[xml][getlong]")
    {
    const wxString xml = L"<other>5</other>";
    CHECK(XmlFormat::GetLong(Beg(xml), End(xml), L"count", /*default*/ 123) == 123);
    }

TEST_CASE("XmlFormat::GetLong handles negative and spaced numbers", "[xml][getlong]")
    {
    const wxString xml = L"<delta>   -17  </delta>";
    CHECK(XmlFormat::GetLong(Beg(xml), End(xml), L"delta", /*default*/ 0) == -17);
    }

// ------------------------ GetDouble (no Approx; use WithinAbs) ------------------------

TEST_CASE("XmlFormat::GetDouble parses decimal content", "[xml][getdouble]")
    {
    const wxString xml = L"<ratio>3.14159</ratio>";
    const double v = XmlFormat::GetDouble(Beg(xml), End(xml), L"ratio", /*default*/ 0.0);
    CHECK_THAT(v, WithinAbs(3.14159, kEps));
    }

TEST_CASE("XmlFormat::GetDouble supports scientific notation", "[xml][getdouble]")
    {
    const wxString xml = L"<avogadro>6.02e23</avogadro>";
    const double v = XmlFormat::GetDouble(Beg(xml), End(xml), L"avogadro", /*default*/ -1.0);
    CHECK_THAT(v, WithinAbs(6.02e23, 6.02e23 * 1e-12)); // looser abs tol for large mags
    }

TEST_CASE("XmlFormat::GetDouble returns default when tag missing", "[xml][getdouble]")
    {
    const wxString xml = L"<other>1.0</other>";
    const double v = XmlFormat::GetDouble(Beg(xml), End(xml), L"ratio", /*default*/ 9.9);
    CHECK_THAT(v, WithinAbs(9.9, kEps));
    }

// ------------------------ GetAttributeString ------------------------

TEST_CASE("XmlFormat::GetAttributeString extracts quoted attribute value", "[xml][getattr][string]")
    {
    const wxString xml = L"<item name=\"Widget\" size=\"10\"/>";
    const wxString out = XmlFormat::GetAttributeString(Beg(xml), End(xml), L"name");
    CHECK(out == L"Widget");
    }

TEST_CASE("XmlFormat::GetAttributeString returns empty when attribute missing",
          "[xml][getattr][string]")
    {
    const wxString xml = L"<item size=\"10\"/>";
    const wxString out = XmlFormat::GetAttributeString(Beg(xml), End(xml), L"name");
    CHECK(out.empty());
    }

TEST_CASE("XmlFormat::GetAttributeString returns empty when quotes missing",
          "[xml][getattr][string]")
    {
    const wxString xml = L"<item name=Widget />";
    const wxString out = XmlFormat::GetAttributeString(Beg(xml), End(xml), L"name");
    CHECK(out.empty());
    }

// ------------------------ GetAttributeLongValue (attribute-only overload) ------------------------

TEST_CASE("XmlFormat::GetAttributeLongValue extracts integer attribute", "[xml][getattr][long]")
    {
    const wxString xml = L"<item size=\"123\"/>";
    CHECK(XmlFormat::GetAttributeLongValue(Beg(xml), End(xml), L"size") == 123);
    }

TEST_CASE("XmlFormat::GetAttributeLongValue returns 0 when attribute missing",
          "[xml][getattr][long]")
    {
    const wxString xml = L"<item />";
    CHECK(XmlFormat::GetAttributeLongValue(Beg(xml), End(xml), L"size") == 0);
    }

// ------------------------ GetAttributeDoubleValue (entity + attribute + default)
// ------------------------

TEST_CASE("XmlFormat::GetAttributeDoubleValue finds attribute on an element",
          "[xml][getattr][double]")
    {
    const wxString xml = L"<point x=\"1.5\" y=\"-2.25\"/>";
        {
        const double vx =
            XmlFormat::GetAttributeDoubleValue(Beg(xml), End(xml), L"point", L"x", /*default*/ 0.0);
        CHECK_THAT(vx, WithinAbs(1.5, kEps));
        }
        {
        const double vy =
            XmlFormat::GetAttributeDoubleValue(Beg(xml), End(xml), L"point", L"y", /*default*/ 0.0);
        CHECK_THAT(vy, WithinAbs(-2.25, kEps));
        }
    }

TEST_CASE("XmlFormat::GetAttributeDoubleValue returns default when attribute missing",
          "[xml][getattr][double]")
    {
    const wxString xml = L"<point x=\"1.5\"/>";
    const double v =
        XmlFormat::GetAttributeDoubleValue(Beg(xml), End(xml), L"point", L"y", /*default*/ 9.9);
    CHECK_THAT(v, WithinAbs(9.9, kEps));
    }

TEST_CASE("XmlFormat::GetAttributeDoubleValue returns default when element missing",
          "[xml][getattr][double]")
    {
    const wxString xml = L"<other z=\"2.5\"/>";
    const double v =
        XmlFormat::GetAttributeDoubleValue(Beg(xml), End(xml), L"point", L"z", /*default*/ -1.0);
    CHECK_THAT(v, WithinAbs(-1.0, kEps));
    }

TEST_CASE("XmlFormat::GetAttributeDoubleValue returns default when value not a number",
          "[xml][getattr][double]")
    {
    const wxString xml = L"<point x=\"NaNish\"/>";
    const double v =
        XmlFormat::GetAttributeDoubleValue(Beg(xml), End(xml), L"point", L"x", /*default*/ 123.456);
    CHECK_THAT(v, WithinAbs(123.456, kEps));
    }
