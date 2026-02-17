///////////////////////////////////////////////////////////////////////////////
// Name:        chernoffplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "chernoffplot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::ChernoffFacesPlot, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    wxRect ChernoffFacesPlot::FaceObject::GetBoundingBox([[maybe_unused]]
                                                         wxDC &
                                                         dc) const
        {
        const auto scaledSize =
            wxSize(m_size.GetWidth() * GetScaling(), m_size.GetHeight() * GetScaling());

        switch (GetAnchoring())
            {
        case Anchoring::TopLeftCorner:
            return wxRect(GetAnchorPoint(), scaledSize);
        case Anchoring::TopRightCorner:
            return wxRect(wxPoint(GetAnchorPoint().x - scaledSize.GetWidth(), GetAnchorPoint().y),
                          scaledSize);
        case Anchoring::BottomLeftCorner:
            return wxRect(wxPoint(GetAnchorPoint().x, GetAnchorPoint().y - scaledSize.GetHeight()),
                          scaledSize);
        case Anchoring::BottomRightCorner:
            return wxRect(wxPoint(GetAnchorPoint().x - scaledSize.GetWidth(),
                                  GetAnchorPoint().y - scaledSize.GetHeight()),
                          scaledSize);
        case Anchoring::Center:
            [[fallthrough]];
        default:
            return wxRect(wxPoint(GetAnchorPoint().x - scaledSize.GetWidth() / 2,
                                  GetAnchorPoint().y - scaledSize.GetHeight() / 2),
                          scaledSize);
            }
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::FaceObject::SetBoundingBox(
        const wxRect& rect, [[maybe_unused]] wxDC& dc, [[maybe_unused]] double parentScaling)
        {
        SetAnchorPoint(rect.GetTopLeft());
        SetAnchoring(Anchoring::TopLeftCorner);
        m_size = rect.GetSize();
        }

    //----------------------------------------------------------------
    wxRect ChernoffFacesPlot::FaceObject::Draw(wxDC & dc) const
        {
        if (!IsShown())
            {
            return wxRect{};
            }

        const wxRect boundingBox = GetBoundingBox(dc);

        GraphItems::GraphicsContextFallback gcf{ &dc, boundingBox };
        auto* gc = gcf.GetGraphicsContext();
        if (gc != nullptr)
            {
            ChernoffFacesPlot::DrawFace(gc, boundingBox, m_features, m_faceColorLighter,
                                        m_faceColorDarker, m_outlineColor, m_lipstickColor,
                                        m_eyeColor, m_hairColor, m_hairStyle, m_gender);
            }

        return boundingBox;
        }

    //----------------------------------------------------------------
    ChernoffFacesPlot::ChernoffFacesPlot(Canvas * canvas,
                                         const wxColour& faceColor /*= wxColour(255, 224, 189)*/)
        : Graph2D(canvas)
        {
        if (faceColor.IsOk())
            {
            m_faceColor = faceColor;
            }

        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    double ChernoffFacesPlot::NormalizeValue(double value, double minVal, double maxVal) noexcept
        {
        if (!std::isfinite(value) || !std::isfinite(minVal) || !std::isfinite(maxVal))
            {
            return DEFAULT_FEATURE_VALUE;
            }
        if (minVal >= maxVal)
            {
            return DEFAULT_FEATURE_VALUE;
            }
        const auto normalized = safe_divide<double>(value - minVal, maxVal - minVal);
        return std::clamp(normalized, 0.0, 1.0);
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::CalculateColumnRange(const wxString& columnName, double& outMin,
                                                 double& outMax) const
        {
        outMin = std::numeric_limits<double>::max();
        outMax = std::numeric_limits<double>::lowest();

        if (GetDataset() == nullptr || columnName.empty())
            {
            outMin = 0;
            outMax = 1;
            return;
            }

        const auto colIter = GetDataset()->GetContinuousColumn(columnName);
        if (colIter == GetDataset()->GetContinuousColumns().cend())
            {
            outMin = 0;
            outMax = 1;
            return;
            }

        for (const auto& val : colIter->GetValues())
            {
            if (std::isfinite(val))
                {
                outMin = std::min(outMin, val);
                outMax = std::max(outMax, val);
                }
            }

        if (outMin > outMax)
            {
            outMin = 0;
            outMax = 1;
            }
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                    const wxString& faceWidthColumn,
                                    const std::optional<wxString>& faceHeightColumn,
                                    const std::optional<wxString>& eyeSizeColumn,
                                    const std::optional<wxString>& eyePositionColumn,
                                    const std::optional<wxString>& eyebrowSlantColumn,
                                    const std::optional<wxString>& pupilPositionColumn,
                                    const std::optional<wxString>& noseSizeColumn,
                                    const std::optional<wxString>& mouthWidthColumn,
                                    const std::optional<wxString>& mouthCurvatureColumn,
                                    const std::optional<wxString>& faceSaturationColumn,
                                    const std::optional<wxString>& earSizeColumn)
        {
        m_faces.clear();
        SetDataset(data);

        if (GetDataset() == nullptr)
            {
            return;
            }

        // store column names for legend
        m_faceWidthColumnName = faceWidthColumn;
        m_faceHeightColumnName = faceHeightColumn.value_or(wxString{});
        m_eyeSizeColumnName = eyeSizeColumn.value_or(wxString{});
        m_eyePositionColumnName = eyePositionColumn.value_or(wxString{});
        m_eyebrowSlantColumnName = eyebrowSlantColumn.value_or(wxString{});
        m_pupilPositionColumnName = pupilPositionColumn.value_or(wxString{});
        m_noseSizeColumnName = noseSizeColumn.value_or(wxString{});
        m_mouthWidthColumnName = mouthWidthColumn.value_or(wxString{});
        m_mouthCurvatureColumnName = mouthCurvatureColumn.value_or(wxString{});
        m_faceSaturationColumnName = faceSaturationColumn.value_or(wxString{});
        m_earSizeColumnName = earSizeColumn.value_or(wxString{});

        // validate required column
        const auto faceWidthCol = GetDataset()->GetContinuousColumn(faceWidthColumn);
        if (faceWidthCol == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': face width column not found for Chernoff faces plot."),
                                 faceWidthColumn)
                    .ToUTF8());
            }

        // get optional column iterators
        const auto faceHeightCol =
            GetDataset()->GetContinuousColumn(faceHeightColumn.value_or(wxString{}));
        const auto eyeSizeCol =
            GetDataset()->GetContinuousColumn(eyeSizeColumn.value_or(wxString{}));
        const auto eyePositionCol =
            GetDataset()->GetContinuousColumn(eyePositionColumn.value_or(wxString{}));
        const auto eyebrowSlantCol =
            GetDataset()->GetContinuousColumn(eyebrowSlantColumn.value_or(wxString{}));
        const auto pupilPositionCol =
            GetDataset()->GetContinuousColumn(pupilPositionColumn.value_or(wxString{}));
        const auto noseSizeCol =
            GetDataset()->GetContinuousColumn(noseSizeColumn.value_or(wxString{}));
        const auto mouthWidthCol =
            GetDataset()->GetContinuousColumn(mouthWidthColumn.value_or(wxString{}));
        const auto mouthCurvatureCol =
            GetDataset()->GetContinuousColumn(mouthCurvatureColumn.value_or(wxString{}));
        const auto faceSaturationCol =
            GetDataset()->GetContinuousColumn(faceSaturationColumn.value_or(wxString{}));
        const auto earSizeCol =
            GetDataset()->GetContinuousColumn(earSizeColumn.value_or(wxString{}));

        const auto colsEnd = GetDataset()->GetContinuousColumns().cend();

        // calculate ranges for normalization
        double fwMin{ 0.0 }, fwMax{ 0.0 };
        CalculateColumnRange(faceWidthColumn, fwMin, fwMax);

        double fhMin{ 0.0 }, fhMax{ 0.0 };
        CalculateColumnRange(faceHeightColumn.value_or(wxString{}), fhMin, fhMax);

        double esMin{ 0.0 }, esMax{ 0.0 };
        CalculateColumnRange(eyeSizeColumn.value_or(wxString{}), esMin, esMax);

        double epMin{ 0.0 }, epMax{ 0.0 };
        CalculateColumnRange(eyePositionColumn.value_or(wxString{}), epMin, epMax);

        double ebsMin{ 0.0 }, ebsMax{ 0.0 };
        CalculateColumnRange(eyebrowSlantColumn.value_or(wxString{}), ebsMin, ebsMax);

        double ppMin{ 0.0 }, ppMax{ 0.0 };
        CalculateColumnRange(pupilPositionColumn.value_or(wxString{}), ppMin, ppMax);

        double nsMin{ 0.0 }, nsMax{ 0.0 };
        CalculateColumnRange(noseSizeColumn.value_or(wxString{}), nsMin, nsMax);

        double mwMin{ 0.0 }, mwMax{ 0.0 };
        CalculateColumnRange(mouthWidthColumn.value_or(wxString{}), mwMin, mwMax);

        double mcMin{ 0.0 }, mcMax{ 0.0 };
        CalculateColumnRange(mouthCurvatureColumn.value_or(wxString{}), mcMin, mcMax);

        double fsMin{ 0.0 }, fsMax{ 0.0 };
        CalculateColumnRange(faceSaturationColumn.value_or(wxString{}), fsMin, fsMax);

        double erMin{ 0.0 }, erMax{ 0.0 };
        CalculateColumnRange(earSizeColumn.value_or(wxString{}), erMin, erMax);

        // limit observations
        const size_t rowCount = std::min(GetDataset()->GetRowCount(), MAX_FACES);
        if (GetDataset()->GetRowCount() > MAX_FACES)
            {
            wxLogWarning(L"Chernoff plot limited to %zu observations; "
                         L"%zu observations truncated.",
                         MAX_FACES, GetDataset()->GetRowCount() - MAX_FACES);
            }

        m_faces.reserve(rowCount);

        // process each observation
        for (size_t i = 0; i < rowCount; ++i)
            {
            FaceFeatures face;

            face.faceWidth = NormalizeValue(faceWidthCol->GetValue(i), fwMin, fwMax);

            if (faceHeightCol != colsEnd)
                {
                face.faceHeight = NormalizeValue(faceHeightCol->GetValue(i), fhMin, fhMax);
                }
            if (eyeSizeCol != colsEnd)
                {
                face.eyeSize = NormalizeValue(eyeSizeCol->GetValue(i), esMin, esMax);
                }
            if (eyePositionCol != colsEnd)
                {
                face.eyePosition = NormalizeValue(eyePositionCol->GetValue(i), epMin, epMax);
                }
            if (eyebrowSlantCol != colsEnd)
                {
                face.eyebrowSlant = NormalizeValue(eyebrowSlantCol->GetValue(i), ebsMin, ebsMax);
                }
            if (pupilPositionCol != colsEnd)
                {
                face.pupilPosition = NormalizeValue(pupilPositionCol->GetValue(i), ppMin, ppMax);
                }
            if (noseSizeCol != colsEnd)
                {
                face.noseSize = NormalizeValue(noseSizeCol->GetValue(i), nsMin, nsMax);
                }
            if (mouthWidthCol != colsEnd)
                {
                face.mouthWidth = NormalizeValue(mouthWidthCol->GetValue(i), mwMin, mwMax);
                }
            if (mouthCurvatureCol != colsEnd)
                {
                face.mouthCurvature = NormalizeValue(mouthCurvatureCol->GetValue(i), mcMin, mcMax);
                }
            if (faceSaturationCol != colsEnd)
                {
                face.faceSaturation = NormalizeValue(faceSaturationCol->GetValue(i), fsMin, fsMax);
                }
            if (earSizeCol != colsEnd)
                {
                face.earSize = NormalizeValue(earSizeCol->GetValue(i), erMin, erMax);
                }

            // get label from ID column if available
            if (GetDataset()->GetIdColumn().GetRowCount() > i)
                {
                face.label = GetDataset()->GetIdColumn().GetValue(i);
                }

            m_faces.push_back(std::move(face));
            }
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        if (m_faces.empty())
            {
            return;
            }

        const wxRect drawArea = GetPlotAreaBoundingBox();

        // calculate grid dimensions
        const size_t faceCount = m_faces.size();
        const size_t cols =
            static_cast<size_t>(std::ceil(std::sqrt(static_cast<double>(faceCount))));
        const size_t rows = static_cast<size_t>(
            std::ceil(safe_divide<double>(faceCount, static_cast<double>(cols))));

        // calculate cell size, leaving room for labels
        const int labelHeight = m_showLabels ? ScaleToScreenAndCanvas(20) : 0;
        const int cellWidth = safe_divide<int>(drawArea.GetWidth(), static_cast<int>(cols));
        const int cellHeight = safe_divide<int>(drawArea.GetHeight(), static_cast<int>(rows));
        const int faceSize = std::min(cellWidth, cellHeight - labelHeight);

        if (faceSize <= 0)
            {
            return;
            }

        // center the grid
        const auto totalWidth = static_cast<int>(cols) * cellWidth;
        const auto totalHeight = static_cast<int>(rows) * cellHeight;
        const int offsetX = drawArea.GetX() + safe_divide<int>(drawArea.GetWidth() - totalWidth, 2);
        const int offsetY =
            drawArea.GetY() + safe_divide<int>(drawArea.GetHeight() - totalHeight, 2);

        // add each face as a drawable object
        size_t faceIndex = 0;
        for (size_t row = 0; row < rows && faceIndex < faceCount; ++row)
            {
            for (size_t col = 0; col < cols && faceIndex < faceCount; ++col)
                {
                const auto x = offsetX + static_cast<int>(col) * cellWidth +
                               safe_divide<int>(cellWidth - faceSize, 2);
                const auto y = offsetY + static_cast<int>(row) * cellHeight;

                // create face
                AddObject(std::make_unique<FaceObject>(
                    GraphItems::GraphItemInfo{}
                        .Pen(wxNullPen)
                        .Selectable(true)
                        .Anchoring(Anchoring::TopLeftCorner)
                        .AnchorPoint(wxPoint{ x, y }),
                    m_faces[faceIndex], wxSize{ faceSize, faceSize }, m_faceColorLighter,
                    m_faceColor, m_outlineColor, m_lipstickColor, m_eyeColor, m_hairColor,
                    m_hairStyle, m_gender));

                // add label below face if enabled
                if (m_showLabels && !m_faces[faceIndex].label.empty())
                    {
                    auto label = std::make_unique<GraphItems::Label>(
                        GraphItems::GraphItemInfo(m_faces[faceIndex].label)
                            .Pen(wxNullPen)
                            .AnchorPoint(wxPoint{ x, y + faceSize })
                            .Anchoring(Anchoring::TopLeftCorner)
                            .MinimumUserSizeDIPs(faceSize, std::nullopt)
                            .LabelAlignment(TextAlignment::Centered)
                            .LabelPageHorizontalAlignment(PageHorizontalAlignment::Centered)
                            .DPIScaling(GetDPIScaleFactor()));
                    label->GetFont().SetPointSize(
                        std::max(6, static_cast<int>(label->GetFont().GetPointSize() * 0.75)));
                    AddObject(std::move(label));
                    }

                ++faceIndex;
                }
            }
        }

    //----------------------------------------------------------------
    void ChernoffFacesPlot::DrawFace(
        wxGraphicsContext * gc, const wxRect& rect, const FaceFeatures& features,
        const wxColour& faceColorLighter, const wxColour& faceColorDarker,
        const wxColour& outlineColor, const wxColour& lipstickColor, const wxColour& eyeColor,
        const wxColour& hairColor, const HairStyle hairStyle, const Gender gender)
        {
        if (gc == nullptr)
            {
            return;
            }

        // interpolate between lighter and darker skin colors based on saturation
        // 0 = lighter color, 1 = darker color
        const double colorBlendFactor = features.faceSaturation;
        const wxColour faceCol(
            static_cast<unsigned char>(faceColorLighter.Red() +
                                       colorBlendFactor *
                                           (faceColorDarker.Red() - faceColorLighter.Red())),
            static_cast<unsigned char>(faceColorLighter.Green() +
                                       colorBlendFactor *
                                           (faceColorDarker.Green() - faceColorLighter.Green())),
            static_cast<unsigned char>(faceColorLighter.Blue() +
                                       colorBlendFactor *
                                           (faceColorDarker.Blue() - faceColorLighter.Blue())));

        // calculate face dimensions
        const double cx = rect.GetX() + rect.GetWidth() * math_constants::half;
        const double cy = rect.GetY() + rect.GetHeight() * math_constants::half;
        const double baseRadius = std::min(rect.GetWidth(), rect.GetHeight()) * 0.4;

        // face width: 0.7 to 1.3 multiplier
        const double faceWidth = baseRadius * (0.7 + 0.6 * features.faceWidth);
        // face height: 0.8 to 1.2 multiplier
        const double faceHeight = baseRadius * (0.8 + 0.4 * features.faceHeight);

        const wxPen outlinePen{ outlineColor, 1 };

        // draw ears first (behind face)
        const double earScale = math_constants::half + features.earSize;
        const double earWidth = faceHeight * 0.12 * earScale;
        const double earHeight = faceHeight * 0.2 * earScale;

        gc->SetBrush(wxBrush(faceCol));
        gc->SetPen(outlinePen);
        // left ear
        gc->DrawEllipse(cx - faceWidth - earWidth * 0.3, cy - earHeight * 0.5, earWidth, earHeight);
        // right ear
        gc->DrawEllipse(cx + faceWidth - earWidth * 0.7, cy - earHeight * 0.5, earWidth, earHeight);

        // draw face oval
        gc->SetBrush(wxBrush{ faceCol });
        gc->SetPen(outlinePen);
        gc->DrawEllipse(cx - faceWidth, cy - faceHeight, faceWidth * 2, faceHeight * 2);

        // draw rosy cheeks for female faces (radial gradient)
        if (gender == Gender::Female)
            {
            gc->SetPen(*wxTRANSPARENT_PEN);
            const double cheekRadius = faceHeight * 0.225;
            const double cheekY = cy + faceHeight * 0.1;
            const wxColour rosyCenter(255, 150, 170, 80);
            const wxColour rosyEdge(255, 200, 210, 0);

            // left cheek
            const double leftCheekX = cx - faceWidth * 0.55;
            auto leftCheekBrush = gc->CreateRadialGradientBrush(
                leftCheekX, cheekY, leftCheekX, cheekY, cheekRadius, rosyCenter, rosyEdge);
            gc->SetBrush(leftCheekBrush);
            gc->DrawEllipse(leftCheekX - cheekRadius, cheekY - cheekRadius, cheekRadius * 2,
                            cheekRadius * 2);

            // right cheek
            const double rightCheekX = cx + faceWidth * 0.55;
            auto rightCheekBrush = gc->CreateRadialGradientBrush(
                rightCheekX, cheekY, rightCheekX, cheekY, cheekRadius, rosyCenter, rosyEdge);
            gc->SetBrush(rightCheekBrush);
            gc->DrawEllipse(rightCheekX - cheekRadius, cheekY - cheekRadius, cheekRadius * 2,
                            cheekRadius * 2);
            }

        // draw hair over forehead for female (after face, before eyebrows)
        if (gender == Gender::Female && hairStyle != HairStyle::Bald)
            {
            const wxColour hairHighlight = hairColor.ChangeLightness(130);
            const wxColour hairShadow = hairColor.ChangeLightness(80);
            // for dark hair, artists use a deep blue tint for strands instead of black
            const wxColour hairStrandColor =
                Colors::ColorContrast::IsDark(hairColor) ?
                    wxColour(30, 40, 90, 50) : // deep indigo-blue for dark hair
                    wxColour(0, 0, 0, 40);     // dark strands for light hair

            // calculate where eyebrows will be drawn (to stop hair above them)
            const double eyeYPreCalc = cy - faceHeight * (0.15 + 0.2 * features.eyePosition);
            const double eyeRadiusPreCalc = faceHeight * 0.1 * (0.6 + 0.8 * features.eyeSize);
            const double browYLimit = eyeYPreCalc - eyeRadiusPreCalc * 1.8 - faceHeight * 0.05;

            if (hairStyle == HairStyle::Bob)
                {
                // bob: hair frames face with bangs covering forehead
                wxGraphicsPath bobHair = gc->CreatePath();
                // outer edge - start from bottom left, go up and around
                bobHair.MoveToPoint(cx - faceWidth * 0.92, cy + faceHeight * 0.6);
                bobHair.AddQuadCurveToPoint(cx - faceWidth * 0.98, cy + faceHeight * 0.2,
                                            cx - faceWidth * 0.98, cy - faceHeight * 0.3);
                bobHair.AddQuadCurveToPoint(cx - faceWidth * 1.0, cy - faceHeight * 0.98, cx,
                                            cy - faceHeight * 1.03);
                bobHair.AddQuadCurveToPoint(cx + faceWidth * 1.0, cy - faceHeight * 0.98,
                                            cx + faceWidth * 0.98, cy - faceHeight * 0.3);
                bobHair.AddQuadCurveToPoint(cx + faceWidth * 0.98, cy + faceHeight * 0.2,
                                            cx + faceWidth * 0.92, cy + faceHeight * 0.6);
                // inner edge - cut out for face, going back up
                bobHair.AddLineToPoint(cx + faceWidth * 0.82, cy + faceHeight * 0.48);
                bobHair.AddQuadCurveToPoint(cx + faceWidth * 0.88, cy + faceHeight * 0.1,
                                            cx + faceWidth * 0.85, browYLimit + faceHeight * 0.15);
                // bangs across forehead - asymmetric sweep (higher on right, lower on left)
                bobHair.AddQuadCurveToPoint(cx + faceWidth * 0.3, browYLimit - faceHeight * 0.06,
                                            cx, browYLimit);
                bobHair.AddQuadCurveToPoint(cx - faceWidth * 0.4, browYLimit + faceHeight * 0.04,
                                            cx - faceWidth * 0.72, browYLimit + faceHeight * 0.06);
                bobHair.AddQuadCurveToPoint(cx - faceWidth * 0.92, browYLimit + faceHeight * 0.02,
                                            cx - faceWidth * 0.88, cy + faceHeight * 0.1);
                bobHair.AddQuadCurveToPoint(cx - faceWidth * 0.86, cy + faceHeight * 0.25,
                                            cx - faceWidth * 0.82, cy + faceHeight * 0.48);
                bobHair.CloseSubpath();

                gc->SetBrush(wxBrush(hairColor));
                gc->SetPen(wxPen(hairShadow, 1));
                gc->FillPath(bobHair);
                gc->StrokePath(bobHair);

                // curved sheen following top curve of hair
                wxGraphicsPath bobHairSheen = gc->CreatePath();
                // outer arc (follows hair outline)
                bobHairSheen.MoveToPoint(cx - faceWidth * 0.75, cy - faceHeight * 0.5);
                bobHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.6, cy - faceHeight * 0.9,
                                                 cx - faceWidth * 0.1, cy - faceHeight * 0.98);
                // inner arc (parallel, inside the hair)
                bobHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.5, cy - faceHeight * 0.8,
                                                 cx - faceWidth * 0.65, cy - faceHeight * 0.5);
                bobHairSheen.CloseSubpath();
                auto bobSheenBrush = gc->CreateLinearGradientBrush(
                    cx - faceWidth * 0.5, cy - faceHeight * 0.95, cx - faceWidth * 0.5,
                    cy - faceHeight * 0.6, hairHighlight,
                    wxColour(hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0));
                gc->SetBrush(bobSheenBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->FillPath(bobHairSheen);

                // hair strands for texture
                gc->SetPen(wxPen(hairStrandColor, 1));
                // top/crown strands - stay well above bangs line
                const double strandBottomLimit = browYLimit - faceHeight * 0.08;
                for (int s = 0; s < 6; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx - faceWidth * 0.4 + s * faceWidth * 0.16;
                    const double topY = cy - faceHeight * 1.0;
                    strand.MoveToPoint(startX, topY);
                    strand.AddQuadCurveToPoint(startX + faceWidth * 0.2, topY + faceHeight * 0.25,
                                               startX + faceWidth * 0.1, strandBottomLimit);
                    gc->StrokePath(strand);
                    }
                // left outer side strands - stay within hair boundary, curve outward
                for (int s = 0; s < 4; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx - faceWidth * 0.88;
                    const double startY = cy - faceHeight * 0.4 + s * faceHeight * 0.22;
                    strand.MoveToPoint(startX, startY);
                    strand.AddQuadCurveToPoint(
                        startX - faceWidth * 0.15, startY + faceHeight * 0.12,
                        startX - faceWidth * 0.08, startY + faceHeight * 0.28);
                    gc->StrokePath(strand);
                    }
                // right outer side strands - stay within hair boundary, curve outward
                for (int s = 0; s < 4; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx + faceWidth * 0.88;
                    const double startY = cy - faceHeight * 0.4 + s * faceHeight * 0.22;
                    strand.MoveToPoint(startX, startY);
                    strand.AddQuadCurveToPoint(
                        startX + faceWidth * 0.15, startY + faceHeight * 0.12,
                        startX + faceWidth * 0.08, startY + faceHeight * 0.28);
                    gc->StrokePath(strand);
                    }
                }
            else if (hairStyle == HairStyle::Pixie)
                {
                // pixie: short textured hair covering forehead
                wxGraphicsPath pixieHair = gc->CreatePath();
                // outer edge
                pixieHair.MoveToPoint(cx - faceWidth * 0.82, cy - faceHeight * 0.3);
                pixieHair.AddQuadCurveToPoint(cx - faceWidth * 0.92, cy - faceHeight * 0.95, cx,
                                              cy - faceHeight * 1.05);
                pixieHair.AddQuadCurveToPoint(cx + faceWidth * 0.92, cy - faceHeight * 0.95,
                                              cx + faceWidth * 0.82, cy - faceHeight * 0.3);
                // inner edge with textured bangs
                pixieHair.AddQuadCurveToPoint(cx + faceWidth * 0.55, browYLimit - faceHeight * 0.05,
                                              cx + faceWidth * 0.28,
                                              browYLimit + faceHeight * 0.02);
                pixieHair.AddQuadCurveToPoint(cx + faceWidth * 0.1, browYLimit - faceHeight * 0.03,
                                              cx - faceWidth * 0.1, browYLimit + faceHeight * 0.03);
                pixieHair.AddQuadCurveToPoint(cx - faceWidth * 0.38, browYLimit - faceHeight * 0.02,
                                              cx - faceWidth * 0.82, cy - faceHeight * 0.3);
                pixieHair.CloseSubpath();

                gc->SetBrush(wxBrush(hairColor));
                gc->SetPen(wxPen(hairShadow, 1));
                gc->FillPath(pixieHair);
                gc->StrokePath(pixieHair);

                // curved sheen following top curve of hair
                wxGraphicsPath pixieHairSheen = gc->CreatePath();
                // outer arc (follows hair outline)
                pixieHairSheen.MoveToPoint(cx - faceWidth * 0.65, cy - faceHeight * 0.5);
                pixieHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.55, cy - faceHeight * 0.88,
                                                   cx - faceWidth * 0.1, cy - faceHeight * 1.0);
                // inner arc (parallel, inside the hair)
                pixieHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.45, cy - faceHeight * 0.78,
                                                   cx - faceWidth * 0.55, cy - faceHeight * 0.5);
                pixieHairSheen.CloseSubpath();
                auto pixieSheenBrush = gc->CreateLinearGradientBrush(
                    cx - faceWidth * 0.45, cy - faceHeight * 0.95, cx - faceWidth * 0.45,
                    cy - faceHeight * 0.6, hairHighlight,
                    wxColour(hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0));
                gc->SetBrush(pixieSheenBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->FillPath(pixieHairSheen);

                // hair strands for texture - short wispy strands
                gc->SetPen(wxPen(hairStrandColor, 1));
                // top strands radiating outward with more arc
                for (int s = 0; s < 7; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double offsetX = -0.5 + s * 0.17;
                    const double startX = cx + faceWidth * offsetX;
                    const double startY = cy - faceHeight * 0.98;
                    strand.MoveToPoint(startX, startY);
                    strand.AddQuadCurveToPoint(
                        startX + faceWidth * 0.25 * offsetX, startY + faceHeight * 0.25,
                        startX + faceWidth * 0.15 * offsetX, browYLimit + faceHeight * 0.01);
                    gc->StrokePath(strand);
                    }
                // additional crown strands with strong arc
                for (int s = 0; s < 5; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx - faceWidth * 0.3 + s * faceWidth * 0.15;
                    strand.MoveToPoint(startX, cy - faceHeight * 1.0);
                    strand.AddQuadCurveToPoint(startX + faceWidth * 0.2, cy - faceHeight * 0.65,
                                               startX + faceWidth * 0.08,
                                               browYLimit - faceHeight * 0.02);
                    gc->StrokePath(strand);
                    }
                }
            else if (hairStyle == HairStyle::LongStraight)
                {
                // long straight: flowing hair with side-swept bangs
                wxGraphicsPath longHair = gc->CreatePath();
                // outer edge - start bottom left, go up and around, down to bottom right
                longHair.MoveToPoint(cx - faceWidth * 0.95, cy + faceHeight * 0.95);
                longHair.AddQuadCurveToPoint(cx - faceWidth * 1.0, cy + faceHeight * 0.4,
                                             cx - faceWidth * 0.98, cy - faceHeight * 0.3);
                longHair.AddQuadCurveToPoint(cx - faceWidth * 1.0, cy - faceHeight * 0.98, cx,
                                             cy - faceHeight * 1.03);
                longHair.AddQuadCurveToPoint(cx + faceWidth * 1.0, cy - faceHeight * 0.98,
                                             cx + faceWidth * 0.98, cy - faceHeight * 0.3);
                longHair.AddQuadCurveToPoint(cx + faceWidth * 1.0, cy + faceHeight * 0.4,
                                             cx + faceWidth * 0.95, cy + faceHeight * 0.95);
                // inner cutout for face - go back up
                longHair.AddLineToPoint(cx + faceWidth * 0.82, cy + faceHeight * 0.75);
                longHair.AddQuadCurveToPoint(cx + faceWidth * 0.88, cy + faceHeight * 0.25,
                                             cx + faceWidth * 0.85, browYLimit + faceHeight * 0.12);
                // side-swept bangs
                longHair.AddQuadCurveToPoint(cx + faceWidth * 0.38, browYLimit - faceHeight * 0.02,
                                             cx, browYLimit + faceHeight * 0.04);
                longHair.AddQuadCurveToPoint(cx - faceWidth * 0.45, browYLimit + faceHeight * 0.02,
                                             cx - faceWidth * 0.85, browYLimit + faceHeight * 0.08);
                longHair.AddQuadCurveToPoint(cx - faceWidth * 0.88, cy + faceHeight * 0.25,
                                             cx - faceWidth * 0.82, cy + faceHeight * 0.75);
                longHair.CloseSubpath();

                gc->SetBrush(wxBrush(hairColor));
                gc->SetPen(wxPen(hairShadow, 1));
                gc->FillPath(longHair);
                gc->StrokePath(longHair);

                // curved sheen following top curve of hair
                wxGraphicsPath longHairSheen = gc->CreatePath();
                // outer arc (follows hair outline)
                longHairSheen.MoveToPoint(cx - faceWidth * 0.75, cy - faceHeight * 0.5);
                longHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.6, cy - faceHeight * 0.9,
                                                  cx - faceWidth * 0.1, cy - faceHeight * 0.98);
                // inner arc (parallel, inside the hair)
                longHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.5, cy - faceHeight * 0.8,
                                                  cx - faceWidth * 0.65, cy - faceHeight * 0.5);
                longHairSheen.CloseSubpath();
                auto longSheenBrush = gc->CreateLinearGradientBrush(
                    cx - faceWidth * 0.5, cy - faceHeight * 0.95, cx - faceWidth * 0.5,
                    cy - faceHeight * 0.6, hairHighlight,
                    wxColour(hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0));
                gc->SetBrush(longSheenBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->FillPath(longHairSheen);

                // hair strands for texture - long flowing strands
                gc->SetPen(wxPen(hairStrandColor, 1));
                // top/crown strands flowing down with arc
                for (int s = 0; s < 6; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx - faceWidth * 0.5 + s * faceWidth * 0.2;
                    strand.MoveToPoint(startX, cy - faceHeight * 0.98);
                    strand.AddQuadCurveToPoint(startX + faceWidth * 0.18, cy - faceHeight * 0.4,
                                               startX + faceWidth * 0.1,
                                               browYLimit + faceHeight * 0.03);
                    gc->StrokePath(strand);
                    }
                // bangs strands - side swept with more arc
                for (int s = 0; s < 6; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx - faceWidth * 0.5 + s * faceWidth * 0.2;
                    strand.MoveToPoint(startX, cy - faceHeight * 0.92);
                    strand.AddQuadCurveToPoint(
                        startX + faceWidth * 0.22, browYLimit - faceHeight * 0.12,
                        startX + faceWidth * 0.15, browYLimit + faceHeight * 0.05);
                    gc->StrokePath(strand);
                    }
                // outer edge strands (on the hair portion outside the face)
                for (int side = -1; side <= 1; side += 2)
                    {
                    for (int s = 0; s < 3; ++s)
                        {
                        wxGraphicsPath strand = gc->CreatePath();
                        const double startX = cx + side * faceWidth * (0.92 + s * 0.03);
                        const double startY = cy - faceHeight * (0.5 - s * 0.2);
                        strand.MoveToPoint(startX, startY);
                        strand.AddQuadCurveToPoint(
                            startX + side * faceWidth * 0.12, startY + faceHeight * 0.4,
                            startX + side * faceWidth * 0.05, cy + faceHeight * 0.85);
                        gc->StrokePath(strand);
                        }
                    }
                }
            else if (hairStyle == HairStyle::Bun)
                {
                // bun: hair pulled back smoothly with bun on top (no bangs)
                wxGraphicsPath bunHair = gc->CreatePath();
                // outer edge
                bunHair.MoveToPoint(cx - faceWidth * 0.85, cy - faceHeight * 0.15);
                bunHair.AddQuadCurveToPoint(cx - faceWidth * 0.92, cy - faceHeight * 0.9, cx,
                                            cy - faceHeight * 1.0);
                bunHair.AddQuadCurveToPoint(cx + faceWidth * 0.92, cy - faceHeight * 0.9,
                                            cx + faceWidth * 0.85, cy - faceHeight * 0.15);
                // inner edge - follows hairline, no bangs
                bunHair.AddQuadCurveToPoint(cx + faceWidth * 0.55, cy - faceHeight * 0.45, cx,
                                            cy - faceHeight * 0.5);
                bunHair.AddQuadCurveToPoint(cx - faceWidth * 0.55, cy - faceHeight * 0.45,
                                            cx - faceWidth * 0.85, cy - faceHeight * 0.15);
                bunHair.CloseSubpath();

                gc->SetBrush(wxBrush(hairColor));
                gc->SetPen(wxPen(hairShadow, 1));
                gc->FillPath(bunHair);
                gc->StrokePath(bunHair);

                // curved sheen following top curve of hair
                wxGraphicsPath bunHairSheen = gc->CreatePath();
                // outer arc (follows hair outline)
                bunHairSheen.MoveToPoint(cx - faceWidth * 0.7, cy - faceHeight * 0.55);
                bunHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.55, cy - faceHeight * 0.85,
                                                 cx - faceWidth * 0.1, cy - faceHeight * 0.95);
                // inner arc (parallel, inside the hair)
                bunHairSheen.AddQuadCurveToPoint(cx - faceWidth * 0.45, cy - faceHeight * 0.75,
                                                 cx - faceWidth * 0.6, cy - faceHeight * 0.55);
                bunHairSheen.CloseSubpath();
                auto bunHairSheenBrush = gc->CreateLinearGradientBrush(
                    cx - faceWidth * 0.5, cy - faceHeight * 0.9, cx - faceWidth * 0.5,
                    cy - faceHeight * 0.6, hairHighlight,
                    wxColour(hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0));
                gc->SetBrush(bunHairSheenBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->FillPath(bunHairSheen);

                // the bun itself
                const double bunRadius = faceHeight * 0.18;
                const double bunX = cx;
                const double bunY = cy - faceHeight * 1.05;
                gc->DrawEllipse(bunX - bunRadius, bunY - bunRadius, bunRadius * 2, bunRadius * 2);

                // sheen on bun
                auto bunSheen =
                    gc->CreateRadialGradientBrush(bunX - bunRadius * 0.3, bunY - bunRadius * 0.3,
                                                  bunX, bunY, bunRadius, hairHighlight, hairColor);
                gc->SetBrush(bunSheen);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawEllipse(bunX - bunRadius, bunY - bunRadius, bunRadius * 2, bunRadius * 2);

                // outline the bun
                gc->SetPen(wxPen(hairShadow, 1));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawEllipse(bunX - bunRadius, bunY - bunRadius, bunRadius * 2, bunRadius * 2);

                // hair strands for texture - swept back look
                gc->SetPen(wxPen(hairStrandColor, 1));
                // crown strands sweeping back toward bun with strong arc
                for (int s = 0; s < 6; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double startX = cx - faceWidth * 0.4 + s * faceWidth * 0.16;
                    const double startY = cy - faceHeight * 0.48;
                    strand.MoveToPoint(startX, startY);
                    strand.AddQuadCurveToPoint(startX + faceWidth * 0.15, cy - faceHeight * 0.75,
                                               cx, cy - faceHeight * 0.95);
                    gc->StrokePath(strand);
                    }
                // side strands curving up toward bun
                for (int side = -1; side <= 1; side += 2)
                    {
                    for (int s = 0; s < 3; ++s)
                        {
                        wxGraphicsPath strand = gc->CreatePath();
                        const double startX = cx + side * faceWidth * (0.82 - s * 0.06);
                        const double startY = cy - faceHeight * (0.18 + s * 0.1);
                        strand.MoveToPoint(startX, startY);
                        strand.AddQuadCurveToPoint(
                            startX + side * faceWidth * 0.15, cy - faceHeight * 0.55,
                            cx + side * faceWidth * 0.1, cy - faceHeight * 0.9);
                        gc->StrokePath(strand);
                        }
                    }
                // strands on the bun itself - spiral pattern with more curve
                for (int s = 0; s < 6; ++s)
                    {
                    wxGraphicsPath strand = gc->CreatePath();
                    const double angle = -1.0 + s * 0.5;
                    const double innerR = bunRadius * 0.2;
                    const double outerR = bunRadius * 0.9;
                    strand.MoveToPoint(bunX + innerR * std::cos(angle),
                                       bunY + innerR * std::sin(angle));
                    strand.AddQuadCurveToPoint(bunX + bunRadius * 0.7 * std::cos(angle + 0.5),
                                               bunY + bunRadius * 0.7 * std::sin(angle + 0.5),
                                               bunX + outerR * std::cos(angle + 0.8),
                                               bunY + outerR * std::sin(angle + 0.8));
                    gc->StrokePath(strand);
                    }
                }
            }
        // draw hair for male faces - high top fade
        else if (gender == Gender::Male && hairStyle == HairStyle::HighTopFade)
            {
            const wxColour hairHighlight = hairColor.ChangeLightness(130);
            const wxColour hairShadow = hairColor.ChangeLightness(80);

            // thin arc of hair on top - connects to head at sides
            wxGraphicsPath hair = gc->CreatePath();
            // outer edge - starts where head curve is, follows around
            hair.MoveToPoint(cx - faceWidth * 0.95, cy - faceHeight * 0.3);
            hair.AddCurveToPoint(cx - faceWidth * 1.0, cy - faceHeight * 0.7, cx - faceWidth * 0.55,
                                 cy - faceHeight * 1.05, cx, cy - faceHeight * 1.08);
            hair.AddCurveToPoint(cx + faceWidth * 0.55, cy - faceHeight * 1.05,
                                 cx + faceWidth * 1.0, cy - faceHeight * 0.7, cx + faceWidth * 0.95,
                                 cy - faceHeight * 0.3);
            // inner edge - higher hairline showing forehead
            hair.AddCurveToPoint(cx + faceWidth * 0.9, cy - faceHeight * 0.6, cx + faceWidth * 0.5,
                                 cy - faceHeight * 0.85, cx, cy - faceHeight * 0.88);
            hair.AddCurveToPoint(cx - faceWidth * 0.5, cy - faceHeight * 0.85, cx - faceWidth * 0.9,
                                 cy - faceHeight * 0.6, cx - faceWidth * 0.95,
                                 cy - faceHeight * 0.3);
            hair.CloseSubpath();

            gc->SetBrush(wxBrush(hairColor));
            gc->SetPen(wxPen(hairShadow, 1));
            gc->FillPath(hair);
            gc->StrokePath(hair);

            // curved sheen
            wxGraphicsPath sheen = gc->CreatePath();
            sheen.MoveToPoint(cx - faceWidth * 0.7, cy - faceHeight * 0.65);
            sheen.AddQuadCurveToPoint(cx - faceWidth * 0.4, cy - faceHeight * 0.95,
                                      cx - faceWidth * 0.05, cy - faceHeight * 1.0);
            sheen.AddQuadCurveToPoint(cx - faceWidth * 0.35, cy - faceHeight * 0.88,
                                      cx - faceWidth * 0.6, cy - faceHeight * 0.65);
            sheen.CloseSubpath();

            auto sheenBrush = gc->CreateLinearGradientBrush(
                cx - faceWidth * 0.4, cy - faceHeight * 0.98, cx - faceWidth * 0.4,
                cy - faceHeight * 0.75, hairHighlight,
                wxColour(hairColor.Red(), hairColor.Green(), hairColor.Blue(), 0));
            gc->SetBrush(sheenBrush);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->FillPath(sheen);
            }

        // draw eyes
        const double eyeY = cy - faceHeight * (0.15 + 0.2 * features.eyePosition);
        const double eyeSpacing = faceWidth * 0.4;
        const double eyeRadius = faceHeight * 0.1 * (0.6 + 0.8 * features.eyeSize);

        // sclera (white of eye)
        gc->SetBrush(*wxWHITE_BRUSH);
        gc->SetPen(outlinePen);
        // left eye
        gc->DrawEllipse(cx - eyeSpacing - eyeRadius, eyeY - eyeRadius, eyeRadius * 2,
                        eyeRadius * 1.6);
        // right eye
        gc->DrawEllipse(cx + eyeSpacing - eyeRadius, eyeY - eyeRadius, eyeRadius * 2,
                        eyeRadius * 1.6);

        // iris (colored part) and pupils
        const double pupilOffset = (features.pupilPosition - 0.5) * eyeRadius * 0.6;
        const double irisRadius = eyeRadius * 0.55;
        const double pupilRadius = eyeRadius * 0.25;

        // draw iris
        gc->SetBrush(wxBrush{ eyeColor });
        gc->SetPen(*wxTRANSPARENT_PEN);
        // left iris
        gc->DrawEllipse(cx - eyeSpacing + pupilOffset - irisRadius, eyeY - irisRadius * 0.9,
                        irisRadius * 2, irisRadius * 2);
        // right iris
        gc->DrawEllipse(cx + eyeSpacing + pupilOffset - irisRadius, eyeY - irisRadius * 0.9,
                        irisRadius * 2, irisRadius * 2);

        // draw pupils (black center)
        gc->SetBrush(*wxBLACK_BRUSH);
        // left pupil
        gc->DrawEllipse(cx - eyeSpacing + pupilOffset - pupilRadius, eyeY - pupilRadius * 0.9,
                        pupilRadius * 2, pupilRadius * 2);
        // right pupil
        gc->DrawEllipse(cx + eyeSpacing + pupilOffset - pupilRadius, eyeY - pupilRadius * 0.9,
                        pupilRadius * 2, pupilRadius * 2);

        // draw feminine eyelashes
        if (gender == Gender::Female)
            {
            gc->SetPen(wxPen{ outlineColor, 1 });
            const double lashLength = eyeRadius * 0.4;
            const double eyeTop = eyeY - eyeRadius * 0.8;

            // draw 3 lashes per eye, angled outward
            for (int i = 0; i < 3; ++i)
                {
                const double offset = (i - 1) * eyeRadius * 0.5;
                const double angle = (i - 1) * 0.3; // angle outward

                // left eye lashes
                wxGraphicsPath leftLash = gc->CreatePath();
                leftLash.MoveToPoint(cx - eyeSpacing + offset, eyeTop);
                leftLash.AddLineToPoint(cx - eyeSpacing + offset - angle * lashLength,
                                        eyeTop - lashLength);
                gc->StrokePath(leftLash);

                // right eye lashes
                wxGraphicsPath rightLash = gc->CreatePath();
                rightLash.MoveToPoint(cx + eyeSpacing + offset, eyeTop);
                rightLash.AddLineToPoint(cx + eyeSpacing + offset + angle * lashLength,
                                         eyeTop - lashLength);
                gc->StrokePath(rightLash);
                }
            }

        // draw eyebrows using lines
        gc->SetPen(wxPen{ outlineColor, 2 });
        const double browY = eyeY - eyeRadius * 1.8;
        const double browSlant = (features.eyebrowSlant - 0.5) * eyeRadius * 0.8;
        const double browLength = eyeRadius * 1.2;

        wxGraphicsPath leftBrow = gc->CreatePath();
        leftBrow.MoveToPoint(cx - eyeSpacing - browLength, browY + browSlant);
        leftBrow.AddLineToPoint(cx - eyeSpacing + browLength, browY - browSlant);
        gc->StrokePath(leftBrow);

        wxGraphicsPath rightBrow = gc->CreatePath();
        rightBrow.MoveToPoint(cx + eyeSpacing - browLength, browY - browSlant);
        rightBrow.AddLineToPoint(cx + eyeSpacing + browLength, browY + browSlant);
        gc->StrokePath(rightBrow);

        // draw nose (simple line with small base)
        gc->SetPen(outlinePen);
        const double noseScale = 0.6 + 0.8 * features.noseSize;
        const double noseLength = faceHeight * 0.25 * noseScale;
        const double noseWidth = faceWidth * 0.08 * noseScale;
        const double noseTop = cy - faceHeight * 0.05;
        const double noseBottom = noseTop + noseLength;

        wxGraphicsPath nose = gc->CreatePath();
        nose.MoveToPoint(cx, noseTop);
        nose.AddLineToPoint(cx, noseBottom);
        nose.AddLineToPoint(cx - noseWidth, noseBottom);
        nose.MoveToPoint(cx, noseBottom);
        nose.AddLineToPoint(cx + noseWidth, noseBottom);
        gc->StrokePath(nose);

        // draw mouth using quadratic curve
        const double mouthY = cy + faceHeight * 0.45;
        const double mouthWidthVal = faceWidth * 0.35 * (0.5 + features.mouthWidth);
        // curvature: 0=frown, 0.5=neutral, 1=smile
        const double curvature = (features.mouthCurvature - 0.5) * faceHeight * 0.2;

        if (gender == Gender::Female)
            {
            // draw lips with lipstick
            gc->SetBrush(wxBrush{ lipstickColor });
            gc->SetPen(wxPen{ lipstickColor.ChangeLightness(70), 1 });

            // ensure lips have fullness even when mouth is straight
            const double lipFullness = faceHeight * 0.06;
            const double upperLipHeight = faceHeight * 0.045;
            const double lowerLipDepth = std::max(lipFullness, std::abs(curvature) + lipFullness);

            // upper lip with cupid's bow
            wxGraphicsPath upperLip = gc->CreatePath();
            upperLip.MoveToPoint(cx - mouthWidthVal, mouthY);
            upperLip.AddQuadCurveToPoint(cx - mouthWidthVal * 0.5, mouthY - upperLipHeight, cx,
                                         mouthY - upperLipHeight * 0.5);
            upperLip.AddQuadCurveToPoint(cx + mouthWidthVal * 0.5, mouthY - upperLipHeight,
                                         cx + mouthWidthVal, mouthY);
            upperLip.AddQuadCurveToPoint(cx, mouthY + curvature * 0.3 + lipFullness * 0.3,
                                         cx - mouthWidthVal, mouthY);
            gc->FillPath(upperLip);
            gc->StrokePath(upperLip);

            // lower lip
            wxGraphicsPath lowerLip = gc->CreatePath();
            lowerLip.MoveToPoint(cx - mouthWidthVal, mouthY);
            lowerLip.AddQuadCurveToPoint(cx, mouthY + curvature + lowerLipDepth, cx + mouthWidthVal,
                                         mouthY);
            lowerLip.AddQuadCurveToPoint(cx, mouthY + curvature * 0.3 + lipFullness * 0.3,
                                         cx - mouthWidthVal, mouthY);
            gc->FillPath(lowerLip);
            gc->StrokePath(lowerLip);
            }
        else
            {
            // male: simple line mouth
            gc->SetPen(wxPen{ outlineColor, 2 });
            wxGraphicsPath mouth = gc->CreatePath();
            mouth.MoveToPoint(cx - mouthWidthVal, mouthY);
            mouth.AddQuadCurveToPoint(cx, mouthY + curvature, cx + mouthWidthVal, mouthY);
            gc->StrokePath(mouth);
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> ChernoffFacesPlot::CreateLegend(const LegendOptions& options)
        {
        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo{}
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .DPIScaling(GetDPIScaleFactor())
                .FontColor(GetLeftYAxis().GetFontColor()));

        wxString legendText;

        // add header if requested
        if (options.IsIncludingHeader())
            {
            legendText = _(L"Features:\n");
            legend->GetLinesIgnoringLeftMargin().insert(0);
            legend->GetHeaderInfo().Enable(true);
            }

        const auto addFeatureEntry = [&](const wxString& featureName, const wxString& columnName)
        {
            if (!columnName.empty())
                {
                legendText.append(wxString::Format(L"%s: %s\n", featureName, columnName));
                legend->GetLegendIcons().emplace_back(Icons::IconShape::HorizontalLine,
                                                      wxPen(m_outlineColor),
                                                      wxBrush(m_outlineColor));
                }
        };

        addFeatureEntry(_(L"Face width"), m_faceWidthColumnName);
        addFeatureEntry(_(L"Face height"), m_faceHeightColumnName);
        addFeatureEntry(_(L"Eye size"), m_eyeSizeColumnName);
        addFeatureEntry(_(L"Eye position"), m_eyePositionColumnName);
        addFeatureEntry(_(L"Eyebrow slant"), m_eyebrowSlantColumnName);
        addFeatureEntry(_(L"Pupil direction"), m_pupilPositionColumnName);
        addFeatureEntry(_(L"Nose size"), m_noseSizeColumnName);
        addFeatureEntry(_(L"Mouth width"), m_mouthWidthColumnName);
        addFeatureEntry(_(L"Smile/frown"), m_mouthCurvatureColumnName);
        addFeatureEntry(_(L"Face color"), m_faceSaturationColumnName);
        addFeatureEntry(_(L"Ear size"), m_earSizeColumnName);

        legend->SetText(legendText.Trim());
        AdjustLegendSettings(*legend, options.GetPlacementHint());

        return legend;
        }
    } // namespace Wisteria::Graphs
