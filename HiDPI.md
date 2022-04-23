High DPI Display
=============================

Wisteria works with either regular or high DPI displays.

If designing your own graph, you may need to set the DPI scaling factor for objects:

```cpp
Label groupHeader(
        GraphItemInfo(
        L"My Section Header").
        // set the label's scaling to the plot's
        // scaling, which relates to the size or
        // zoom factor of the plot
        Scaling(GetScaling()).
        // set the label's window from the
        // parent plot's canvas parent
        DPIScaling(GetDPIScaleFactor()) );
```

Note that the DPI scaling that a plot item uses (based on the current @c wxDC that is drawing or measuring
the object) differs from the general scaling of the object
(via `GetScaling()`, `SetScaling()`, and the `Scaling()` property when being constructed).
An item's scaling relates zoom factor of the plot's size, which changes as the window
is resized or zoomed into. An item's scaling can be set to the plot's scaling, but it can also
be set to something else (generally 1.0) if the size it is constructed with should not be scaled to the
window.

DPI scaling, on the other hand, should always be set to any object to ensure that the proper
DPI scale factor is used for it.

Normally, the framework will handle this for you. For example, `Graph2D::AddObject()` will set the
object's DPI for you. However, if you are using a temporary, stack-based object to measure with
in your calculation code, then you should set the object's DPI scaling to the parent's DPI scaling.