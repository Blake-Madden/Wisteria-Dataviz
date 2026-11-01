/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_REPORT_ODP_EXPORT_H
#define WISTERIA_REPORT_ODP_EXPORT_H

#include "../base/canvas.h"
#include "reportslideexportbase.h"
#include <string_view>
#include <vector>
#include <wx/gdicmn.h>
#include <wx/string.h>

namespace Wisteria
    {
    /// @brief Options for LibreOffice Impress (@c .odp) report export.
    struct OdpExportOptions : public SlideExportOptionsBase
        {
        /// @brief The height, in inches, shared by the Standard and Widescreen slide sizes.
        constexpr static double SLIDE_HEIGHT_INCHES{ 7.5 };
        /// @brief The Standard (4:3) slide width, in inches.
        constexpr static double SLIDE_WIDTH_4X3_INCHES{ 10.0 };
        /// @brief The Widescreen (16:9) slide width, in inches.
        constexpr static double SLIDE_WIDTH_16X9_INCHES{ 13.333 };

        /// @returns The slide width in inches.
        [[nodiscard]]
        double GetSlideWidthInches() const;

        /// @returns The slide height in inches.
        [[nodiscard]]
        double GetSlideHeightInches() const;

        /// @returns The slide size in DIPs (96 per inch), used as the render target size.
        [[nodiscard]]
        wxSize GetSlideSizeDIPs() const;
        };

    /// @brief Exports a collection of canvases and tables as slides into a single @c .odp file.
    /// @details List and grid controls become native tables, spanning as many slides as needed.
    class ReportOdpExport : public ReportSlideExportBase
        {
      public:
        /** @brief Constructor. Exports all canvases as slides to an @c .odp file immediately.
            @param canvases The canvases (pages) to export.
            @param filePath The output @c .odp file path.
            @param options The ODP export options.*/
        ReportOdpExport(const std::vector<Canvas*>& canvases, const wxString& filePath,
                        const OdpExportOptions& options = OdpExportOptions{});
        /** @brief Constructor. Exports all pages as slides to an @c .odp file immediately.
            @param exportPages The canvases, list controls, and grid controls to export.
            @param filePath The output @c .odp file path.
            @param options The ODP export options.*/
        ReportOdpExport(const std::vector<ReportExportPage>& exportPages, const wxString& filePath,
                        const OdpExportOptions& options = OdpExportOptions{});

      private:
        /// @brief The ODF package's fixed MIME type.
        constexpr static std::wstring_view MIME_TYPE{
            L"application/vnd.oasis.opendocument.presentation"
        };
        /// @brief The maximum length, in characters, of a slide picture's description.
        constexpr static size_t MAX_ALT_TEXT_LENGTH{ 2000 };

        /// @returns @p value formatted as an ODF length attribute in inches (e.g. @c "7.5in").
        [[nodiscard]]
        static wxString ToInches(double value);
        /// @returns @p color as an ODF @c "#RRGGBB" color attribute value.
        [[nodiscard]]
        static wxString ColorToOdfHex(const wxColour& color);
        /// @returns The manifest listing every part written into the package.
        [[nodiscard]]
        static wxString BuildManifestXml(const std::vector<wxString>& pictureFileNames);
        /// @returns The picture frame's position/size attributes, letterboxed to fit the slide.
        [[nodiscard]]
        static wxString BuildPicturePlacementAttributes(double slideWidthInches,
                                                        double slideHeightInches, int imageWidth,
                                                        int imageHeight);
        /// @returns A slide's @c \<presentation:notes\> content, or empty if @p notes is empty.
        [[nodiscard]]
        static wxString BuildNotesXml(const wxString& notes);
        /// @returns The @c \<draw:page\> transition/auto-advance attributes for @p options,
        ///     or an empty string when there is nothing to emit.
        [[nodiscard]]
        static wxString BuildTransitionAttributes(const OdpExportOptions& options);
        /// @returns The title slide's @c \<draw:page\> content.
        /// @param[in,out] automaticStyles Style definitions the title slide needs are
        ///     appended here.
        [[nodiscard]]
        static wxString BuildTitleSlideXml(const OdpExportOptions& options, double slideWidthInches,
                                           double slideHeightInches,
                                           const wxString& transitionAttributes,
                                           wxString& automaticStyles);
        /// @returns The style definitions shared by all table slides.
        [[nodiscard]]
        static wxString BuildTableStyles();
        /// @returns The table cell style definitions of a slide.
        /// @param cellMarginX The space on the left and right of a cell's text, in inches.
        /// @param slideNumber The number of the slide, which makes its style names unique.
        [[nodiscard]]
        static wxString BuildTableCellStyles(double cellMarginX, size_t slideNumber);
        /// @returns The frames of a slide showing a table.
        /// @param slide The table slide.
        /// @param slideNumber The number of the slide, which makes its style names unique.
        /// @param[in,out] automaticStyles Style definitions the slide needs are appended here.
        [[nodiscard]]
        static wxString BuildTableSlideXml(const TableSlide& slide, size_t slideNumber,
                                           wxString& automaticStyles);
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_REPORT_ODP_EXPORT_H
