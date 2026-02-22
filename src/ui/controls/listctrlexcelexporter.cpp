///////////////////////////////////////////////////////////////////////////////
// Name:        listctrlexcelexporter.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "listctrlexcelexporter.h"
#include <cmath>
#include <cwctype>
#include <wx/intl.h>

namespace Wisteria::UI
    {
    //------------------------------------------------------
    bool ListCtrlExcelExporter::Export(const ListCtrlEx* listCtrl, const wxFileName& filePath,
                                       const bool includeColumnHeaders)
        {
        if (listCtrl == nullptr || listCtrl->GetItemCount() == 0)
            {
            return false;
            }

        // set up for this export operation
        m_listCtrl = listCtrl;
        m_sharedStrings.clear();
        m_sharedStringsList.clear();
        m_styles.clear();
        m_stylesList.clear();

        // create the directory if it doesn't exist
        wxFileName::Mkdir(filePath.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

        // collect all strings and styles first
        CollectStringsAndStyles(includeColumnHeaders);

        // create the ZIP file
        wxFFileOutputStream fileStream(filePath.GetFullPath());
        if (!fileStream.IsOk())
            {
            m_listCtrl = nullptr;
            return false;
            }

        wxZipOutputStream zipStream(fileStream);
        if (!zipStream.IsOk())
            {
            m_listCtrl = nullptr;
            return false;
            }

        // helper lambda to add a file to the ZIP
        const auto addToZip = [&zipStream](const wxString& path, const wxString& content) -> bool
        {
            wxZipEntry* entry = new wxZipEntry(path);
            if (!zipStream.PutNextEntry(entry))
                {
                return false;
                }
            const wxScopedCharBuffer utf8 = content.ToUTF8();
            zipStream.Write(utf8.data(), utf8.length());
            return zipStream.CloseEntry();
        };

        // add all required files to the ZIP
        const bool success =
            addToZip(L"[Content_Types].xml", BuildContentTypesXml()) &&
            // quneiform-suppress-begin
            addToZip(L"_rels/.rels", BuildRelsXml()) &&
            // quneiform-suppress-end
            addToZip(L"xl/workbook.xml", BuildWorkbookXml()) &&
            addToZip(L"xl/_rels/workbook.xml.rels", BuildWorkbookRelsXml()) &&
            addToZip(L"xl/worksheets/sheet1.xml", BuildSheetXml(includeColumnHeaders)) &&
            (m_sharedStringsList.empty() ||
             addToZip(L"xl/sharedStrings.xml", BuildSharedStringsXml())) &&
            addToZip(L"xl/styles.xml", BuildStylesXml());

        const bool closeSuccess = zipStream.Close() && fileStream.Close();

        // clean up
        m_listCtrl = nullptr;
        m_sharedStrings.clear();
        m_sharedStringsList.clear();
        m_styles.clear();
        m_stylesList.clear();

        return success && closeSuccess;
        }

    //------------------------------------------------------
    void ListCtrlExcelExporter::CollectStringsAndStyles(const bool includeColumnHeaders)
        {
        // add a default style (index 0) with no formatting
        CellStyle defaultStyle;
        GetOrAddStyle(defaultStyle);

        // collect column headers
        if (includeColumnHeaders)
            {
            for (long col = 0; col < m_listCtrl->GetColumnCount(); ++col)
                {
                const wxString headerText = m_listCtrl->GetColumnName(col);
                GetOrAddSharedString(headerText);
                }
            // add header style (blue background, white text)
            CellStyle headerStyle;
            headerStyle.m_backgroundColor = wxColour(0x33, 0x7B, 0xC4); // #337BC4
            headerStyle.m_textColor = *wxWHITE;
            GetOrAddStyle(headerStyle);
            }

        // collect data cells
        for (long row = 0; row < m_listCtrl->GetItemCount(); ++row)
            {
            for (long col = 0; col < m_listCtrl->GetColumnCount(); ++col)
                {
                // get cell style (includes colors and number format)
                const CellStyle cellStyle = GetCellStyle(row, col);
                GetOrAddStyle(cellStyle);

                // if not numeric, add to shared strings
                if (!IsCellNumeric(row, col))
                    {
                    const wxString cellText = m_listCtrl->GetItemTextEx(row, col);
                    if (!cellText.empty())
                        {
                        GetOrAddSharedString(cellText);
                        }
                    }
                }
            }
        }

    //------------------------------------------------------
    size_t ListCtrlExcelExporter::GetOrAddSharedString(const wxString& str)
        {
        auto it = m_sharedStrings.find(str);
        if (it != m_sharedStrings.end())
            {
            return it->second;
            }
        const size_t index = m_sharedStringsList.size();
        m_sharedStrings[str] = index;
        m_sharedStringsList.emplace_back(str);
        return index;
        }

    //------------------------------------------------------
    size_t ListCtrlExcelExporter::GetOrAddStyle(const CellStyle& style)
        {
        auto it = m_styles.find(style);
        if (it != m_styles.end())
            {
            return it->second;
            }
        const size_t index = m_stylesList.size();
        m_styles[style] = index;
        m_stylesList.emplace_back(style);
        return index;
        }

    //------------------------------------------------------
    wxString ListCtrlExcelExporter::ColumnToLetter(const long column)
        {
        wxString result;
        long col = column;
        while (col >= 0)
            {
            result.Prepend(static_cast<wchar_t>(L'A' + (col % 26)));
            col = (col / 26) - 1;
            }
        return result;
        }

    //------------------------------------------------------
    wxString ListCtrlExcelExporter::EscapeXml(const wxString& str)
        {
        wxString result;
        result.reserve(str.length() * 1.1);
        for (size_t i = 0; i < str.length(); ++i)
            {
            const wchar_t ch = str[i];
            if (ch == L'&')
                {
                result += L"&amp;";
                }
            else if (ch == L'<')
                {
                result += L"&lt;";
                }
            else if (ch == L'>')
                {
                result += L"&gt;";
                }
            else if (ch == L'"')
                {
                result += L"&quot;";
                }
            else if (ch == L'\'')
                {
                result += L"&apos;";
                }
            // filter out control characters (except tab, newline, carriage return)
            else if (ch >= 0x20 || ch == L'\t' || ch == L'\n' || ch == L'\r')
                {
                result += ch;
                }
            }
        return result;
        }

    //------------------------------------------------------
    wxString ListCtrlExcelExporter::ColorToArgb(const wxColour& color)
        {
        if (!color.IsOk())
            {
            return L"FF000000"; // default black
            }
        return wxString::Format(L"FF%02X%02X%02X", color.Red(), color.Green(), color.Blue());
        }

    //------------------------------------------------------
    wxString ListCtrlExcelExporter::GetSheetName() const
        {
        wxString name = m_listCtrl->GetLabel();

        // remove invalid characters for Excel sheet names
        name.Replace(L"\\", L"_");
        name.Replace(L"/", L"_");
        name.Replace(L"?", L"_");
        name.Replace(L"*", L"_");
        name.Replace(L"[", L"_");
        name.Replace(L"]", L"_");

        // truncate to Excel's 31 character limit
        if (name.length() > 31)
            {
            name.Truncate(31);
            }

        // use default if empty
        if (name.empty())
            {
            name = L"Sheet1";
            }

        return name;
        }

    //------------------------------------------------------
    ListCtrlExcelExporter::CellStyle ListCtrlExcelExporter::GetRowStyle(const long row)
        {
        CellStyle style;

        const wxItemAttr* const virtualAttrib = m_listCtrl->OnGetItemAttr(row);
        if (m_listCtrl->IsVirtual() && virtualAttrib != nullptr)
            {
            style.m_backgroundColor = virtualAttrib->GetBackgroundColour();
            style.m_textColor = virtualAttrib->GetTextColour();
            }
        else
            {
            style.m_backgroundColor = m_listCtrl->GetItemBackgroundColour(row);
            style.m_textColor = m_listCtrl->GetItemTextColour(row);
            }

        return style;
        }

    //------------------------------------------------------
    ListCtrlExcelExporter::CellStyle ListCtrlExcelExporter::GetCellStyle(const long row,
                                                                         const long column)
        {
        // start with row style (colors)
        CellStyle style = GetRowStyle(row);

        // get number format from data provider if available
        if (m_listCtrl->IsVirtual())
            {
            const auto& dataProvider = m_listCtrl->GetVirtualDataProvider();
            if (dataProvider != nullptr)
                {
                if (dataProvider->IsKindOf(wxCLASSINFO(ListCtrlExNumericDataProvider)))
                    {
                    const auto* numericProvider =
                        dynamic_cast<const ListCtrlExNumericDataProvider*>(dataProvider.get());
                    const auto& matrix = numericProvider->GetMatrix();
                    if (static_cast<size_t>(row) < matrix.size() &&
                        static_cast<size_t>(column) < matrix[row].size())
                        {
                        style.m_numberFormat = matrix[row][column].GetNumberFormatType();
                        }
                    }
                else if (dataProvider->IsKindOf(wxCLASSINFO(ListCtrlExDataProvider)))
                    {
                    const auto* stringProvider =
                        dynamic_cast<const ListCtrlExDataProvider*>(dataProvider.get());
                    const auto& matrix = stringProvider->GetMatrix();
                    if (static_cast<size_t>(row) < matrix.size() &&
                        static_cast<size_t>(column) < matrix[row].size())
                        {
                        style.m_numberFormat = matrix[row][column].GetNumberFormatType();
                        }
                    }
                }
            }

        return style;
        }

    //------------------------------------------------------
    size_t ListCtrlExcelExporter::GetExcelNumberFormatId(const Wisteria::NumberFormatInfo& format)
        {
        // Excel built-in number format IDs:
        // 0 = General
        // 1 = 0
        // 2 = 0.00
        // 3 = #,##0
        // 4 = #,##0.00
        // 9 = 0%
        // 10 = 0.00%
        // We'll use built-in IDs where possible

        if (format.m_type == NumberFormatInfo::NumberFormatType::PercentageFormatting)
            {
            // use 0% or 0.00% depending on precision
            return (format.m_precision == 0) ? 9 : 10;
            }

        // for standard formatting, use general (0)
        return 0;
        }

    //------------------------------------------------------
    bool ListCtrlExcelExporter::IsCellNumeric(const long row, const long column)
        {
        // for virtual lists, check the data provider type
        if (m_listCtrl->IsVirtual())
            {
            const auto& dataProvider = m_listCtrl->GetVirtualDataProvider();
            if (dataProvider != nullptr)
                {
                if (dataProvider->IsKindOf(wxCLASSINFO(ListCtrlExNumericDataProvider)))
                    {
                    const auto* numericProvider =
                        dynamic_cast<const ListCtrlExNumericDataProvider*>(dataProvider.get());
                    const auto& matrix = numericProvider->GetMatrix();
                    if (static_cast<size_t>(row) < matrix.size() &&
                        static_cast<size_t>(column) < matrix[row].size())
                        {
                        const auto& cell = matrix[row][column];
                        // if label code is 0, it's a numeric value
                        // (label codes start at 1 for actual labels)
                        if (cell.m_labelCode == 0 && !std::isnan(cell.m_numericValue))
                            {
                            return true;
                            }
                        }
                    // numeric provider but cell is a label, not a number
                    return false;
                    }
                if (dataProvider->IsKindOf(wxCLASSINFO(ListCtrlExDataProvider)))
                    {
                    const auto* stringProvider =
                        dynamic_cast<const ListCtrlExDataProvider*>(dataProvider.get());
                    const auto& matrix = stringProvider->GetMatrix();
                    if (static_cast<size_t>(row) < matrix.size() &&
                        static_cast<size_t>(column) < matrix[row].size())
                        {
                        const auto& cell = matrix[row][column];
                        const wxString& text = cell.m_strVal;
                        if (text.empty())
                            {
                            return false;
                            }

                        // if format is PercentageFormatting, strip % and try to parse
                        if (cell.GetNumberFormatType().m_type ==
                            NumberFormatInfo::NumberFormatType::PercentageFormatting)
                            {
                            wxString cleanedText = text;
                            cleanedText.Replace(L"%", wxString{});
                            cleanedText.Trim().Trim(false);
                            double value{ 0.0 };
                            return cleanedText.ToDouble(&value);
                            }

                        // for other formats, use standard parsing (rejects % and $)
                        double value{ 0.0 };
                        return ParseFormattedNumber(text, value);
                        }
                    return false;
                    }
                }
            }

        // for non-virtual lists, try to parse as formatted number
        const wxString text = m_listCtrl->GetItemTextEx(row, column);
        if (text.empty())
            {
            return false;
            }

        double value{ 0.0 };
        return ParseFormattedNumber(text, value);
        }

    //------------------------------------------------------
    double ListCtrlExcelExporter::GetCellNumericValue(const long row, const long column)
        {
        // for virtual lists, check the data provider type
        if (m_listCtrl->IsVirtual())
            {
            const auto& dataProvider = m_listCtrl->GetVirtualDataProvider();
            if (dataProvider != nullptr)
                {
                if (dataProvider->IsKindOf(wxCLASSINFO(ListCtrlExNumericDataProvider)))
                    {
                    const auto* numericProvider =
                        dynamic_cast<const ListCtrlExNumericDataProvider*>(dataProvider.get());
                    return numericProvider->GetItemValue(row, column);
                    }
                else if (dataProvider->IsKindOf(wxCLASSINFO(ListCtrlExDataProvider)))
                    {
                    const auto* stringProvider =
                        dynamic_cast<const ListCtrlExDataProvider*>(dataProvider.get());
                    const auto& matrix = stringProvider->GetMatrix();
                    if (static_cast<size_t>(row) < matrix.size() &&
                        static_cast<size_t>(column) < matrix[row].size())
                        {
                        const auto& cell = matrix[row][column];
                        const wxString& text = cell.m_strVal;

                        // if format is PercentageFormatting, strip % and parse
                        if (cell.GetNumberFormatType().m_type ==
                            NumberFormatInfo::NumberFormatType::PercentageFormatting)
                            {
                            wxString cleanedText = text;
                            cleanedText.Replace(L"%", wxString{});
                            cleanedText.Trim().Trim(false);
                            double value{ 0.0 };
                            if (cleanedText.ToDouble(&value))
                                {
                                return value;
                                }
                            }
                        else
                            {
                            double value{ 0.0 };
                            if (ParseFormattedNumber(text, value))
                                {
                                return value;
                                }
                            }
                        }
                    return std::numeric_limits<double>::quiet_NaN();
                    }
                }
            }

        // for non-virtual lists, parse the text
        const wxString text = m_listCtrl->GetItemTextEx(row, column);
        double value{ 0.0 };
        if (ParseFormattedNumber(text, value))
            {
            return value;
            }
        return std::numeric_limits<double>::quiet_NaN();
        }

    //------------------------------------------------------
    bool ListCtrlExcelExporter::ParseFormattedNumber(const wxString& text, double& value)
        {
        if (text.empty())
            {
            return false;
            }

        // first, try a direct parse (handles simple numbers)
        if (text.ToDouble(&value))
            {
            return true;
            }

        // reject strings containing alphabetic characters or symbols that indicate
        // this is not a pure formatted number (letters, percent, currency, etc.)
        for (const auto ch : text)
            {
            if (std::iswalpha(static_cast<wint_t>(ch)) || ch == L'%' || ch == L'$' ||
                ch == L'€' ||             // euro €
                ch == L'£' || ch == L'¥') // pound £, yen ¥
                {
                return false;
                }
            }

        // strip common formatting characters
        wxString cleaned;
        cleaned.reserve(text.length());

        bool hasDecimalPoint = false;
        bool hasComma = false;
        size_t lastCommaPos = wxString::npos;
        size_t lastPeriodPos = wxString::npos;

        // first pass: analyze the string to determine decimal separator
        for (size_t i = 0; i < text.length(); ++i)
            {
            const wxUniChar ch = text[i];
            if (ch == L',')
                {
                hasComma = true;
                lastCommaPos = i;
                }
            else if (ch == L'.')
                {
                hasDecimalPoint = true;
                lastPeriodPos = i;
                }
            }

        // determine which is the decimal separator:
        // - if both exist, the one that appears last is the decimal separator
        // - "1,234.56" -> period is decimal (US format)
        // - "1.234,56" -> comma is decimal (European format)
        // - if only one type exists and is ambiguous, use the system locale
        const std::wstring localeDecimal =
            wxLocale::GetInfo(wxLOCALE_DECIMAL_POINT, wxLOCALE_CAT_NUMBER).ToStdWstring();
        const wchar_t localeDecimalSep = localeDecimal.empty() ? L'.' : localeDecimal[0];

        wchar_t decimalSep{ L'.' };
        if (hasComma && hasDecimalPoint)
            {
            // both present - the last one is the decimal separator
            decimalSep = (lastCommaPos > lastPeriodPos) ? L',' : L'.';
            }
        else if (hasComma && !hasDecimalPoint)
            {
            // only commas - could be thousands separator or decimal
            const size_t commaCount = std::count(text.cbegin(), text.cend(), L',');
            if (commaCount == 1)
                {
                // ambiguous case like "1,234" - use locale to decide
                // if locale uses comma as decimal, treat as decimal; otherwise thousands...
                if (localeDecimalSep == L',')
                    {
                    decimalSep = L',';
                    }
                // ...else, leave as period (comma treated as thousands separator)
                }
            // multiple commas means definitely thousands separators, decimalSep stays '.'
            }
        else if (hasDecimalPoint && !hasComma)
            {
            // only periods - could be thousands separator or decimal
            const size_t periodCount = std::count(text.begin(), text.end(), L'.');
            if (periodCount == 1)
                {
                // ambiguous case like "1.234" - use locale to decide
                // if locale uses period as decimal (or is unknown), treat as decimal;
                // if locale uses comma as decimal, treat period as thousands
                if (localeDecimalSep == L',')
                    {
                    decimalSep = L','; // period will be stripped as thousands
                    }
                // ...else, leave as period (default behavior)
                }
            // multiple periods = definitely thousands separators
            else if (periodCount > 1)
                {
                // periods are thousands, no decimal present
                decimalSep = L',';
                }
            }

        // second pass: build cleaned string
        for (const auto ch : text)
            {
            if (ch >= L'0' && ch <= L'9')
                {
                cleaned += ch;
                }
            else if (ch == L'-' || ch == L'+')
                {
                // allow sign at beginning
                if (cleaned.empty())
                    {
                    cleaned += ch;
                    }
                }
            else if (ch == decimalSep)
                {
                // convert to period for parsing
                cleaned += L'.';
                }
            // skip thousands separators, currency symbols, spaces, percent signs, etc.
            }

        if (cleaned.empty() || cleaned == L"-" || cleaned == L"+")
            {
            return false;
            }

        return cleaned.ToDouble(&value);
        }

    //------------------------------------------------------
    ListCtrlExcelExporter::CellData ListCtrlExcelExporter::GetCellData(const long row,
                                                                       const long column)
        {
        CellData data;
        const CellStyle cellStyle = GetCellStyle(row, column);
        data.m_styleIndex = GetOrAddStyle(cellStyle);
        data.m_numberFormat = cellStyle.m_numberFormat;

        if (IsCellNumeric(row, column))
            {
            data.m_type = CellData::CellType::Number;
            data.m_numericValue = GetCellNumericValue(row, column);

            // convert percentage values for Excel
            // (Excel's % format multiplies by 100, so we need to divide by 100)
            if (cellStyle.m_numberFormat.m_type ==
                NumberFormatInfo::NumberFormatType::PercentageFormatting)
                {
                data.m_numericValue /= 100.0;
                }
            }
        else
            {
            const wxString text = m_listCtrl->GetItemTextEx(row, column);
            if (text.empty())
                {
                data.m_type = CellData::CellType::Empty;
                }
            else
                {
                data.m_type = CellData::CellType::String;
                data.m_stringIndex = GetOrAddSharedString(text);
                }
            }

        return data;
        }

    //------------------------------------------------------
    wxString ListCtrlExcelExporter::BuildContentTypesXml()
        {
        // quneiform-suppress-begin
        wxString xml =
            L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
            "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
            "  <Default Extension=\"rels\" "
            "ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
            "  <Default Extension=\"xml\" ContentType=\"application/xml\"/>\n"
            "  <Override PartName=\"/xl/workbook.xml\" "
            "ContentType=\"application/vnd.openxmlformats-officedocument."
            "spreadsheetml.sheet.main+xml\"/>\n"
            "  <Override PartName=\"/xl/worksheets/sheet1.xml\" "
            "ContentType=\"application/vnd.openxmlformats-officedocument."
            "spreadsheetml.worksheet+xml\"/>\n"
            "  <Override PartName=\"/xl/styles.xml\" "
            "ContentType=\"application/vnd.openxmlformats-officedocument."
            "spreadsheetml.styles+xml\"/>\n";

        if (!m_sharedStringsList.empty())
            {
            xml += "  <Override PartName=\"/xl/sharedStrings.xml\" "
                   "ContentType=\"application/vnd.openxmlformats-officedocument."
                   "spreadsheetml.sharedStrings+xml\"/>\n";
            }

        xml += "</Types>";
        return xml;
        // quneiform-suppress-end
        }

    //------------------------------------------------------
    wxString ListCtrlExcelExporter::BuildRelsXml()
        {
        return L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
               "<Relationships "
               "xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
               "\n"
               "  <Relationship Id=\"rId1\" "
               "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
               "officeDocument\" "
               "Target=\"xl/workbook.xml\"/>\n"
               "</Relationships>";
        }

    //------------------------------------------------------
    wxString ListCtrlExcelExporter::BuildWorkbookXml()
        {
        const wxString sheetName = EscapeXml(GetSheetName());
        return wxString::Format(
            L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
            "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
            "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">\n"
            "  <sheets>\n"
            "    <sheet name=\"%s\" sheetId=\"1\" r:id=\"rId1\"/>\n"
            "  </sheets>\n"
            "</workbook>",
            sheetName);
        }

    //------------------------------------------------------
    wxString ListCtrlExcelExporter::BuildWorkbookRelsXml()
        {
        wxString xml =
            L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
            "<Relationships "
            "xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
            "  <Relationship Id=\"rId1\" "
            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
            "worksheet\" "
            "Target=\"worksheets/sheet1.xml\"/>\n"
            "  <Relationship Id=\"rId2\" "
            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" "
            "Target=\"styles.xml\"/>\n";

        if (!m_sharedStringsList.empty())
            {
            xml += "  <Relationship Id=\"rId3\" "
                   "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/"
                   "sharedStrings\" "
                   "Target=\"sharedStrings.xml\"/>\n";
            }

        xml += "</Relationships>";
        return xml;
        }

    //------------------------------------------------------
    wxString ListCtrlExcelExporter::BuildSheetXml(const bool includeColumnHeaders)
        {
        wxString xml =
            L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
            "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n"
            "  <sheetData>\n";

        long excelRow{ 1 };

        // write column headers
        if (includeColumnHeaders)
            {
            xml += wxString::Format(L"    <row r=\"%ld\">\n", excelRow);

            // get header style index (blue background, white text)
            CellStyle headerStyle;
            headerStyle.m_backgroundColor = wxColour(0x33, 0x7B, 0xC4);
            headerStyle.m_textColor = *wxWHITE;
            const size_t headerStyleIndex = GetOrAddStyle(headerStyle);

            for (long col = 0; col < m_listCtrl->GetColumnCount(); ++col)
                {
                const wxString cellRef = ColumnToLetter(col) + std::to_string(excelRow);
                const wxString headerText = m_listCtrl->GetColumnName(col);
                const size_t stringIndex = GetOrAddSharedString(headerText);

                xml += wxString::Format(L"      <c r=\"%s\" t=\"s\" s=\"%zu\"><v>%zu</v></c>\n",
                                        cellRef, headerStyleIndex, stringIndex);
                }
            xml += L"    </row>\n";
            ++excelRow;
            }

        // write data rows
        for (long row = 0; row < m_listCtrl->GetItemCount(); ++row)
            {
            xml += wxString::Format(L"    <row r=\"%ld\">\n", excelRow);

            for (long col = 0; col < m_listCtrl->GetColumnCount(); ++col)
                {
                const wxString cellRef = ColumnToLetter(col) + std::to_string(excelRow);
                const CellData cellData = GetCellData(row, col);

                switch (cellData.m_type)
                    {
                case CellData::CellType::Number:
                    {
                    // format number with full precision
                    xml +=
                        wxString::Format(L"      <c r=\"%s\" s=\"%zu\"><v>%.15g</v></c>\n", cellRef,
                                         cellData.m_styleIndex, cellData.m_numericValue);
                    break;
                    }
                case CellData::CellType::String:
                    {
                    xml += wxString::Format(L"      <c r=\"%s\" t=\"s\" s=\"%zu\"><v>%zu</v></c>\n",
                                            cellRef, cellData.m_styleIndex, cellData.m_stringIndex);
                    break;
                    }
                case CellData::CellType::Empty:
                    {
                    // empty cells can still have styles
                    if (cellData.m_styleIndex > 0)
                        {
                        xml += wxString::Format(L"      <c r=\"%s\" s=\"%zu\"/>\n", cellRef,
                                                cellData.m_styleIndex);
                        }
                    break;
                    }
                    }
                }

            xml += L"    </row>\n";
            ++excelRow;
            }

        xml += L"  </sheetData>\n"
               "</worksheet>";

        return xml;
        }

    //------------------------------------------------------
    wxString ListCtrlExcelExporter::BuildSharedStringsXml()
        {
        wxString xml = wxString::Format(
            L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
            "<sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
            "count=\"%zu\" uniqueCount=\"%zu\">\n",
            m_sharedStringsList.size(), m_sharedStringsList.size());

        for (const auto& str : m_sharedStringsList)
            {
            xml += L"  <si><t>" + EscapeXml(str) + L"</t></si>\n";
            }

        xml += L"</sst>";
        return xml;
        }

    //------------------------------------------------------
    wxString ListCtrlExcelExporter::BuildStylesXml()
        {
        wxString xml =
            L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
            "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n";

        // number formats (we'll use built-in formats only)
        xml += L"  <numFmts count=\"0\"/>\n";

        // fonts: collect unique text colors
        std::vector<wxColour> fontColors;
        fontColors.emplace_back(wxColour{}); // default (index 0) - no color specified

        for (const auto& style : m_stylesList)
            {
            if (style.m_textColor.IsOk())
                {
                // check if this color is already in our list
                bool found{ false };
                for (const auto& fc : fontColors)
                    {
                    if (fc.IsOk() && fc.GetRGBA() == style.m_textColor.GetRGBA())
                        {
                        found = true;
                        break;
                        }
                    }
                if (!found)
                    {
                    fontColors.emplace_back(style.m_textColor);
                    }
                }
            }

        xml += wxString::Format(L"  <fonts count=\"%zu\">\n", fontColors.size());
        for (const auto& fontColor : fontColors)
            {
            xml += L"    <font>\n";
            if (fontColor.IsOk())
                {
                xml += wxString::Format(L"      <color rgb=\"%s\"/>\n", ColorToArgb(fontColor));
                }
            // quneiform-suppress-begin
            xml += L"      <name val=\"Calibri\"/>\n"
                   "      <family val=\"2\"/>\n"
                   "    </font>\n";
            // quneiform-suppress-end
            }
        xml += L"  </fonts>\n";

        // fills - collect unique background colors
        std::vector<wxColour> fillColors;
        fillColors.emplace_back(wxColour{}); // default (index 0) - no fill
        fillColors.emplace_back(wxColour{}); // gray125 pattern (index 1) - required by Excel

        for (const auto& style : m_stylesList)
            {
            if (style.m_backgroundColor.IsOk())
                {
                // check if this color is already in our list
                bool found{ false };
                for (const auto& fc : fillColors)
                    {
                    if (fc.IsOk() && fc.GetRGBA() == style.m_backgroundColor.GetRGBA())
                        {
                        found = true;
                        break;
                        }
                    }
                if (!found)
                    {
                    fillColors.emplace_back(style.m_backgroundColor);
                    }
                }
            }

        xml += wxString::Format(L"  <fills count=\"%zu\">\n", fillColors.size());
        // first fill must be "none"
        xml += L"    <fill><patternFill patternType=\"none\"/></fill>\n";
        // second fill must be "gray125"
        xml += L"    <fill><patternFill patternType=\"gray125\"/></fill>\n";
        // additional fills for our colors
        for (size_t i = 2; i < fillColors.size(); ++i)
            {
            xml += wxString::Format(L"    <fill><patternFill patternType=\"solid\">"
                                    "<fgColor rgb=\"%s\"/><bgColor indexed=\"64\"/>"
                                    "</patternFill></fill>\n",
                                    ColorToArgb(fillColors[i]));
            }
        xml += L"  </fills>\n";

        // quneiform-suppress-begin
        // borders - just one default border
        xml += L"  <borders count=\"1\">\n"
               "    <border><left/><right/><top/><bottom/><diagonal/></border>\n"
               "  </borders>\n";

        // cell style xfs (master styles)
        xml += L"  <cellStyleXfs count=\"1\">\n"
               "    <xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/>\n"
               "  </cellStyleXfs>\n";
        // quneiform-suppress-end

        // cell xfs (actual styles used by cells)
        xml += wxString::Format(L"  <cellXfs count=\"%zu\">\n", m_stylesList.size());
        for (const auto& style : m_stylesList)
            {
            // find font index for this style
            size_t fontIndex{ 0 };
            if (style.m_textColor.IsOk())
                {
                for (size_t i = 0; i < fontColors.size(); ++i)
                    {
                    if (fontColors[i].IsOk() &&
                        fontColors[i].GetRGBA() == style.m_textColor.GetRGBA())
                        {
                        fontIndex = i;
                        break;
                        }
                    }
                }

            // find fill index for this style
            size_t fillIndex{ 0 };
            if (style.m_backgroundColor.IsOk())
                {
                for (size_t i = 0; i < fillColors.size(); ++i)
                    {
                    if (fillColors[i].IsOk() &&
                        fillColors[i].GetRGBA() == style.m_backgroundColor.GetRGBA())
                        {
                        fillIndex = i;
                        break;
                        }
                    }
                }

            // get number format ID for this style
            const size_t numFmtId = GetExcelNumberFormatId(style.m_numberFormat);

            xml += wxString::Format(L"    <xf numFmtId=\"%zu\" fontId=\"%zu\" fillId=\"%zu\" "
                                    "borderId=\"0\" xfId=\"0\"",
                                    numFmtId, fontIndex, fillIndex);

            if (numFmtId > 0)
                {
                xml += L" applyNumberFormat=\"1\"";
                }
            if (fontIndex > 0)
                {
                xml += L" applyFont=\"1\"";
                }
            if (fillIndex > 0)
                {
                xml += L" applyFill=\"1\"";
                }
            xml += L"/>\n";
            }
        xml += L"  </cellXfs>\n";

        // quneiform-suppress-begin
        // cell styles
        xml += L"  <cellStyles count=\"1\">\n"
               "    <cellStyle name=\"Normal\" xfId=\"0\" builtinId=\"0\"/>\n"
               "  </cellStyles>\n";
        // quneiform-suppress-end

        xml += L"</styleSheet>";
        return xml;
        }
    } // namespace Wisteria::UI
