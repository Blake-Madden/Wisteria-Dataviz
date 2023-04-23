/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __GRIDEXPORT_DLG_H__
#define __GRIDEXPORT_DLG_H__

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/valgen.h>
#include "dialogwithhelp.h"

namespace Wisteria::UI
    {
    /** @brief Stores information about exporting a grid/list control.
        @note Column and row indices are stored as 1-indexed values here because that is the
            indexing that shown in the dialog for the user.
            Caller is responsible for converting these to 0-indexed values.*/
    struct GridExportOptions
        {
        /// @brief The starting row.
        int m_fromRow{ 1 };
        /// @brief The starting column.
        int m_fromColumn{ 1 };
        /// @brief The ending row.
        int m_toRow{ -1 };
        /// @brief The ending column.
        int m_toColumn{ -1 };
        /// @brief Whether column headers should be exported.
        bool m_includeColumnHeaders{ true };
        /// @brief Whether all data should be exported.
        ///     This will override the "from" and "to" fields.
        bool m_exportAll{ true };
        /// @brief Whether selected data should be exported.
        ///     This will override the "from" and "to" fields.
        bool m_exportSelected{ false };
        /// @brief Whether a range should be exported.
        ///     This will override the "from" and "to" fields.
        bool m_exportRange{ false };
        /// @brief Whether the output should be paginated.
        bool m_pageUsingPrinterSettings{ false };
        };

    /// Formats for exporting a grid/list control.
    enum class GridExportFormat
        {
        ExportText,  /*!< Plain (tab-delimited) text.*/
        ExportHtml,  /*!< HTML table.*/
        ExportRtf    /*!< Rich Text Format.*/
        };

    /// @brief Dialog for requesting export options for
    ///     a grid or list control.
    class GridExportDlg final : public DialogWithHelp
        {
    public:
        /** @brief Constructor.
            @param parent The parent to this dialog.
            @param rowCount The total number of rows in the control that is being exported.
            @param columnCount The total number of columns in the control that is being exported.
            @param exportFormat The file type to export as.
            @param id The window ID for this dialog.
            @param caption The dialog title.
            @param pos The window position for this dialog.
            @param size The dialog's size.
            @param style The dialog's window styling.*/
        GridExportDlg(wxWindow* parent, int rowCount, int columnCount,
            const GridExportFormat& exportFormat,
            wxWindowID id = wxID_ANY,
            const wxString& caption = _(L"List Export Options"),
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN);
        /// @private
        GridExportDlg(const GridExportDlg&) = delete;
        /// @private
        GridExportDlg(GridExportDlg&&) = delete;
        /// @private
        GridExportDlg& operator=(const GridExportDlg&) = delete;
        /// @private
        GridExportDlg& operator=(GridExportDlg&&) = delete;

        /// @returns @c true if user requested pagination in the output.
        [[nodiscard]] bool IsPaginating() const noexcept
            { return m_options.m_pageUsingPrinterSettings; }

        /// @returns @c true if user requested column headers in the output.
        [[nodiscard]] bool IsIncludingColumnHeaders() const noexcept
            { return m_options.m_includeColumnHeaders; }
        /** @brief Sets whether to include column headers in the output.
            @param includeColumnHeaders @c true to include column headers,
                @c false to exclude them.*/
        void IncludeColumnHeaders(const bool includeColumnHeaders)
            {
            m_options.m_includeColumnHeaders = includeColumnHeaders;
            TransferDataToWindow();
            }

        /// @returns @c true if user requested to export only the rows that are selected.
        [[nodiscard]] bool IsExportingSelectedRows() const noexcept
            { return m_options.m_exportSelected; }
        /** @brief Specifies whether to export all rows, or just the ones that are selected.
            @param exportSelectedRows @c true to export just the rows that are selected;
                @c false to export all rows.*/
        void ExportSelectedRowsOnly(const bool exportSelectedRows)
            {
            m_options.m_exportSelected = exportSelectedRows;
            // changing this option enable/disable the row range options
            TransferDataToWindow();
            }

        /// @returns The requested start row (1-indexed).
        /// @note Will be @c -1 if not specified.
        [[nodiscard]] int GetFromRow() const noexcept
            { return m_options.m_fromRow; }
        /// @returns The requested ending row (1-indexed).
        /// @note Will be @c -1 if not specified.
        [[nodiscard]] int GetToRow() const noexcept
            { return m_options.m_toRow; }

        /// @returns The requested start column (1-indexed).
        /// @note Will be @c -1 if not specified.
        [[nodiscard]] int GetFromColumn() const noexcept
            { return m_options.m_fromColumn; }
        /// @returns The requested ending column (1-indexed).
        /// @note Will be @c -1 if not specified.
        [[nodiscard]] int GetToColumn() const noexcept
            { return m_options.m_toColumn; }

        /// @returns The user's specified options.
        [[nodiscard]] const GridExportOptions& GetExportOptions() const noexcept
            { return m_options; }
    private:
        void CreateControls();

        GridExportOptions m_options;

        GridExportFormat m_exportFormat{ GridExportFormat::ExportText };

        wxCheckBox* m_paginateCheckBox{ nullptr };
        wxStaticBoxSizer* m_rangeBoxSizer{ nullptr };

        enum ControlIDs
            {
            ID_EXPORT_ALL_OPTION = wxID_HIGHEST,
            ID_EXPORT_SELECTED_OPTION,
            ID_EXPORT_RANGE_OPTION,
            ID_ROWS_FROM_SPIN,
            ID_ROWS_FROM_LABEL,
            ID_ROWS_TO_SPIN,
            ID_ROWS_TO_LABEL
            };
        };
    }

/** @}*/

#endif //__GRIDEXPORT_DLG_H__
