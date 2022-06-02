Images
=============================
[TOC]

Wisteria supports using @c wxBitmapBundles for displaying images. Images can be used for
canvases' backgrounds and watermarks, bar charts' bars, stipple brushes, etc.

Also, the class @c Wisteria::GraphItems::Image provides additional image loading and effect functions.
For example, `LoadFile()` will load a JPEG and adjust its orientation (if necessary).

Other features include creating silhouettes, drawing a glassy effect,
filling an area with a stipple bitmap, stitching multiple images together,
changing pixel colors, changing the opacity, etc.

Loading and Creating Images
=============================

The following shows how to load
multiple images at once, piece them together, and then use the results as a canvas background:

```cpp
// set the background from multiple images
canvas->SetBackgroundImage(
        Image::StitchHorizontally({
            Image::LoadFile(L"C:\\Pictures\\IMG_0517.JPG"),
            Image::LoadFile(L"C:\\Pictures\\IMG_0592.JPG"),
            Image::LoadFile(L"C:\\Pictures\\IMG_1288.JPG")
            })
    );
```

As another example, an image file can be loaded, have an effect applied to it, and then
set as a canvas' watermark:

```cpp
// set the watermark from a silhouette of an image
canvas->SetWatermarkLogo(
        Image::CreateSilhouette(
            Image::LoadFile(L"C:\\Pictures\\IMG_0517.JPG")) );
```

Adjusting Image Sizes
=============================

For most purposes, the framework will rescale an image to the appropriate size. For example,
canvas background images will be resized to fit the canvas (while maintaining its aspect ratio).
Likewise, watermarks will be scaled down to 100x100 pixel images in the corner of a canvas.

If you are designing your own plot and plan to use `Image` objects in it, then the functions `SetBestSize()`,
`SetWidth()`, and `SetHeight()` are available for setting the dimensions of an image. Note that when the
image is drawn, these dimensions are adjusted by the image's scaling. If you wish for the image's
dimensions to stay the same, then keep its scaling at 1, instead of setting it to its parent's scaling.

Generally, these sizes are calculated from the area they being drawn on inside of a derived
`RecalcSizes()` call. For example, an image may be consumed 1/4th of the plot area. In this case, the width
may be set to this size and the scaling of the image should just remain the default 1. This way, the parent's
scaling and DPI don't need to be accounted for because you already have the correct pixel width for the image.

This logic can also be used for sizes when constructing an image object via `CreateGlassEffect()` or `CreateSilhouette()`.
(The `BarChart` class demonstrates this.)