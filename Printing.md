Printing
=============================

The `Wisteria::Canvas` object has print support built into it. To initiate a print (or print preview,
under Microsoft Windows) operation, send a `wxID_PRINT` (or `wxID_PREVIEW`) event to a `Wisteria::Canvas` window.
Any objects on the canvas (e.g., plots, legends) will then be printed.

Print Settings
----------------------------
To access and customize a canvas's printer settings, call `Wisteria::Canvas::GetPrinterData()`. This will enable you
to change the orientation and paper size of the printout. (The defaults are U.S. Letter paper size
with portrait orientation.)

Each canvas has its own print settings, allowing for canvases to have different page orientations.

You can also copy print settings into a canvas by calling `Wisteria::Canvas::SetPrinterData()`.

Page Fitting
----------------------------
When printing a canvas, the default is to draw its content as-is onto the paper, maintaining its aspect ratio.
(This aspect ratio is controlled by calling `Wisteria::Canvas::SetCanvasMinWidthDIPs()` and
`Wisteria::Canvas::SetCanvasMinHeightDIPs()`.)

A canvas can also auto-fit to its printer-page size by calling
`Wisteria::Canvas::FitToPageWhenPrinting()`. This results in the canvas temporarily adjusting its
 aspect ratio to match the page and filling it entirely when being printed. If a canvas only
 contains a single graph, then this is only recommended if printing in @c wxLANDSCAPE; otherwise, the graph will
 appear stretched vertically. This can be useful, however, if a canvas contains multiple graphs vertically
 and you are printing in @c wxPORTRAIT; this results in a report-like appearance which more precisely fits the page.

Headers & Footers
----------------------------

Headers and footers can be added to a printout via `Wisteria::Canvas::SetLeftPrinterHeader()`,
`Wisteria::Canvas::SetCenterPrinterHeader()`, `Wisteria::Canvas::SetRightPrinterHeader()`,
`Wisteria::Canvas::SetLeftPrinterFooter()`, `Wisteria::Canvas::SetCenterPrinterFooter()`, and
`Wisteria::Canvas::SetRightPrinterFooter()`.

Headers and footers support placeholder tags for items such as page, date, and time information.
These tags are expanded dynamically when printing takes place. These tags are:

- `@TITLE@`: The title of the printed document.
- `@DATE@`: The date when it was printed.
- `@TIME@`: The time when it was printed.
- `@PAGENUM@`: The current page number.
- `@PAGESCNT@`: The number of printed pages.

Note that a user interface, `PrinterHeaderFooterDlg`, is also available to prompt for headers and footers.

Watermarks
----------------------------

Finally, watermarks can be drawn across the canvas via `Wisteria::Canvas::SetWatermark()`.
Note that the text, color, and direction of this watermark are fully customizable.

Notes
----------------------------
It should be noted that all printer settings are managed on the canvas level, not globally. In other words,
each canvas has its own paper size, header and footers, watermarks, etc. If your application allows customizing
printer settings by the client, then it will need to apply any updates to all active canvases.