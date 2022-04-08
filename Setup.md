Setting up the Library
=============================

There are a few library and related wxWidgts features that can be enabled when your application starts up.

Locale Formatting
=============================

The framework will format numbers using the locale set by `wxUILocale`. For example, to use the
system's current locale, run the following in your application's start up:

```cpp
wxUILocale::UseDefault();
```

Image Handling
=============================

To enable image importing and exporting, call the following in your application's start up:

```cpp
wxInitAllImageHandlers();
```

Debug Information
=============================

If designing your own graph type, drawing debug information can be helpful. For example, you can
enable features such as drawing bounding boxes when you select items, showing useful information
about the plot (e.g., its current scaling), etc. To control this, call
`Wisteria::Settings::EnableDebugFlag()` to toggle specific options, or
`Wisteria::Settings::EnableAllDebugFlags()` to enable all debugging features.

Note that `DrawBoundingBoxesOnSelection` is enabled by default if `wxDEBUG_LEVEL` is defined as 2 or higher. 