#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/util/xml_format.h

using namespace Catch::Matchers;

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
        }

    SECTION("Format Xml Font")
        {
        CHECK(XmlFormat::FormatFontAttributes(wxFontInfo(12).Bold(true).Italic(true).Underlined(true).FaceName(L"Arial")) ==
            wxString(LR"( font-point-size="12" font-style="93" font-weight="700" font-underline="1" font-face-name="Arial")"));
        }

    SECTION("Format Xml Section With Attribute")
        {
        wxString bufferText;
        bufferText += XmlFormat::FormatSectionWithAttribute(
                                                            L"path",
                                                            L"My File & Other Stúff.txt",
                                                            L"description",
                                                            L"Other Stúff", 2);
        bufferText += XmlFormat::FormatSectionWithAttribute(
                                                            L"path",
                                                            L"File   with      spaces.txt",
                                                            L"description",
                                                            L"Lots    of   spaces", 2);
        bufferText += XmlFormat::FormatSectionWithAttribute(
                                                            L"path",
                                                            L"Weird 'characters\"&@;1?<>.txt",
                                                            L"description",
                                                            L"Weird chars '\"&@;1?<>", 2);
        bufferText += XmlFormat::FormatSectionWithAttribute(
                                                            L"path",
                                                            L"サーバコンピュータで構成され.txt",
                                                            L"description",
                                                            L"Japanese text: サーバコンピュータで構成され", 2);
        bufferText += XmlFormat::FormatSectionWithAttribute(
                                                            L"path",
                                                            L"   abcdefghijklmnopqrstuvwxyz0123456789`~!@#$%^&*()_-+=[]{}|\\:;\"'<,>.?/ÇüéƒäàåçêëïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒá¬.txt",
                                                            L"description",
                                                            L"ÇüéƒäàåçêëïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒá¬", 2);
        bufferText += XmlFormat::FormatSectionWithAttribute(
                                                            L"path",
                                                            L"//somewebsite\\%32%index.html",
                                                            L"description",
                                                            L"//somewebsite\\%32%index.html", 2);
        std::vector<comparable_first_pair<wxString,wxString>> strings;
        XmlFormat::GetStringsWithExtraInfo(bufferText.wc_str(), bufferText.wc_str()+bufferText.length(),
            L"path", "description", strings);

        CHECK(wxString(L"My File & Other Stúff.txt") == strings[0].first);
        CHECK(wxString(L"Other Stúff") == strings[0].second);

        CHECK(wxString(L"File   with      spaces.txt") == strings[1].first);
        CHECK(wxString(L"Lots    of   spaces") == strings[1].second);

        CHECK(wxString(L"Weird 'characters\"&@;1?<>.txt") == strings[2].first);
        CHECK(wxString(L"Weird chars '\"&@;1?<>") == strings[2].second);

        CHECK(wxString(L"サーバコンピュータで構成され.txt") == strings[3].first);
        CHECK(wxString(L"Japanese text: サーバコンピュータで構成され") == strings[3].second);

        CHECK(wxString(L"   abcdefghijklmnopqrstuvwxyz0123456789`~!@#$%^&*()_-+=[]{}|\\:;\"'<,>.?/ÇüéƒäàåçêëïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒá¬.txt") == strings[4].first);
        CHECK(wxString(L"ÇüéƒäàåçêëïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒá¬") == strings[4].second);

        CHECK(wxString(L"//somewebsite\\%32%index.html") == strings[5].first);
        CHECK(wxString(L"//somewebsite\\%32%index.html") == strings[5].second);
        }
    }
