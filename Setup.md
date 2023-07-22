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
about the plot (e.g., its current scaling), etc. These features are toggled via preprocessor flags;
refer to `Wisteria::DebugSettings` for an explanation of these flags and which features they enable.

Note that `DEBUG_BOXES`, `DEBUG_FILE_IO`, and `DEBUG_LOG_INFO` are enabled by default if `wxDEBUG_LEVEL` is defined as `2` or higher.
All other features are disabled.