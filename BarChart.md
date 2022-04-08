Bar Chart
=============================

Note: this overview is meant for those designing a new chart derived from the `BarChart` class.

The `Wisteria::Graphs::BarChart` class is a highly customizable chart, which is also used as the basis for numerous other graphs.
The following is an explanation of how these are built and how they can be customized to create more complex charts.

If deriving a new graph type from `BarChart`, you can create the bars by calling `AddBar()`. This can either be done when the data
is being set, or inside of a derived call to `RecaleSizes()`. Note that if you derive `RecalcSizes()`, be sure to call
`BarChart::RecalcSizes()` after you are done adding the bars. Also note that `ClearBars()` should be called if you are reconstructing
the bars every time `RecalcSizes()` is called.

The code examples below demonstrate customizing a regular bar chart directly, but if you are deriving a new chart type then
similar code can be done inside of your class (refer to `GanttChart` and `LikertChart` for examples).

Axes & Canvas
=============================

When accessing the axes, prefer using `GetScalingAxis()` and `GetBarAxis()` instead of something like `GetLeftYAxis()`.
`GetScalingAxis()` and `GetBarAxis()` will return the correct axis based on how the chart is oriented (see `BarChart::SetOrientation()`).

Also, when adding a bar to the chart, both the bar and scaling axes will be widened to accommodate it. If you wish to control
the scaling axis manually, then pass `false` as the second argument to `AddBar()`.

Finally, if the number of bars added to the chart read a certain limit, the parent canvas will be enlarged to better display them.
This value is controlled via `SetBarsPerDefaultCanvasSize()` and should be called from your derived class's constructor to
a suitable value.

Bar Blocks
=============================

Rather than being a single box, each bar is constructed from a series of blocks (`Wisteria::Graphs::BarChart::BarBlock`).
This allows for creating categorized bar charts, as well as designing other charts where each block can represent a
different thing (e.g., `Wisteria::Graphs::GanttChart`).

Bar blocks can have their own color, decal (i.e., a label drawn on it), tag (to help identify them) and custom width.
They also be hidden, meaning that section of a bar can be made invisible.

Bar blocks are constructed from a `Wisteria::Graphs::BarChart::BarBlockInfo` object, which offers chainable properties
in a single call (demonstrated later).

Below is an example of creating a bar with three customized blocks:

```cpp
constexpr auto liabilitiesAmount = 8'000'000;
constexpr auto assetsAmount = 2'500'000;
// used to scale the dollar amounts down to
// something reasonable for the axis scale
constexpr auto axisScaleDivisor = 1'000'000;

auto plot = std::make_shared<BarChart>(subframe->m_canvas);
// make it a horizontal barchart
plot->SetOrientation(BarChart::BarOrientation::BarHorizontal);

plot->AddBar(BarChart::Bar(1,
    {
    // first block
    BarChart::BarBlock(
        // the size of the block
        BarChart::BarBlockInfo(liabilitiesAmount/axisScaleDivisor).
        // set its color to orange
        Brush(Colors::ColorBrewer::GetColor(Colors::Color::Orange)).
        // add a decal, which is a label displayed across the block
        Decal(Label(PlotItemInfo(wxString::Format(L"$(%s)",
            wxNumberFormatter::ToString(liabilitiesAmount,0,
                wxNumberFormatter::Style::Style_NoTrailingZeroes |
                wxNumberFormatter::Style::Style_WithThousandsSep))).
            // the decal can be customized, such as its font color
            FontColor(*wxRED).
            // this can control how the decal is positioned on the block
            ChildAlignment(RelativeAlignment::FlushLeft))).
            // an internal tag that can be used to find/edit the block later
            Tag(L"LIABILITIES")),
    // second
    BarChart::BarBlock(
        BarChart::BarBlockInfo(assetsAmount/axisScaleDivisor).
        Brush(Colors::ColorBrewer::GetColor(Colors::Color::LavenderMist)).
        Decal(Label(PlotItemInfo(wxString::Format(L"$%s",
            wxNumberFormatter::ToString(assetsAmount,0,
                wxNumberFormatter::Style::Style_NoTrailingZeroes |
                wxNumberFormatter::Style::Style_WithThousandsSep))).
            Font(plot->GetBarAxis().GetFont()).
            FontColor(*wxBLACK).
            ChildAlignment(RelativeAlignment::FlushRight))).
            // shown by default, just show how we could hide a block
            // on construction
            Show(true)),
    // third block
    BarChart::BarBlock(
        BarChart::BarBlockInfo(2).
        Brush(Colors::ColorBrewer::GetColor(Colors::Color::LightGray)).
        Decal(Label(PlotItemInfo(_(L"Liabilities & Assets comparison")).
            Font(plot->GetBarAxis().GetFont()).
                // this can control how to fit the decal inside of the block
                LabelFitting(LabelFit::SplitTextToFit).
            FontColor(*wxBLACK).
            ChildAlignment(RelativeAlignment::FlushLeft))).
            Tag(L"DECAL_BLOCK")),
    },
    _(""), Label(_("Company Finances")), BarChart::BarEffect::Glassy) );
```

These blocks can later be searched for and customized. In the following example, we customize
the bar blocks to show a healthier view of our company's finances:

```cpp
// hide the liabilities block
auto liabilityBlock = plot->GetBars().at(0).FindBlock(L"LIABILITIES");
if (liabilityBlock != plot->GetBars().at(0).GetBlocks().end())
    { liabilityBlock->Show(false); }

// change the decal on the last block
auto decalBlock = plot->GetBars().at(0).FindBlock(L"DECAL_BLOCK");
if (decalBlock != plot->GetBars().at(0).GetBlocks().end())
    { decalBlock->SetDecal(Label(L"Assets")); }
```

Custom Widths
=============================

By default, bars will have uniform widths along the bar axis.

    |
    |    ___
    |   |   |
    |___|   |
    |   |   |    ___
    |   |   |___|   |
    |   |   |   |   |
    |___|___|___|___|
    1   2   3   4   5

Individual bars can, however, have custom widths that can override this (via `Bar::SetCustomWidth()`).

    |
    |    ___
    |   |   |
    |___|   |
    |   |   |    _______
    |   |   |___|       |
    |   |   |   |       |
    |___|___|___|_______|
    1   2   3   4   5   6

Blocks within the bars can also have custom widths, which will override its parent bar's custom width.

    |
    |    ___
    |   |   |
    |___|   |
    |   |   |    ___
    |   |   |___|   |
    |   |   |   |___|___
    |___|___|___|_______|
    1   2   3   4   5   6

Custom Starting Point
=============================

By default, bars will start from the axis they are connected to. This can be overridden
(via `Bar::SetCustomScalingAxisStartPosition()`) to instead have a bar start further out
from the rest of the bars. As shown below, the last bar starts around 1.5 along the Y axis
(instead of 0):

    3 |
      |            ___
      |    ___    |   |
    2 |   |   |   |   |
      |___|   |   |___|
      |   |   |
    1 |   |   |___
      |   |   |   |
      |___|___|___|____
      1   2   3   4   5