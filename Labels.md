Labels
=============================

Labels are objects used to display text throughout the system. This applies to titles, axis labels, legends,
annotations, decals, etc. Labels are self-contained text boxes that manage attributes such as:

- Text content
- Font
- Font color
- Background color
- Outline
- Text alignment
- Justification
- Line spacing
- Orientation
- Minimum size
- Line decorations
- Margins
- Headers
- Legend attributes

They can be displayed horizontally or vertically (via `Label::SetTextOrientation()`), and support multiline text.

A `Label` is edited mostly through its `GetGraphItemInfo()` method. This returns its `GraphItemInfo`, which can edit
multiple fields through chainable property calls. For example, the following will set a right axis title's font color,
font background color, outline, and text all in one call:

```cpp
theChart->GetRightYAxis().GetTitle().GetGraphItemInfo().FontColor(*wxBLUE).
    FontBackgroundColor(*wxWHITE).Pen(*wxGREEN).Text(_(L"Header"));
```

Reformatting Text
=============================

There are functions available for splitting text to control how it is displayed. For example,
`Label::SplitTextByCharacter()` will insert a newline after each character, causing the text to appear as a vertical column:

    I
    S
    S
    U
    E
    S

This can be useful for Y axis titles, as demonstrated below:

```cpp
// set the left title of a plot to be written downward, letter-by-letter
thePlot->GetLeftYAxis().GetTitle().GetGraphItemInfo().
    Text(L"ISSUES").Orient(Orientation::Horizontal).
    // center the text for the best appearance
    LabelAlignment(TextAlignment::Centered);
thePlot->GetLeftYAxis().GetTitle().SplitTextByCharacter();
```

Another feature is to split the text based on a maximum line length or bounding box. Refer to
`Label::SplitTextToFitLength()` and `Label::SplitTextToFitBoundingBox()` for these features.

Alignment
=============================

The text alignment of the label, as well as its alignment relative to its parent, can be
customized. The following demonstrates left aligning a chart's caption and justifying its content:

```cpp
theChart->GetCaption().GetGraphItemInfo().
    // push the caption to the left of the chart's area
    ChildAlignment(RelativeAlignment::FlushLeft).
    // justify its text
    LabelAlignment(TextAlignment::Justified).
    Text(_(L"Bar widths indicate the proportional number of respondants"));
// split the text so that the justification is enacted
theChart->GetCaption().SplitTextToFitLength(20);
```

