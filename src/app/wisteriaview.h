///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaview.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef WISTERIA_VIEW_H
#define WISTERIA_VIEW_H

#include "../base/reportbuilder.h"
#include "../ui/controls/sidebar.h"
#include "../util/windowcontainer.h"
#include <vector>
#include <wx/docview.h>
#include <wx/grid.h>
#include <wx/ribbon/art.h>
#include <wx/ribbon/bar.h>
#include <wx/ribbon/buttonbar.h>
#include <wx/splitter.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    class DatasetGridTable;
    }

/// @brief View class for Wisteria Dataviz projects.
class WisteriaView : public wxView
    {
  public:
    WisteriaView() = default;
    WisteriaView(const WisteriaView&) = delete;
    WisteriaView& operator=(const WisteriaView&) = delete;

    bool OnCreate(wxDocument* doc, long flags) override;

    void OnDraw(wxDC* dc) override {}

    bool OnClose(bool deleteWindow) override;

    void ShowSideBar(const bool show = true);

    [[nodiscard]]
    bool IsSideBarShown() const noexcept
        {
        return m_sidebarShown;
        }

    [[nodiscard]]
    Wisteria::UI::SideBar* GetSideBar() noexcept
        {
        return m_sideBar;
        }

    [[nodiscard]]
    wxSplitterWindow* GetSplitter() noexcept
        {
        return m_splitter;
        }

    /// @returns The canvases/pages in the project.
    [[nodiscard]]
    const std::vector<Wisteria::Canvas*>& GetPages() const noexcept
        {
        return m_pages;
        }

  private:
    void LoadProject();
    void OnSidebarClick(wxCommandEvent& event);
    void OnPrintAll(wxCommandEvent& event);
    void OnInsertDataset(wxCommandEvent& event);
    void AddDatasetToProject(const std::shared_ptr<Wisteria::Data::Dataset>& dataset,
                             const wxString& name,
                             const Wisteria::Data::Dataset::ColumnPreviewInfo& columnInfo = {});
    void ApplyColumnHeaderIcons(wxGrid* grid, Wisteria::UI::DatasetGridTable* table);
    void AdjustGridColumnsForIcons(wxGrid* grid);

    wxDocChildFrame* m_frame{ nullptr };
    wxSplitterWindow* m_splitter{ nullptr };
    Wisteria::UI::SideBar* m_sideBar{ nullptr };
    wxPanel* m_workArea{ nullptr };
    bool m_sidebarShown{ true };

    Wisteria::ReportBuilder m_reportBuilder;
    std::vector<Wisteria::Canvas*> m_pages;
    WindowContainer m_workWindows;

    constexpr static size_t DATA_ICON_INDEX{ 0 };
    constexpr static size_t PAGE_ICON_INDEX{ 1 };

    wxDECLARE_DYNAMIC_CLASS(WisteriaView);
    };

#endif // WISTERIA_VIEW_H
