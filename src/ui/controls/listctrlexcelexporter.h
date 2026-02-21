/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef LISTCTRL_EXCEL_EXPORTER_H
#define LISTCTRL_EXCEL_EXPORTER_H

#include "listctrlex.h"
#include <map>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

namespace Wisteria::UI
    {
    /** @brief Exports a ListCtrlEx to an *Excel* XLSX file.
        @details This class creates an XLSX file (which is a ZIP archive containing XML files)
            from the data in a ListCtrlEx. It preserves:
            - Numeric values as numbers (not formatted text)
            - Text values as strings
            - Cell background colors
            - Cell text colors

        Usage:
        @code
        ListCtrlExcelExporter exporter;
        if (!exporter.Export(myListCtrl, wxFileName("output.xlsx")))
            {
            wxMessageBox("Export failed!");
            }
        @endcode
    */
    class ListCtrlExcelExporter
        {
      public:
        /// @brief Default constructor.
        ListCtrlExcelExporter() = default;

        /// @private
        ListCtrlExcelExporter(const ListCtrlExcelExporter&) = delete;
        /// @private
        ListCtrlExcelExporter& operator=(const ListCtrlExcelExporter&) = delete;

        /** @brief Exports the list control to an XLSX file.
            @param listCtrl The list control to export.
            @param filePath The path to save the XLSX file.
            @param includeColumnHeaders Whether to include column headers as the first row.
            @returns @c true if successful, @c false otherwise.*/
        [[nodiscard]]
        bool Export(const ListCtrlEx* listCtrl, const wxFileName& filePath,
                    bool includeColumnHeaders = true);

        /** @brief Strips formatting from a number string and attempts to parse it.
            @param text The text to parse.
            @param[out] value The parsed value.
            @returns @c true if parsing succeeded, @c false otherwise.
            @note When both period and comma are present, the last one is treated as
                the decimal separator. When only one type is present and ambiguous
                (e.g., "1,234"), the system locale is used to determine interpretation.*/
        [[nodiscard]]
        static bool ParseFormattedNumber(const wxString& text, double& value);

      private:
        /// @brief Style information for a cell (colors and number format).
        struct CellStyle
            {
            /// @brief Background color.
            wxColour m_backgroundColor;
            /// @brief Font color.
            wxColour m_textColor;
            /// @brief Number format info.
            Wisteria::NumberFormatInfo m_numberFormat{
                Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting
            };

            /// @private
            bool operator<(const CellStyle& other) const
                {
                // compare background color first
                if (m_backgroundColor.IsOk() != other.m_backgroundColor.IsOk())
                    {
                    return !m_backgroundColor.IsOk();
                    }
                if (m_backgroundColor.IsOk() && other.m_backgroundColor.IsOk())
                    {
                    if (m_backgroundColor.GetRGBA() != other.m_backgroundColor.GetRGBA())
                        {
                        return m_backgroundColor.GetRGBA() < other.m_backgroundColor.GetRGBA();
                        }
                    }
                // compare text color
                if (m_textColor.IsOk() != other.m_textColor.IsOk())
                    {
                    return !m_textColor.IsOk();
                    }
                if (m_textColor.IsOk() && other.m_textColor.IsOk())
                    {
                    if (m_textColor.GetRGBA() != other.m_textColor.GetRGBA())
                        {
                        return m_textColor.GetRGBA() < other.m_textColor.GetRGBA();
                        }
                    }
                // compare number format
                if (m_numberFormat.m_type != other.m_numberFormat.m_type)
                    {
                    return m_numberFormat.m_type < other.m_numberFormat.m_type;
                    }
                if (m_numberFormat.m_precision != other.m_numberFormat.m_precision)
                    {
                    return m_numberFormat.m_precision < other.m_numberFormat.m_precision;
                    }
                if (m_numberFormat.m_displayThousandsSeparator !=
                    other.m_numberFormat.m_displayThousandsSeparator)
                    {
                    return m_numberFormat.m_displayThousandsSeparator <
                           other.m_numberFormat.m_displayThousandsSeparator;
                    }
                return false;
                }
            };

        /// @brief Cell data including value type and style index.
        struct CellData
            {
            enum class CellType
                {
                Number,
                String,
                Empty
                };
            CellType m_type{ CellType::Empty };
            double m_numericValue{ 0.0 };
            size_t m_stringIndex{ 0 };
            size_t m_styleIndex{ 0 };
            Wisteria::NumberFormatInfo m_numberFormat{
                Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting
            };
            };

        /// @brief Builds the content for "[Content_Types].xml."
        [[nodiscard]]
        wxString BuildContentTypesXml();

        /// @brief Builds the content for "_rels/.rels."
        [[nodiscard]]
        static wxString BuildRelsXml();

        /// @brief Builds the content for "xl/workbook.xml."
        [[nodiscard]]
        wxString BuildWorkbookXml();

        /// @brief Builds the content for "xl/_rels/workbook.xml.rels."
        [[nodiscard]]
        wxString BuildWorkbookRelsXml();

        /// @brief Builds the content for "xl/worksheets/sheet1.xml."
        [[nodiscard]]
        wxString BuildSheetXml(bool includeColumnHeaders);

        /// @brief Builds the content for "xl/sharedStrings.xml."
        [[nodiscard]]
        wxString BuildSharedStringsXml();

        /// @brief Builds the content for "xl/styles.xml."
        [[nodiscard]]
        wxString BuildStylesXml();

        /// @brief Collects all unique strings and styles from the list control.
        void CollectStringsAndStyles(bool includeColumnHeaders);

        /// @brief Gets or adds a string to the shared strings table.
        /// @param str The string to add.
        /// @returns The index of the string in the shared strings table.
        size_t GetOrAddSharedString(const wxString& str);

        /// @brief Gets or adds a style to the styles table.
        /// @param style The style to add.
        /// @returns The index of the style in the styles table.
        size_t GetOrAddStyle(const CellStyle& style);

        /// @brief Converts a column number to *Excel* column letters (0=A, 1=B, ..., 26=AA, etc.)
        [[nodiscard]]
        static wxString ColumnToLetter(long column);

        /// @brief Escapes XML special characters in a string.
        [[nodiscard]]
        static wxString EscapeXml(const wxString& str);

        /// @brief Converts a wxColour to an *Excel* ARGB hex string (AARRGGBB format).
        [[nodiscard]]
        static wxString ColorToArgb(const wxColour& color);

        /// @brief Gets the sheet name from the list control's label.
        /// @details Truncates to 31 characters and removes invalid characters.
        [[nodiscard]]
        wxString GetSheetName() const;

        /// @brief Gets the cell data (value, type, style) for a specific cell.
        [[nodiscard]]
        CellData GetCellData(long row, long column);

        /// @brief Gets the row style (colors) for a specific row.
        [[nodiscard]]
        CellStyle GetRowStyle(long row);

        /// @brief Gets the cell style including number format for a specific cell.
        [[nodiscard]]
        CellStyle GetCellStyle(long row, long column);

        /// @brief Gets the Excel number format ID for a given format.
        /// @details Returns 0 for general format, or a custom format ID for percentages, etc.
        [[nodiscard]]
        static size_t GetExcelNumberFormatId(const Wisteria::NumberFormatInfo& format);

        /// @brief Checks if a cell contains a numeric value.
        [[nodiscard]]
        bool IsCellNumeric(long row, long column);

        /// @brief Gets the numeric value of a cell.
        [[nodiscard]]
        double GetCellNumericValue(long row, long column);

        // current list control being exported
        const ListCtrlEx* m_listCtrl{ nullptr };

        // shared strings table: string -> index
        std::map<wxString, size_t> m_sharedStrings;
        std::vector<wxString> m_sharedStringsList;

        // styles table: style -> index
        std::map<CellStyle, size_t> m_styles;
        std::vector<CellStyle> m_stylesList;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // LISTCTRL_EXCEL_EXPORTER_H