If using a minimum user-defined size (see [Uniform Widths](#uniform-widths) below), then the vertical
alignment of a Label can also be controlled via `Label::GetGraphItemInfo().SetPageAlignment()`
(or the `LabelPageAlignment` property when being constructed).

```cpp
Label groupHeader(
        GraphItemInfo(
        L"My Section Header").
        Scaling(GetScaling()).
        Window(GetWindow()).
        // bottom align text within its (minimum-sized bouding box)
        LabelPageVerticalAlignment(PageVerticalAlignment::BottomAligned));
// At least 200 pixels (not DIPs) tall.
// (The minimum user width is left unused with std::nullopt)
groupHeader.SetMinimumUserSize(wxSize(std::nullopt, 200));
```

Line Decorations
=============================

Stylings can be applied to make the label appear like an index card:

    First line
    ---------------
    Second line
    ---------------
    Third line
    ---------------

or dotted lined paper:

    First line
    - - - - - - - -
    Second line
    - - - - - - - -
    Third line
    - - - - - - - -

or even arrowed, lined paper:

    First line
    -------------->
    Second line
    -------------->
    Third line
    -------------->

This is controlled via `Label::SetLabelStyle()`.

Adding a Header
=============================

The first line of a `Label` can be treated as a header, where subsequent lines will appear like a list. For example:

```cpp
theChart->GetTitle().GetHeaderInfo().
    // specify that we want to use the first line as a header
    Enable(true).
    // center the header
    Alignment(TextAlignment::Centered).
    // make the first line (i.e., the header) bold
    GetFont().MakeBold();
theChart->GetTitle().
    SetText(L"REMAINING\n- Critical issues\n- Focus group feedback\n- Help integration");
```

To use this feature, access a label's `GetHeaderInfo()`, enable it, and set any of its properties. Note that a label's
header manages its own properties, so it can have a different font and alignment compared to the rest of the label's content.

This feature can be useful when creating a legend (see below).

Constraining a Label within an Area
=============================

To ensure that a label fits within a given area, use `CalcFontSizeToFitBoundingBox()` to find the proper font size
to make the label fit.

Adding an Annotation
=============================

A label can be added to a plot to act as an annotation. This can be done via `Graph2D::AddEmbeddedObject()`, where
the label will be anchored by the provided data point. (The label's anchoring value will determine how the anchor
point is used). In the following example, a note will be added at the
intersection of 3 (the X axis value) and 59 (the Y axis values):

```cpp
auto note = std::make_shared<Label>(
    GraphItemInfo(L"What happened this week?\nAre we sure this is correct???").
    Pen(*wxLIGHT_GREY).
    FontBackgroundColor(ColorBrewer::GetColor(Color::AntiqueWhite)).
    Anchoring(Anchoring::TopRightCorner).Padding(4, 4, 4, 4));
linePlot->AddEmbeddedObject(note,
    // top corner of note
    wxPoint(3, 38));
```

Optionally, a line from the note can also be drawn to a specific data point:

```cpp
linePlot->AddEmbeddedObject(note,
    // top corner of note
    wxPoint(3, 38),
    // the suspect data point to make the note point to
    wxPoint(4, 59));
```

Building a Legend
=============================

Labels can also be used to construct a legend.

```cpp
auto legend = std::make_shared<GraphItems::Label>(
    // set the legend's text (newlines start a new legend item)
    GraphItemInfo(_(L"Items:\nFirst item\nSecond Item")).
    // don't show an outline around the legend
    Pen(wxNullPen).
    // add space for the icons
    Padding(8, 8, 0, Label::GetMinLegendWidth() );
// set the first line of the text ("Items:"), as the header of the legend
legend->GetHeaderInfo().Alignment(TextAlignment::FlushLeft).Enable(true);
// add icons to the legend
legend->GetLegendIcons().emplace_back(
    LegendIcon(IconShape::AsteriskIcon, *wxBLACK,
        *wxBLACK));
legend->GetLegendIcons().emplace_back(
    LegendIcon(IconShape::CrossIcon, *wxBLACK,
        *wxBLACK));

// "canvas" is the parent canvas that this legend will be added to.
// Canvas::CalcMinWidthProportion() calculates the percent of the canvas
// that the legend will need, and passing this value to SetCanvasWidthProportion()
// will tell the canvas later what this size is.
legend->SetCanvasWidthProportion(canvas->CalcMinWidthProportion(legend));
```

Note that the text for a legend is one string, where each newline indicates a new item in the legend.
If the `Label`'s header is enabled, then the first line will be treated as the header and subsequent
lines as the legend items.

After a legend is constructed, it can be added to a canvas. For example, here we could have a canvas
that will hold a graph and a legend to the right of it:

```cpp
canvas->SetFixedObjectsGridSize(1, 2);
canvas->SetFixedObject(0, 0, graph);

// calculate its required width and add it
legend->SetCanvasWidthProportion(canvas->CalcMinWidthProportion(legend));
canvas->SetFixedObject(0, 1, legend);
```

By default, a legend placed next to a graph will be top aligned inside of its area. To center or
bottom align it, use the legend's `SetPageVerticalAlignment()` method:

```cpp
legend->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
```

Likewise, a legend beneath or above its graph can be horizontally centered or right aligned
via `SetPageHorizontalAlignment()`.

Most graph types have built-in functions to construct a legend, which can be edited prior to moving
into a canvas. If edits are made to the returned legend, be sure to call `Canvas::CalcMinWidthProportion()`
(see above) to recalculate its best fit before adding it to a canvas.

Uniform Widths
=============================

To set a group of labels to a uniform width, call `Label::SetMinimumUserSize()` for each label, passing
in the same size value. This can be useful when lining up a series of labels to appear as a column.

Note that this uniform width will be more noticeable  if the labels are either showing a
background color (via `Label::GetGraphItemInfo().FontBackgroundColor()`) or outline
(via `Label::GetGraphItemInfo().Pen()`).