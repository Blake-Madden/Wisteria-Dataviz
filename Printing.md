Printing
=============================

The `Wisteria::Canvas` object has print support built into it. To initiate a print (or print preview,
under Microsoft Windows) operation, send a `wxID_PRINT` (or `wxID_PREVIEW`) event to a `Wisteria::Canvas` window.
Any objects on the canvas (e.g., plots, legends) will then be printed.

Headers & Footers
----------------------------

To customize the printer settings, call `Wisteria::Canvas::SetPrinterData()`. This will enable you
to change the orientation and paper size of the printout.

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

Finally, watermarks can be drawn across the canvas via `Wisteria::Canvas::DrawWatermarkLabel()`.
Note that the text, color, and direction of this watermark are fully customizable.