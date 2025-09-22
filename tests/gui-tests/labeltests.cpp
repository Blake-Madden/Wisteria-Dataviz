#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../src/base/label.h"

using namespace Wisteria::GraphItems;

TEST_CASE("Split text to fit length", "[label]")
    {
    Label lbl;
    lbl.SetText(L"High School Sophomore");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"High School\nSophomore");

    lbl.SetText(L"High School: Sophomore");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"High School:\nSophomore");
    // trim padding
    lbl.SetText(L"High School Sophomore ");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"High School\nSophomore");
    // no delimiters
    lbl.SetText(L"HighSchoolSophomore");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"HighSchoolSophomore");
    // delimiter at end only
    lbl.SetText(L"HighSchoolSophomore ");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"HighSchoolSophomore");
    // string not long enough
    lbl.SetText(L"High School Sophomore");
    lbl.SplitTextToFitLength(30);
    CHECK(lbl.GetText() == L"High School Sophomore");
    // dumb suggested length
    lbl.SetText(L"High School Sophomore");
    lbl.SplitTextToFitLength(0);
    CHECK(lbl.GetText() == L"High\nSchool\nSophomore");
    // lot of delimiting
    lbl.SetText(L"High School Sophomore");
    lbl.SplitTextToFitLength(4);
    CHECK(lbl.GetText() == L"High\nSchool\nSophomore");
    }

TEST_CASE("Split text to fit length with new lines", "[label]")
    {
    Label lbl;
    lbl.SetText(L"High School-Junior\nhigh");
    lbl.SplitTextToFitLength(10);
    CHECK(lbl.GetText() == L"High School-\nJunior high");
    }

TEST_CASE("Label Fonts", "[label]")
    {
    SECTION("Fix Font OSX Bad Fonts")
        {
        wxLogNull ln;
        wxFont ft(wxFontInfo().FaceName(".Lucida Grande UI"));
        Label::FixFont(ft);
        CHECK(ft.IsOk());
        CHECK(ft.GetPointSize() > 0);
    #ifdef __WXOSX__
        CHECK(wxString("Lucida Grande") == ft.GetFaceName());
    #else
        CHECK(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName() == ft.GetFaceName());
    #endif
        wxFont ft2(wxFontInfo().FaceName(".Helvetica Neue DeskInterface"));
        Label::FixFont(ft2);
        CHECK(ft2.IsOk());
        CHECK(ft2.GetPointSize() > 0);
    #ifdef __WXOSX__
        CHECK(wxString("Helvetica Neue") == ft2.GetFaceName());
    #else
        CHECK(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName() == ft2.GetFaceName());
    #endif
        }
    SECTION("Fix Font Bad Sizes")
        {
        wxLogNull ln;
        wxFont ft(wxFontInfo(0).FaceName("Arial"));
        Label::FixFont(ft);
        CHECK(ft.IsOk());
        CHECK(ft.GetPointSize() > 0);
        CHECK(ft.GetFaceName().length());

        wxFont ft2(wxFontInfo(0).FaceName("Arial"));
        Label::FixFont(ft2);
        CHECK(ft2.IsOk());
        CHECK(ft2.GetPointSize() > 0);
        CHECK(ft2.GetFaceName().length());

        wxFont ft3(wxFontInfo(3).FaceName("Arial"));
        Label::FixFont(ft3);
        CHECK(ft3.IsOk());
        CHECK(ft3.GetPointSize() > 3);
        CHECK(ft3.GetFaceName().length());
        }
    SECTION("Fix Font Bad Font Name")
        {
        wxLogNull ln;
        wxFont ft(wxFontInfo().FaceName("Lucida HUGE"));
        Label::FixFont(ft);
        CHECK(ft.IsOk());
        CHECK(ft.GetPointSize() > 0);
        // macOS's default font facename will be a mapping value,
        // not a real facename
#ifndef __WXOSX__
        CHECK(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName() == ft.GetFaceName());
#endif
        }
    SECTION("Fix Font Nothing Wrong")
        {
        wxLogNull ln;
        wxFont ft(wxFontInfo().FaceName("Arial"));
        Label::FixFont(ft);
        CHECK(ft.IsOk());
        CHECK(ft.GetPointSize() > 0);
        CHECK(ft.GetFaceName().length());
        }
    }
