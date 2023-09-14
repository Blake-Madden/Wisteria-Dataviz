/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __FORMATTEDTEXTCTRL_H__
#define __FORMATTEDTEXTCTRL_H__

#include <wx/wx.h>
#if defined(__WXMSW__)
    #include <wx/msw/dc.h>
    #include <wx/msw/private.h>
    #include <richedit.h>
#endif
#include <wx/textctrl.h>
#include <wx/fdrepdlg.h>
#include <wx/strconv.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/dcprint.h>
#include <wx/dcps.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/html/htmprint.h>
#include <wx/datetime.h>
#include <wx/busyinfo.h>
#include <wx/clipbrd.h>
#include <wx/xrc/xmlres.h>
#include <memory>
#include <vector>
#include <string>
#include "../../import/rtf_extract_text.h"
#include "../../i18n-check/src/donttranslate.h"
#include "../../util/clipboard_rtf.h"
#include "../../base/canvas.h"

/// @private
static constexpr int TWIPS_PER_INCH = 1440;

/// @brief A text control that shows formatted (RTF or Pango) content.
/// @details Native RTF/Pango markup can be fed directly into the control, as well
///     as retrieved for easy exporting. Native printing support is also built in.
class FormattedTextCtrl final : public wxTextCtrl
    {
    FormattedTextCtrl() = default; // needed for WX macros
public:
    /** @brief Constructor.
        @param parent The parent window.
        @param id The control's ID.
        @param pos The control's position.
        @param size The control's size.
        @param style The control's style.
        @param validator A validator to connect to the control.*/
    explicit FormattedTextCtrl(wxWindow* parent, wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = 0,
        const wxValidator& validator = wxDefaultValidator) :
            wxTextCtrl(parent, id, wxString{}, pos, size,
                style|wxTE_RICH2|wxTE_MULTILINE|wxTE_WORDWRAP|wxTE_NOHIDESEL,
                validator, L"FormattedTextCtrl"),
                // default paper size is 8.5" x 11"
                m_paperSize(wxSize(8.5 * TWIPS_PER_INCH,
                                   11 * TWIPS_PER_INCH))
        {
        Bind(wxEVT_CONTEXT_MENU, &FormattedTextCtrl::OnContextMenu, this);
        Bind(wxEVT_FIND, &FormattedTextCtrl::OnFind, this);
        Bind(wxEVT_FIND_NEXT, &FormattedTextCtrl::OnFind, this);
        Bind(wxEVT_FIND_CLOSE, &FormattedTextCtrl::OnFind, this);
        Bind(wxEVT_MENU, &FormattedTextCtrl::OnSave, this, wxID_SAVE);
        Bind(wxEVT_MENU, &FormattedTextCtrl::OnPreview, this, wxID_PREVIEW);
        Bind(wxEVT_MENU, &FormattedTextCtrl::OnPrint, this, wxID_PRINT);
        Bind(wxEVT_MENU, &FormattedTextCtrl::OnSelectAll, this, wxID_SELECTALL);
        Bind(wxEVT_MENU, &FormattedTextCtrl::OnCopyAll, this, XRCID("ID_COPY_ALL"));
        }
    /// @private
    FormattedTextCtrl(const FormattedTextCtrl&) = delete;
    /// @private
    FormattedTextCtrl& operator=(const FormattedTextCtrl&) = delete;
    /// @private
    ~FormattedTextCtrl()
        { wxDELETE(m_menu); }

    /// @brief Sets the content to be used when exporting or printing.
    /// @details This is useful for when the control is themed and you
    ///     don't want to include the background color when exporting.
    /// @param text The formatted text to export (designed for white backgrounds).
    void SetUnthemedFormattedText(const wchar_t* text)
        { m_unthemedContent = text; }
    /// @returns The window's content as unthemed RTF.\n
    ///     (This is RFT meant for white background and black text.)
    /// @param fixHighlightingTags Whether or not to include the "addChcbpatTag" tag,
    ///     which is used to highlight the background of text.\n
    ///     This is necessary for some programs that don't support the "highlight" or "cb"
    ///     command (e.g., LibreOffice).
    [[nodiscard]]
    wxString GetUnthemedFormattedTextRtf([[maybe_unused]] const bool fixHighlightingTags = true);
    /// @returns The window's content as HTML.
    ///     (This is HTML meant for white background and black text.)
    /// @param CssStylePrefix A prefix to append to the CSS classes defined in the style section.\n
    ///     This is useful to prevent duplicate CSS classes when combining HTML output from multiple controls.\n
    ///     This is only used for macOS and Windows where the RTF converts its color table to a CSS table.
    ///     For GTK+, all the formatting is done in place.
    [[nodiscard]]
    wxString GetUnthemedFormattedTextHtml([[maybe_unused]] const wxString& CssStylePrefix = wxString{});
    /// @brief Inserts formatted text into the control.
    /// @param formattedText The formatted text.
    /// @details On Windows and Mac this is RTF text, on Linux, it is Pango markup.
    void SetFormattedText(const wchar_t* formattedText);
    /// @returns The window's content as RTF.
    /// @param fixHighlightingTags Whether or not to include the "addChcbpatTag" tag,
    ///     which is used to highlight the background of text.\n
    ///     This is necessary for some programs that don't support the "highlight" or "cb"
    ///     command (e.g., LibreOffice).
    [[nodiscard]]
    wxString GetFormattedTextRtf(const bool fixHighlightingTags = true);
    /// @returns The length of the formatted text (this includes the length of all markup tags).
    [[nodiscard]]
    unsigned long GetFormattedTextLength() const noexcept
        { return m_rtfLength; }
#ifdef __WXMSW__
    /// @private
    [[nodiscard]]
    static DWORD wxCALLBACK EditStreamOutCallback(DWORD_PTR dwCookie, LPBYTE pbBuff,
                                                  LONG cb, [[maybe_unused]] LONG* pcb);
#endif

    /// @brief Assign a context menu to the control.
    /// @note Control takes ownership of this @c menu.
    /// @param menu The menu to display.
    void AssignContextMenu(wxMenu* menu) noexcept
        {
        delete m_menu;
        m_menu = menu;
        }
    /** @brief Searches for a given string in the control.
        @details Text will automatically be selected (if found).\n
            If the search reaches the end of the document,
            then the user will be prompted to see if he/she wants to wrap around the search.
        @param textToFind The string to search for.
        @param searchDown Whether to search downward from the current position.
        @param matchWholeWord Whether the search should use whole-word matching.
        @param caseSensitiveSearch Whether the search should be case-sensitive.
        @returns The index of the found text, or wxNOT_FOUND if not found.*/
    long FindText(const wchar_t* textToFind, const bool searchDown,
                  const bool matchWholeWord, const bool caseSensitiveSearch);

    /// @private
    void OnFind(wxFindDialogEvent& myEvent);
    /// @private
    void OnContextMenu([[maybe_unused]] wxContextMenuEvent& event);
    /// @private
    void OnSave([[maybe_unused]] wxCommandEvent& event);
    /// @private
    void OnPreview([[maybe_unused]] wxCommandEvent& event);
    /// @private
    void OnPrint([[maybe_unused]] wxCommandEvent& event);
    /// @private
    void OnCopyAll([[maybe_unused]] wxCommandEvent& event );
    /// @private
    void OnSelectAll([[maybe_unused]] wxCommandEvent& event );

    /** @name Saving Functions
        @brief Functions related to saving.*/
    /// @{

    /// @brief Saves the text control as a file, based on its extension.
    /// @param path The filepath to save to.
    /// @returns @c true if successful.
    bool Save(const wxFileName& path);
    /// @brief Saves the text control as an HTML file.
    /// @param path The filepath to save to.
    /// @returns @c true if successful.
    bool SaveAsHtml(const wxFileName& path);
    /// @brief Saves the text control as an RTF file.
    /// @param path The filepath to save to.
    /// @returns @c true if successful.
    bool SaveAsRtf(const wxFileName& path);
#ifdef __WXGTK__
    /// @brief Saves the text control as a Pango file.
    /// @param path The filepath to save to.
    /// @returns @c true if successful.
    bool GtkSaveAsPango(const wxFileName& path);
#endif
    /// @}

    /** @brief Sets the title for the document.
        @param title The title to display for the document.
        @note GetLabel() acts on the entire contents of control,
            so this is just a way to really set a title to the control
            (useful for printing and saving)*/
    void SetTitleName(const wxString& title)
        { m_titleName = title; }
    /// @returns The title of the document.
    [[nodiscard]]
    const wxString& GetTitleName() const noexcept
        { return m_titleName; }

    /** @name Printing Functions
        @brief Functions related to printing.*/
    /// @{

    /// @brief Sets the printer settings.
    /// @param printData The printer settings.
    void SetPrinterSettings(wxPrintData* printData) noexcept
        { m_printData = printData; }
    /// @brief Sets the paper size in TWIPs.
    /// @param size The size to use.
    void SetPaperSizeInTwips(const wxSize size)
        {
        // if landscape, then "turn the page on its side" by flipping the page size
        if (m_printOrientation == wxLANDSCAPE)
            { m_paperSize.Set(size.y, size.x); }
        else
            { m_paperSize = size; }
        }
    /// @brief Sets the paper size in inches.
    /// @param widthInInches The width to use.
    /// @param heightInInches The height to use.
    void SetPaperSizeInInches(const double widthInInches, const double heightInInches)
        {
        SetPaperSizeInTwips(wxSize(static_cast<int>(widthInInches * TWIPS_PER_INCH),
                                   static_cast<int>(heightInInches * TWIPS_PER_INCH)));
        }
    /// @brief Sets the paper size in millimeters.
    /// @param size The size to use.
    void SetPaperSizeInMillimeters(const wxSize size)
        {
        SetPaperSizeInInches(size.x * 0.0393700787, size.y * 0.0393700787);
        }
    /// @brief Sets the paper size.
    /// @param size The paper size to use.
    void SetPaperSize(const wxPaperSize size);
    /// @returns The printable page's rectangle (including margins).
    [[nodiscard]]
    wxRect GetPageRect() const
        { return wxRect(wxPoint(0, 0), m_paperSize); }
    /// @returns The width of the printing area.
    [[nodiscard]]
    int GetPageContentAreaWidth() const
        { return m_paperSize.x - m_rectMargin.GetLeft() - m_rectMargin.GetRight(); }
    /// @returns The actual area being printed (page minus the margins).
    [[nodiscard]]
    wxRect GetPageContentRect() const
        {
        wxRect printRect(m_rectMargin.GetLeft(), m_rectMargin.GetTop(),
                    m_paperSize.x - m_rectMargin.GetRight(),
                    m_paperSize.y - m_rectMargin.GetBottom());
        if (GetLeftPrinterHeader().length() ||
            GetCenterPrinterHeader().length() ||
            GetRightPrinterHeader().length())
            {
            printRect.height -= TWIPS_PER_INCH/2;
            printRect.y += TWIPS_PER_INCH/2;
            }
        if (GetLeftPrinterFooter().length() ||
            GetCenterPrinterFooter().length() ||
            GetRightPrinterFooter().length())
            {
            printRect.height -= TWIPS_PER_INCH/2;
            }
        return printRect;
        }
    /// @brief Sets the margin rect.
    /// @param rectMargin The margin to use.\n
    ///     The left and top of the rect defines the left and top margins,
    ///     and the right and bottom of the rect defines the width and height
    ///     of the right and bottom margins.
    void SetMarginRect(const wxRect& rectMargin) noexcept
        { m_rectMargin = rectMargin; }
    /// @returns The margin rect.
    [[nodiscard]]
    wxRect GetMarginRect() const noexcept
        { return m_rectMargin; }
    /// @brief Sets the paper orientation.
    /// @param orientation The paper orientation.
    void SetPrintOrientation(const wxPrintOrientation orientation)
        {
        // if orientation is changing then "turn" the paper
        if (m_printOrientation != orientation)
            {
            m_paperSize.Set(m_paperSize.y, m_paperSize.x);
            }
        m_printOrientation = orientation;
        }

    /// @brief Sets the left printer header.
    /// @param header The header value.
    void SetLeftPrinterHeader(const wxString& header)
        { m_leftPrinterHeader = header; }
    /// @returns The left printer header.
    [[nodiscard]]
    const wxString& GetLeftPrinterHeader() const noexcept
        { return m_leftPrinterHeader; }

    /// @brief Sets the center printer header.
    /// @param header The header value.
    void SetCenterPrinterHeader(const wxString& header)
        { m_centerPrinterHeader = header; }
    /// @returns The center printer header.
    [[nodiscard]]
    const wxString& GetCenterPrinterHeader() const noexcept
        { return m_centerPrinterHeader; }

    /// @brief Sets the right printer header.
    /// @param header The header value.
    void SetRightPrinterHeader(const wxString& header)
        { m_rightPrinterHeader = header; }
    /// @returns The right printer header.
    [[nodiscard]]
    const wxString& GetRightPrinterHeader() const noexcept
        { return m_rightPrinterHeader; }

    /// @brief Sets the left printer footer.
    /// @param footer The footer value.
    void SetLeftPrinterFooter(const wxString& footer)
        { m_leftPrinterFooter = footer; }
    /// @returns The left printer footer.
    [[nodiscard]]
    const wxString& GetLeftPrinterFooter() const noexcept
        { return m_leftPrinterFooter; }

    /// @brief Sets the center printer footer.
    /// @param footer The footer value.
    void SetCenterPrinterFooter(const wxString& footer)
        { m_centerPrinterFooter = footer; }
    /// @returns The center printer footer.
    [[nodiscard]]
    const wxString& GetCenterPrinterFooter() const noexcept
        { return m_centerPrinterFooter; }

    /// @brief Sets the right printer footer.
    /// @param footer The footer value.
    void SetRightPrinterFooter(const wxString& footer)
        { m_rightPrinterFooter = footer; }
    /// @returns The right printer footer.
    [[nodiscard]]
    const wxString& GetRightPrinterFooter() const noexcept
        { return m_rightPrinterFooter; }

    /// @brief Sets the watermark for the text window when printed.
    /// @param watermark The watermark information.
    /// @todo Not supported on macOS yet.
    void SetWatermark(const Wisteria::Canvas::Watermark& watermark) noexcept
        { m_waterMark = watermark; }
    /// @returns The watermark drawn across the printouts.
    [[nodiscard]]
    const Wisteria::Canvas::Watermark& GetWatermark() const noexcept
        { return m_waterMark; }
    /// @}
private:
    [[nodiscard]]
    wxString ExpandUnixPrintString(const wxString& printString) const
        {
        const wxDateTime now = wxDateTime::Now();
        wxString expandedString = printString;

        expandedString.Replace(L"@PAGENUM@", _DT(L"@PN"), true);
        expandedString.Replace(L"@PAGESCNT@", _DT(L"@PC"), true);
        expandedString.Replace(L"@TITLE@", GetTitleName(), true);
        expandedString.Replace(L"@DATE@", now.FormatDate(), true);
        expandedString.Replace(L"@TIME@", now.FormatTime(), true);

        return expandedString;
        }
#ifdef __WXGTK__
    enum class GtkFormat
        {
        HtmlFormat,
        RtfFormat
        };
    /// @returns The Pango buffer from the control directly.
    /// @note Call @c GetUnthemedFormattedText() to get the unthemed Pango text under GTK+.
    ///     (This is appropriate for exporting as Pango or printing.)
    [[nodiscard]]
    wxString GtkGetThemedPangoText();
    [[nodiscard]]
    wxString GtkGetFormattedText(const GtkFormat format, const bool useThemed = false);
#endif
    /** Fix highlighting so that it appears in programs that don't support the various
        background color tags. Basically, we add all variations of background color tags.
        Here is the reasoning:
        1. LibreOffice and TextEdit (macOS) don't understand "highlight".
        2. Wordpad doesn't understand "chcbpat".
        3. LibreOffice, Word, and WordPad don't understand "cb".*/
    [[nodiscard]]
    static wxString FixHighlightingTags(const wxString& text);
    /// @returns The formatted text meant for white backgrounds (e.g., paper).
    /// @note This will be in the native format (RTF for Windows and macOS, Pango for other UNIX systems).
    [[nodiscard]]
    const wxString& GetUnthemedFormattedText() const noexcept
        { return m_unthemedContent; }
    /// Copies print information from this control to its
    /// "dummy" control used for printing. We use a different
    /// text control for printing that is meant for showing
    /// black text on a white background (if this control is themed,
    /// then it can't be used for printing).
    void CopyPrintSettings(FormattedTextCtrl* that)
        {
        if (!that)
            { return; }
        that->m_titleName = m_titleName;
        that->m_paperSize = m_paperSize;
        that->m_rectMargin = m_rectMargin;
        that->m_printData = m_printData;
        that->m_printOrientation = m_printOrientation;
        that->m_leftPrinterHeader = m_leftPrinterHeader;
        that->m_centerPrinterHeader = m_centerPrinterHeader;
        that->m_rightPrinterHeader = m_rightPrinterHeader;
        that->m_leftPrinterFooter = m_leftPrinterFooter;
        that->m_centerPrinterFooter = m_centerPrinterFooter;
        that->m_rightPrinterFooter = m_rightPrinterFooter;
        that->m_waterMark = m_waterMark;
        }

    wxMenu* m_menu{ nullptr };
    unsigned long m_rtfLength{ 0 };
    wxString m_titleName;

    // printing data
    FormattedTextCtrl* m_printWindow{ nullptr };
    wxSize m_paperSize;
    wxRect m_rectMargin{ 720, 720, 720, 720 };

    wxPrintData* m_printData{ nullptr };
    wxPrintOrientation m_printOrientation{ wxLANDSCAPE };

    // headers
    wxString m_leftPrinterHeader;
    wxString m_centerPrinterHeader;
    wxString m_rightPrinterHeader;
    // footers
    wxString m_leftPrinterFooter;
    wxString m_centerPrinterFooter;
    wxString m_rightPrinterFooter;

    Wisteria::Canvas::Watermark m_waterMark;

    wxString m_unthemedContent;

    wxDECLARE_DYNAMIC_CLASS(FormattedTextCtrl);
     };

/** @}*/

#endif // __FORMATTEDTEXTCTRL_H__
