///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriadoc.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef WISTERIA_DOC_H
#define WISTERIA_DOC_H

#include "../base/reportbuilder.h"
#include "../wxSimpleJSON/src/wxSimpleJSON.h"
#include <wx/docview.h>
#include <wx/filename.h>
#include <wx/wx.h>

/// @brief Document class for Wisteria Dataviz projects.
class WisteriaDoc : public wxDocument
    {
    wxDECLARE_DYNAMIC_CLASS(WisteriaDoc);

  public:
    WisteriaDoc() = default;
    WisteriaDoc(const WisteriaDoc&) = delete;
    WisteriaDoc& operator=(const WisteriaDoc&) = delete;

    /** @returns The path of a file, relative to the project's path.
        @param filePath The filepath to convert.*/
    [[nodiscard]]
    wxString MakeRelativePath(const wxString& filePath) const;

    /** @returns The path of a (relative to the project) file,
            expanded to its full path.
        @param filePath The filepath to convert.*/
    [[nodiscard]]
    wxString ResolveFilePath(const wxString& filePath) const;

  private:
    bool OnNewDocument() override;
    bool OnOpenDocument(const wxString& filename) override;
    bool DoSaveDocument(const wxString& filename) override;
    void SaveProject(const wxString& filePath);

    // page item save helpers
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SavePageItem(const Wisteria::GraphItems::GraphItemBase* item,
                                     const Wisteria::Canvas* canvas) const;
    void SaveItem(wxSimpleJSON::Ptr_t& itemNode, const Wisteria::GraphItems::GraphItemBase* item,
                  const Wisteria::Canvas* canvas) const;
    [[nodiscard]]
    wxString SavePenToStr(const wxPen& pen) const;
    [[nodiscard]]
    wxString SaveBrushToStr(const wxBrush& brush) const;
    [[nodiscard]]
    wxString SaveLabelPropertiesToStr(const Wisteria::GraphItems::Label& label) const;

    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveLabel(const Wisteria::GraphItems::Label* label,
                                  const Wisteria::Canvas* canvas) const;
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveImage(const Wisteria::GraphItems::Image* image,
                                  const Wisteria::Canvas* canvas) const;
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveShape(const Wisteria::GraphItems::Shape* shape,
                                  const Wisteria::Canvas* canvas) const;
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveFillableShape(const Wisteria::GraphItems::FillableShape* shape,
                                          const Wisteria::Canvas* canvas) const;
    void SaveGraph(const Wisteria::Graphs::Graph2D* graph, wxSimpleJSON::Ptr_t& graphNode,
                   const Wisteria::Canvas* canvas) const;
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveGraphByType(const Wisteria::Graphs::Graph2D* graph,
                                        const Wisteria::Canvas* canvas) const;

    static void SaveDatasetImportOptions(wxSimpleJSON::Ptr_t& dsNode,
                                         const Wisteria::Data::Dataset::ColumnPreviewInfo& colInfo,
                                         const Wisteria::Data::ImportInfo& info);
    static void
    SaveTransformOptions(wxSimpleJSON::Ptr_t& dsNode,
                         const Wisteria::ReportBuilder::DatasetTransformOptions& txOpts);
    static void
    SaveFormulas(wxSimpleJSON::Ptr_t& dsNode,
                 const std::vector<Wisteria::ReportBuilder::DatasetFormulaInfo>& formulas);
    static void SaveSubsetFilters(wxSimpleJSON::Ptr_t& subsetNode,
                                  const Wisteria::ReportBuilder::DatasetSubsetOptions& sOpts);
    void SaveSubsets(wxSimpleJSON::Ptr_t& parentNode, const wxString& sourceName) const;
    void SavePivots(wxSimpleJSON::Ptr_t& parentNode, const wxString& sourceName) const;
    void SaveMerges(wxSimpleJSON::Ptr_t& parentNode, const wxString& sourceName) const;
    [[nodiscard]]
    wxSimpleJSON::Ptr_t SaveCommonAxis(const Wisteria::GraphItems::Axis* axis,
                                       const Wisteria::Canvas* canvas) const;

    [[nodiscard]]
    static wxString GetGraphTypeString(const Wisteria::Graphs::Graph2D* graph);

    // save helpers
    [[nodiscard]]
    static wxString EscapeJsonStr(const wxString& str);
    [[nodiscard]]
    wxString ColorToStr(const wxColour& color) const;
    };

#endif // WISTERIA_DOC_H
