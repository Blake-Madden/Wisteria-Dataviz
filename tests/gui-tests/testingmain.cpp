#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/catch_session.hpp>
#include <wx/wx.h>
#include "testableframe.h"

// Main entry point for test harness
//----------------------------------

using FilterEventFunc = int(*)(wxEvent&);
using ProcessEventFunc = bool(*)(wxEvent&);

// The application class
class TestApp final : public wxApp
    {
public:
    // standard overrides
    bool OnInit() final;
    int  OnExit() final;

    // Also override this method to avoid showing any dialogs from here -- and
    // show some details about the exception along the way.
    bool OnExceptionInMainLoop() final
        {
        wxFprintf(stderr, wxASCII_STR("Unhandled exception in the main loop: %s\n"),
                  wxASCII_STR(Catch::translateActiveException().c_str()));

        throw;
        }

    // used by events propagation test
    int FilterEvent(wxEvent& event) final;
    bool ProcessEvent(wxEvent& event) final;

    void SetFilterEventFunc(FilterEventFunc f) { m_filterEventFunc = f; }
    void SetProcessEventFunc(ProcessEventFunc f) { m_processEventFunc = f; }

    // In a console application we could run the tests directly from
    // OnRun(), but in for a GUI test runner we run them when we get the first call to
    // our EVT_IDLE handler to ensure that we do everything from inside the
    // main event loop. This is especially important under wxOSX/Cocoa where
    // the main event loop is different from the others but it's also safer to
    // do it like this in the other ports as we test the GUI code in the same
    // context as it's used usually, in normal programs, and it might behave
    // differently without the event loop.
    void OnIdle(wxIdleEvent& event)
        {
        if ( m_runTests )
            {
            m_runTests = false;

#ifdef __WXOSX__
            // we need to wait until the window is activated and fully ready
            // otherwise no events can be posted
            wxEventLoopBase* const loop = wxEventLoop::GetActive();
            if ( loop )
            {
                loop->DispatchTimeout(1000);
                loop->Yield();
            }
#endif // __WXOSX__

            m_exitcode = RunTests();
            ExitMainLoop();
            }

        event.Skip();
        }

    int OnRun() final
        {
        if ( wxApp::OnRun() != 0 )
            m_exitcode = EXIT_FAILURE;

        return m_exitcode;
        }
private:
    int RunTests();

    // flag telling us whether we should run tests from our EVT_IDLE handler
    bool m_runTests{ true };

    // event handling hooks
    FilterEventFunc m_filterEventFunc{ nullptr };
    ProcessEventFunc m_processEventFunc{ nullptr };

    int m_exitcode{ EXIT_SUCCESS };
    };

wxIMPLEMENT_APP(TestApp);

// ----------------------------------------------------------------------------
// TestApp
// ----------------------------------------------------------------------------
bool TestApp::OnInit()
    {
    wxInitAllImageHandlers();
    // Hack: don't call wxApp::OnInit() to let CATCH handle command line.

    // create a parent window to be used as parent for the GUI controls
    new wxTestableFrame();

    Connect(wxEVT_IDLE, wxIdleEventHandler(TestApp::OnIdle));

    return true;
    }

// Event handling
//--------------------------------------------
int TestApp::FilterEvent(wxEvent& event)
    {
    if (m_filterEventFunc)
        return (*m_filterEventFunc)(event);

    return wxApp::FilterEvent(event);
    }

//--------------------------------------------
bool TestApp::ProcessEvent(wxEvent& event)
    {
    if (m_processEventFunc)
        return (*m_processEventFunc)(event);

    return wxApp::ProcessEvent(event);
    }

//--------------------------------------------
int TestApp::RunTests()
    {
    wxLogNull nl;
    Catch::Session session;
    // Cast is needed under MSW where Catch also provides an overload taking
    // wchar_t, but as it simply converts arguments to char internally anyhow,
    // we can just as well always use the char version.
    session.applyCommandLine(argc, static_cast<char**>(argv));

    // If no command line options for reporting, then change it to output to JUnit format
    // (the default would be the console window, which we won't have).
    if (session.configData().reporterSpecifications.size() == 0)
        {
        session.configData().reporterSpecifications.push_back(
            { "junit", Catch::Optional<std::string>("WisteriaTestResults.xml"),
               Catch::ColourMode::PlatformDefault, std::map<std::string, std::string>{} });
        }

    return session.run();
    }

//--------------------------------------------
int TestApp::OnExit()
    {
    delete GetTopWindow();

    return wxApp::OnExit();
    }
