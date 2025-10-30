Axes
=============================

Customizing a Graph's Axes
=============================

A graph’s axes can be directly accessed via `GetBottomXAxis()`, `GetTopXAxis()`, `GetLeftYAxis()`, or `GetRightYAxis()`.
These functions return a `Wisteria::GraphItems::Axis` object, where you can edit its range, fonts, labels, titles, etc.

By default, the right Y and top X axes are blank. However, you can tell the plot to have them display the same content as
their parallel axis via `MirrorXAxis()` and `MirrorYAxis()`. Or you can copy the content from the parallel axis and make further
edits (e.g., using different custom axis labels).  For example:

```cpp
theChart->GetRightYAxis().CopySettings(theChart->GetLeftYAxis());
theChart->GetRightYAxis().ClearCustomLabels();
theChart->GetRightYAxis().SetCustomLabel(50, Label(L"Baseline"));
theChart->GetRightYAxis().Show();
```

Headers & Footers
=============================

Along with labels running along the length of axis, headers and footers can also be added. For vertical
axes, headers will appear above the axis and drawn perpendicular to it. Likewise, footers will appear
below a vertical axis. For horizontal axes, headers are drawn to the right of an axis and footers to the left,
and both are drawn parallel to the axis.

    HEADER
      |
      |
      |
      |
      |
      |
      |
    FOOTER     FOOTER --------------- HEADER

To edit an axis's header or footer, call `GetHeader()` or `GetFooter()`. This will return a reference to
the header/footer as a `Wisteria::GraphItems::Label` object, which you can then fully edit. Here, you can set the text,
change the font size, font color, text alignment, etc.

For example, an axis's header or footer can be aligned with the axis line via `SetRelativeAlignment()`:

```cpp
theChart->GetRightYAxis().GetHeader().
    SetRelativeAlignment(RelativeAlignment::FlushRight);
theChart->GetRightYAxis().GetFooter().
    SetRelativeAlignment(RelativeAlignment::FlushLeft);
```

    HEADER (FLUSH RIGHT)
                       |
                       |
                       |
                       |
                       |
                       |
                       |
                       FOOTER (FLUSH LEFT)

Refer to the [label](Labels.md) overview for more about how to edit labels.

Tickmark Labels
=============================

The tickmark labels along the axis can be customized in a number of ways:

- Font attributes
- Orientation (relative to the axis line)
- Alignment with the respective point
- Alignment with the other labels
- Which type of label to display (e.g., custom text, underlying value, no display at all)

Orientation
-----------------------------

Tickmark labels can be oriented via `SetAxisLabelOrientation()`, where the orientation
is in reference to the axis line. Below are labels that are perpendicular with its axis line.

     OVER |
          |
          |
          |
          |
    UNDER |

Whereas these are labels parallel to their axis line:

    ______________________________
    UNDER                     OVER

Alignment
-----------------------------

For tickmark labels that are parallel with the axis line, you can specify how to align
them with their respective points via `SetParallelLabelAlignment()`.

     ____________________________                             ____________________________
    |                            |                           |                            |
    FLUSH LEFT                   FLUSH LEFT        FLUSH RIGHT                  FLUSH RIGHT

For tickmark labels that are perpendicular with the axis line, you can specify how to align
them with respect to the other labels via `SetPerpendicularLabelAxisAlignment()`.

       TOP FLUSH RIGHT |       TOP FLUSH LEFT     |
                       |                          |
                       |                          |
                       |                          |
                       |                          |
    BOTTOM FLUSH RIGHT |        BOTTOM FLUSH LEFT |

Label Stacking
-----------------------------

Axes support label stacking, where if any labels overlap each other, then every other label will be drawn
on a second line:

    |
    |
    |
    |
    |
    |
    |
    |
    |
    |       |       |       |
    |_________________________________
    FIRST GRADE        THIRD GRADE
               SECOND GRADE

This behavior is enabled by default for horizontal axes, but not vertical ones. (This feature usually would
not be necessary for vertical axes unless they are using parallel alignment or huge fonts.) This feature can
be toggled via `EnableAutoStacking()`.

Logarithmic Scales
=============================

Logarithmic (or uneven interval) scales can be achieved via `Axis::AddUnevenAxisPoint()`. For example, you can
create a scale where the intervals differ (e.g., 1, 5, 15, 50, 100), but the tickmarks are spaced
equidistantly. For example, this:

```cpp
theChart->GetAxisPoints().clear(); // clear current scale
theChart->GetLeftYAxis().AddUnevenAxisPoint(1, L"1");
theChart->GetLeftYAxis().AddUnevenAxisPoint(5, L"5");
theChart->GetLeftYAxis().AddUnevenAxisPoint(15, L"15");
theChart->GetLeftYAxis().AddUnevenAxisPoint(50, L"50");
theChart->GetLeftYAxis().AddUnevenAxisPoint(100, L"100");
theChart->GetLeftYAxis().AdjustRangeToLabels();
```

