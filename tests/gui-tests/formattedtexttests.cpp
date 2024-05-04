#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../src/ui/controls/formattedtextctrl.h"

using namespace Wisteria::UI;

TEST_CASE("Formatted text control", "[controls][text-control]")
    {
     FormattedTextCtrl* m_textCtrl = new FormattedTextCtrl(wxTheApp->GetTopWindow());
    // "This is some Test text. THIS IS for something like testing a test cONtrol."
    m_textCtrl->SetFormattedText(L"{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0\\fswiss\\fcharset0 Arial;}{\\f1\\fswiss\\fprq2\\fcharset0 Berlin Sans FB;}}{\\colortbl ;\\red128\\green0\\blue128;\\red255\\green0\\blue255;}\\viewkind4\\uc1\\pard\\f0\\fs32 This \\fs20 is some Test \\b text\\b0 . THIS IS for \\i something\\i0  like \\cf1\\highlight2\\f1 testing\\highlight0  \\cf0\\f0 a test cONtrol.\\par}");
    
    SECTION("On Find Up Case Insensitive Partial Match")
        {
        long startOfSelection, endOfSelection;
        m_textCtrl->SetSelection(70, 70);
        wxFindDialogEvent event;
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFindString(L"TEST");
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 61);
        CHECK(endOfSelection == 65);
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 51);
        CHECK(endOfSelection == 55);
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 13);
        CHECK(endOfSelection == 17);
        }
    SECTION("On Find Up Case Sensitive Partial Match")
        {
        long startOfSelection, endOfSelection;
        m_textCtrl->SetSelection(70, 70);
        wxFindDialogEvent event;
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFlags(wxFR_MATCHCASE);
        event.SetFindString(L"test");
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 61);
        CHECK(endOfSelection == 65);
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 51);
        CHECK(endOfSelection == 55);
        }
    SECTION("On Find Up Case Insensitive Full Match")
        {
        long startOfSelection, endOfSelection;
        m_textCtrl->SetSelection(70,70);
        wxFindDialogEvent event;
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFlags(wxFR_WHOLEWORD);
        event.SetFindString(L"test");
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 61);
        CHECK(endOfSelection == 65);
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 13);
        CHECK(endOfSelection == 17);
        }
    SECTION("On Find Up Case Sensitive Full Match")
        {
        m_textCtrl->SetSelection(70, 70);
        wxFindDialogEvent event;
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFlags(wxFR_WHOLEWORD|wxFR_MATCHCASE);
        event.SetFindString(L"test");
        long startOfSelection, endOfSelection;
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 61);
        CHECK(endOfSelection == 65);
        }

    SECTION("On Find Down Case Insensitive Partial Match")
        {
        m_textCtrl->SetSelection(0, 0);
        wxFindDialogEvent event;
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFlags(wxFR_DOWN);
        event.SetFindString(L"TEST");
        m_textCtrl->OnFind(event);
        long startOfSelection, endOfSelection;
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 13);
        CHECK(endOfSelection == 17);
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 51);
        CHECK(endOfSelection == 55);
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 61);
        CHECK(endOfSelection == 65);
        }
    SECTION("On Find Down Case Sensitive Partial Match")
        {
        m_textCtrl->SetSelection(0, 0);
        wxFindDialogEvent event;
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFlags(wxFR_DOWN|wxFR_MATCHCASE);
        event.SetFindString(L"test");
        m_textCtrl->OnFind(event);
        long startOfSelection, endOfSelection;
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 51);
        CHECK(endOfSelection == 55);
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 61);
        CHECK(endOfSelection == 65);
        }
    SECTION("On Find Down Case Insensitive Full Match")
        {
        m_textCtrl->SetSelection(0, 0);
        wxFindDialogEvent event;
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFlags(wxFR_DOWN|wxFR_WHOLEWORD);
        event.SetFindString(L"test");
        m_textCtrl->OnFind(event);
        long startOfSelection, endOfSelection;
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 13);
        CHECK(endOfSelection == 17);
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 61);
        CHECK(endOfSelection == 65);
        }
    SECTION("On Find Down Case Sensitive Full Match")
        {
        m_textCtrl->SetSelection(0, 0);
        wxFindDialogEvent event;
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFlags(wxFR_DOWN|wxFR_WHOLEWORD|wxFR_MATCHCASE);
        event.SetFindString(L"test");
        long startOfSelection, endOfSelection;
        m_textCtrl->OnFind(event);
        m_textCtrl->GetSelection(&startOfSelection, &endOfSelection);
        CHECK(startOfSelection == 61);
        CHECK(endOfSelection == 65);
        }

    wxDELETE(m_textCtrl);
    }
