High DPI Display
=============================

%Wisteria works with either regular or high DPI displays.

If designing your own graph, you will need to set the window for any object that you add to it.
This window will generally be the `Canvas` that the object's plot is being rendered on. For any
`Graph2D` derived graph, you can use the plot's `GetWindow()` method and pass that to the object
when you construct it:

```cpp
Label groupHeader(
        PlotItemInfo(
        L"My Section Header").
        // set the label's scaling to the plot's
        // scaling, which relates to the size or
        // zoom factor of the plot
        Scaling(GetScaling()).
        // set the label's window from the
        // parent plot's canvas parent
        Window(GetWindow()) );
```

Note that the DPI scaling that a plot item uses (based on the parent window) differs from the
general scaling of the object (via `GetScaling()`, `SetScaling()`, and the `Scaling()` property when
being constructed). An items scaling relates scaling of the plot's size, which changes as the window
is resized or zoomed into. An item's scaling can be set to the plot's scaling, but it can also
be set to something else (gernally 1.0) if the size it is constructed with should not be scaled to the
window.

Window DPI scaling, on the other hand, should always be set to any object to ensure that the proper
DPI scale factor is used for it.