will produce this:

    100 -|
         |
     50 -|
         |
     15 -|
         |
      5 -|
         |
      1 -|
         ---------------------

Date Scales
=============================

A date-based axis can be created via the version of `SetRange()` which accepts a start and end date.
Along with the dates, a date interval can be specified (e.g., monthly, fiscal year quarters) to control
which dates to display.

If using fiscal year related intervals, the fiscal year's date range can be customized via `SetRange()`'s `FiscalYear`
parameter. This parameter provides a list of pre-defined types to control the fiscal year's start and end dates.

Finally, for certain types of date-based axes, `AddBrackets()` can add a series of pre-defined brackets showing
smaller date blocks along the axis. For example, this can be used to show the start and end of each quarter of
the axis's fiscal years.

Dual Axes
=============================

Dual axes can be created, where one axis shows one scale and the opposite-side axis shows a different scale.
This can be useful to display, for example, Celsius vs. Fahrenheit or Metric vs. Imperial measurements.

    C  100 -|                   | - 212  F
    E       |                   |        A
    L   75 -|                   | - 167  H
    S       |                   |        R
    I   50 -|                   |        E
    U       |                   | - 98   N
    S   25 -|                   |        H
            |                   |        E
         0 -|                   | - 32   I
            ---------------------        T

```cpp
// set the range of the Celsius scale, going from 0-100
// with major tickmarks every 25 degrees
linePlot->GetLeftYAxis().GetTitle().SetText(L"\u00B0Celsius");
linePlot->GetLeftYAxis().SetRange(0, 100, 0, 25, 1);
// ensure that the left axis isn't copying over the opposite axis
linePlot->MirrorYAxis(false);

// get the range of the Celsius axis and then fill opposite axis
// with the corresponding Fahrenheit values
const auto cel2fahr = [](double cel)
    { return (cel * 9.0 / 5.0) + 32; };
const auto [celRangeStart, celRangeEnd] = linePlot->GetLeftYAxis().GetRange();
linePlot->GetRightYAxis().GetTitle().SetText(L"\u00B0Fahrenheit");
// Note that you must call this overload of SetRange(), passing false
// for the includeExtraInterval argument. This will force the axis
// to use the provided range exactly as-is (without any adjustment),
// thus ensuring that the two axes line up as expected.
linePlot->GetRightYAxis().SetRange(
    cel2fahr(celRangeStart), cel2fahr(celRangeEnd), 0, false);
```

Brackets
=============================

Brackets can be drawn along an axis, enabling you to label sections of it.

       __
      |     1 |
    A |       |
      |       |
      |--   2 |
      |       |
    B |       |
      |       |
      |--   3 |
      |       |
    C |       |
      |       |
      |__   4 |

Brackets can be added to an axis via `Axis::AddBracket()`. The following example will
add a bracket to the left axis where a bracket is drawn from 90-100, with the label "Complex" at position 95
(relative to the parent axis).

```cpp
theChart->GetLeftYAxis().AddBracket(
    Axis::AxisBracket(100, 90, 95, _(L"Complex"), wxColour(66,51,251)));
```

Adding a Custom Axis
=============================

Custom axes can also be added to a graph and customized via `AddCustomAxis()` and `GetCustomAxes()`.

To build a custom axis, first an axis is constructed with a type. For example, to make a vertical custom axis,
set its type to `Wisteria::AxisType::LeftYAxis`. Vertical custom axis are anchored to the bottom X axis, while
horizontal custom axes will be anchored to the left Y axis.

Next, you will need to specify where the custom axis connects with its parent axis. For vertical custom axis, this
is done via `SetCustomXPosition()`. Finally, the ending point of the custom axis must be specified. For a vertical
custom axis this is done via `SetCustomYPosition()`. For example, if the left Y axis goes from 0 (its origin) to 100
and the custom axis calls `SetCustomYPosition(75)`, then it will run parallel with the Y axis going from 0-75.

    100 |
        |
        |
        |         CUSTOM
     75 |           |
        |           |
        |           |
        |           |
     50 |           |
        |           |
        |           |
        |           |
     25 |           |
        |           |
        |           |
        |           |
      1 |___________|_______________
        1    2    3    4    5    6

After the positions of the custom axis are established, then the range, tickmarks, labels, etc. can be customized.

Common Axis
=============================

A helper class, `Wisteria::CommonAxisBuilder`, is available for creating a shared axis for multiple graphs. This
class will take a list of graphs from the same row or column and normalize them to use the same scale. Then it will
turn off the graph's axis labels. Finally, it will return a new axis that can be added next to graphs on the canvas.

```cpp
// with "canvas" being the canvas containing your graphs and
// "linePlot" and "boxPlot" being two graphs in the canvas's
// first row:
canvas->SetFixedObject(0, 2,
    // construct a common axis connected to the line and box plots,
    // and add it to the right of them on the canvas
    CommonAxisBuilder::BuildYAxis(canvas,
        { linePlot, boxPlot }, AxisType::RightYAxis));
```