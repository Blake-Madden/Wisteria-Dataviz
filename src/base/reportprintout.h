/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_REPORT_PRINTOUT_H
#define WISTERIA_REPORT_PRINTOUT_H

#include "canvas.h"
#include <vector>

namespace Wisteria
    {
    /// @brief Printing interface for reports (i.e., collection of canvases/pages).
    /// @note It is recommended to pass the canvas to a @c wxWindowUpdateLocker
    ///     object prior to this to avoid flickering.
    class ReportPrintout final : public wxPrintout
        {
      public:
        /// @brief Constructor.
        /// @param canvases The vector of canvases (i.e., pages) of the report to print.
        /// @param title The title of the report.
        ReportPrintout(const std::vector<Canvas*>& canvases, const wxString& title)
            : wxPrintout(title), m_canvases(canvases)
            {
            }

        /** @returns @c true if specified page number is within the range of pages being printed.
            @param pageNum The page number to check for.
            @note Page # is 1-indexed.*/
        bool HasPage(int pageNum) noexcept override final
            {
            return (pageNum > 0 && static_cast<size_t>(pageNum) <= m_canvases.size());
            }

        /** @brief Retrieves page information for printing.
            @param[out] minPage The lowest possible page index.
            @param[out] maxPage The highest possible page index.
            @param[out] selPageFrom The starting page.
            @param[out] selPageTo The ending page.*/
        void GetPageInfo(int* minPage, int* maxPage, int* selPageFrom,
                         int* selPageTo) override final
            {
            assert(m_canvases.size() && L"No pages in ReportPrintout!");
            *minPage = (m_canvases.size() ? 1 : 0);
            *maxPage = (m_canvases.size() ? m_canvases.size() : 0);
            *selPageFrom = (m_canvases.size() ? 1 : 0);
            *selPageTo = (m_canvases.size() ? m_canvases.size() : 0);
            }

        /** @brief Prints the specified page number.
            @param page The page to print.
            @returns @c true if printing page was successful.*/
        bool OnPrintPage(int page) override final;

      private:
        /// @returns The margin around the printing area.
        [[nodiscard]]
        wxCoord GetMarginPadding(const size_t pageNumber) const
            {
            if (GetCanvasFromPageNumber(pageNumber) == nullptr)
                {
                return 0;
                }
            return 10 * GetCanvasFromPageNumber(pageNumber)->GetDPIScaleFactor();
            }

        /// @returns A header or footer with dynamic constants expanded in them.
        [[nodiscard]]
        wxString ExpandPrintString(const wxString& printString, const int pageNumber) const;

        /// @brief Gets the canvas associated with a page #.
        /// @details Page numbers are 1-indexed, so we need to take that into account.
        [[nodiscard]]
        Canvas* GetCanvasFromPageNumber(const int pageNumber)
            {
            if (pageNumber <= 0 || static_cast<size_t>(pageNumber - 1) >= m_canvases.size())
                {
                return nullptr;
                }
            return m_canvases.at(static_cast<size_t>(pageNumber - 1));
            }

        /// @private
        [[nodiscard]]
        const Canvas* GetCanvasFromPageNumber(const int pageNumber) const
            {
            if (pageNumber <= 0 || static_cast<size_t>(pageNumber - 1) >= m_canvases.size())
                {
                return nullptr;
                }
            return m_canvases.at(static_cast<size_t>(pageNumber - 1));
            }

        std::vector<Canvas*> m_canvases;
        };

    /// @brief Temporarily changes a canvas's aspect ratio to fit the page when printing.
    class PrintFitToPageChanger
        {
      public:
        /// @brief Constructor, which caches the canvas's aspect ratio and then adjusts
        ///     it to fit the specified printout's paper size.
        /// @param canvas The canvas to adjust.
        /// @param printOut The printout containing the paper size.
        PrintFitToPageChanger(Canvas* canvas, const ReportPrintout* printOut);
        /// @brief Destructor, which resets the canvas back to its original aspect ratio and size.
        /// @private
        ~PrintFitToPageChanger();

      private:
        Canvas* m_canvas{ nullptr };
        int m_originalMinWidth{ 0 };
        int m_originalMinHeight{ 0 };
        wxSize m_originalSize;
        };

    /// @brief Temporarily changes a canvas's size to fit a custom size
    ///     when exporting as an image.
    /// @note It is recommended to pass the canvas to a @c wxWindowUpdateLocker
    ///     object prior to this to avoid flickering.
    /// @warning Caller is responsible for maintaining the aspect ratio;
    ///     the provided size will be explicitly used.
    class FitToSaveOptionsChanger
        {
      public:
        /// @brief Constructor, which caches the canvas's size
        ///     and then adjusts it to fit the specified size.
        /// @param canvas The canvas to adjust.
        /// @param newSize The dimensions (in DIPs) to save the image to.
        FitToSaveOptionsChanger(Canvas* canvas, const wxSize newSize);
        /// @brief Destructor, which resets the canvas back to its original size.
        /// @private
        ~FitToSaveOptionsChanger();

      private:
        Canvas* m_canvas{ nullptr };
        int m_originalMinWidth{ 0 };
        int m_originalMinHeight{ 0 };
        wxSize m_originalSize;
        bool m_sizeChanged{ false };
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_REPORT_PRINTOUT_H
