Exporting  {#exporting}
=============================

The `Wisteria::Canvas` object has export support built into it. To initiate an export operation,
send a `wxID_SAVE` event to a `Wisteria::Canvas` window. This will present a list of file formats
to choose from, followed by export options germane to the format that was selected. (For example,
dimension options are always available, but TIFF specific options are only shown if TIFF format
was selected). From there, any objects on the canvas (e.g., plots, legends) will then be exported as an image.

Canvases can also be exported programmatically via `Wisteria::Canvas::Save`. Here, you can pass
the filepath and export settings to use.