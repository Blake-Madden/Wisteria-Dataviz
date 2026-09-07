///////////////////////////////////////////////////////////////////////////////
// Name:        geodatasettests.cpp
// Purpose:     Unit tests for KmlReader, GeoJsonReader, and GeoDataset
// Author:      Blake Madden
// Copyright:   (c) 2026 Blake Madden
// License:     3-Clause BSD license
///////////////////////////////////////////////////////////////////////////////

#include "../../src/data/geodataset.h"
#include "../../src/data/geojsonreader.h"
#include "../../src/data/kmlreader.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using namespace Wisteria::Data;
using namespace Catch::Matchers;

// A small, self-contained KML with the shapes the reader has to cope with:
// a bare <Polygon>, a <MultiGeometry>, a polygon with a hole, and a
// <Point>-only placemark that has no fillable area.
static const wxString SAMPLE_KML = LR"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
<Document>
<name>Test Counties</name>
<Folder>
<Placemark>
<name>Alpha</name>
<ExtendedData>
<SchemaData schemaUrl="#s">
<SimpleData name="GEOID">3900001</SimpleData>
<SimpleData name="NAME">Alpha</SimpleData>
<SimpleData name="ALAND">1000</SimpleData>
</SchemaData>
</ExtendedData>
<Polygon><outerBoundaryIs><LinearRing><coordinates>
-84.0,40.0,0.0 -83.0,40.0,0.0 -83.0,41.0,0.0 -84.0,41.0,0.0 -84.0,40.0,0.0
</coordinates></LinearRing></outerBoundaryIs></Polygon>
</Placemark>
<Placemark>
<name>Beta</name>
<ExtendedData>
<SchemaData schemaUrl="#s">
<SimpleData name="GEOID">3900002</SimpleData>
<SimpleData name="NAME">Beta</SimpleData>
<SimpleData name="ALAND">2000</SimpleData>
</SchemaData>
</ExtendedData>
<MultiGeometry>
<Polygon><outerBoundaryIs><LinearRing><coordinates>
-82.0,40.0,0.0 -81.0,40.0,0.0 -81.0,41.0,0.0 -82.0,41.0,0.0 -82.0,40.0,0.0
</coordinates></LinearRing></outerBoundaryIs></Polygon>
<Polygon><outerBoundaryIs><LinearRing><coordinates>
-80.5,40.0,0.0 -80.0,40.0,0.0 -80.0,40.5,0.0 -80.5,40.5,0.0 -80.5,40.0,0.0
</coordinates></LinearRing></outerBoundaryIs></Polygon>
</MultiGeometry>
</Placemark>
<Placemark>
<name>Gamma</name>
<ExtendedData>
<SchemaData schemaUrl="#s">
<SimpleData name="GEOID">3900003</SimpleData>
<SimpleData name="NAME">Gamma</SimpleData>
<SimpleData name="ALAND">3000</SimpleData>
</SchemaData>
</ExtendedData>
<Polygon>
<outerBoundaryIs><LinearRing><coordinates>
-90.0,30.0,0.0 -88.0,30.0,0.0 -88.0,32.0,0.0 -90.0,32.0,0.0 -90.0,30.0,0.0
</coordinates></LinearRing></outerBoundaryIs>
<innerBoundaryIs><LinearRing><coordinates>
-89.5,30.5,0.0 -88.5,30.5,0.0 -88.5,31.5,0.0 -89.5,31.5,0.0 -89.5,30.5,0.0
</coordinates></LinearRing></innerBoundaryIs>
</Polygon>
</Placemark>
<Placemark>
<name>PointOnly</name>
<Point><coordinates>-85.0,39.0,0.0</coordinates></Point>
</Placemark>
</Folder>
</Document>
</kml>)";

// The same three counties as SAMPLE_KML, expressed as GeoJSON: a Polygon, a
// MultiPolygon, a Polygon with a hole, and a Point-only feature that has no
// fillable area. GEOID is a JSON number, to exercise numeric-property formatting.
static const wxString SAMPLE_GEOJSON = LR"({
"type": "FeatureCollection",
"name": "Test Counties",
"features": [
  {
    "type": "Feature",
    "properties": { "GEOID": 3900001, "NAME": "Alpha", "ALAND": 1000 },
    "geometry": {
      "type": "Polygon",
      "coordinates": [ [ [-84.0,40.0], [-83.0,40.0], [-83.0,41.0], [-84.0,41.0], [-84.0,40.0] ] ]
    }
  },
  {
    "type": "Feature",
    "properties": { "GEOID": 3900002, "NAME": "Beta", "ALAND": 2000 },
    "geometry": {
      "type": "MultiPolygon",
      "coordinates": [
        [ [ [-82.0,40.0], [-81.0,40.0], [-81.0,41.0], [-82.0,41.0], [-82.0,40.0] ] ],
        [ [ [-80.5,40.0], [-80.0,40.0], [-80.0,40.5], [-80.5,40.5], [-80.5,40.0] ] ]
      ]
    }
  },
  {
    "type": "Feature",
    "properties": { "GEOID": 3900003, "NAME": "Gamma", "ALAND": 3000 },
    "geometry": {
      "type": "Polygon",
      "coordinates": [
        [ [-90.0,30.0], [-88.0,30.0], [-88.0,32.0], [-90.0,32.0], [-90.0,30.0] ],
        [ [-89.5,30.5], [-88.5,30.5], [-88.5,31.5], [-89.5,31.5], [-89.5,30.5] ]
      ]
    }
  },
  {
    "type": "Feature",
    "properties": { "GEOID": 3900004, "NAME": "PointOnly" },
    "geometry": { "type": "Point", "coordinates": [-85.0,39.0] }
  }
]
})";

TEST_CASE("KmlReader parsing", "[kml][geodataset]")
    {
    SECTION("Load from text")
        {
        KmlReader reader;
        REQUIRE(reader.LoadText(SAMPLE_KML));
        CHECK(reader.IsOk());
        CHECK(reader.GetLastError().empty());
        CHECK(reader.GetName() == wxString(L"Test Counties"));
        // the point-only placemark is dropped
        REQUIRE(reader.GetRegions().size() == 3);
        }

    SECTION("Bad content fails cleanly")
        {
        KmlReader reader;
        CHECK_FALSE(reader.LoadText(L"not xml at all"));
        CHECK_FALSE(reader.IsOk());
        CHECK_FALSE(reader.GetLastError().empty());
        CHECK(reader.GetRegions().empty());
        }

    SECTION("Labels and attributes")
        {
        KmlReader reader;
        REQUIRE(reader.LoadText(SAMPLE_KML));

        const auto alphaRow = reader.FindRegion(L"Alpha");
        REQUIRE(alphaRow.has_value());
        const GeoRegion& alpha = reader.GetRegions()[*alphaRow];
        CHECK(alpha.m_name == wxString(L"Alpha"));
        CHECK(alpha.GetAttribute(L"GEOID") == wxString(L"3900001"));
        CHECK(alpha.GetAttribute(L"ALAND") == wxString(L"1000"));
        CHECK(alpha.GetAttribute(L"missing", L"n/a") == wxString(L"n/a"));
        }

    SECTION("Bare polygon geometry")
        {
        KmlReader reader;
        REQUIRE(reader.LoadText(SAMPLE_KML));

        const auto alphaRow = reader.FindRegion(L"Alpha");
        REQUIRE(alphaRow.has_value());
        const GeoRegion& alpha = reader.GetRegions()[*alphaRow];
        REQUIRE(alpha.m_polygons.size() == 1);
        // the ring is closed, so 5 points
        CHECK(alpha.m_polygons[0].m_outerBoundary.size() == 5);
        CHECK(alpha.m_polygons[0].m_innerBoundaries.empty());
        CHECK_THAT(alpha.m_polygons[0].m_outerBoundary[0].m_longitude, WithinAbs(-84.0, 1e-9));
        CHECK_THAT(alpha.m_polygons[0].m_outerBoundary[0].m_latitude, WithinAbs(40.0, 1e-9));

        const auto& box = alpha.m_boundingBox;
        REQUIRE(box.IsOk());
        CHECK_THAT(box.m_minLongitude, WithinAbs(-84.0, 1e-9));
        CHECK_THAT(box.m_maxLongitude, WithinAbs(-83.0, 1e-9));
        CHECK_THAT(box.m_minLatitude, WithinAbs(40.0, 1e-9));
        CHECK_THAT(box.m_maxLatitude, WithinAbs(41.0, 1e-9));
        CHECK_THAT(box.GetWidth(), WithinAbs(1.0, 1e-9));
        CHECK_THAT(box.GetHeight(), WithinAbs(1.0, 1e-9));
        }

    SECTION("MultiGeometry yields several polygons")
        {
        KmlReader reader;
        REQUIRE(reader.LoadText(SAMPLE_KML));

        const auto betaRow = reader.FindRegion(L"Beta");
        REQUIRE(betaRow.has_value());
        CHECK(reader.GetRegions()[*betaRow].m_polygons.size() == 2);
        }

    SECTION("Inner boundary is kept as a hole")
        {
        KmlReader reader;
        REQUIRE(reader.LoadText(SAMPLE_KML));

        const auto gammaRow = reader.FindRegion(L"Gamma");
        REQUIRE(gammaRow.has_value());
        const GeoRegion& gamma = reader.GetRegions()[*gammaRow];
        REQUIRE(gamma.m_polygons.size() == 1);
        REQUIRE(gamma.m_polygons[0].m_innerBoundaries.size() == 1);
        CHECK(gamma.m_polygons[0].m_innerBoundaries[0].size() == 5);
        }

    SECTION("Overall bounding box covers every region")
        {
        KmlReader reader;
        REQUIRE(reader.LoadText(SAMPLE_KML));

        const auto& box = reader.GetBoundingBox();
        REQUIRE(box.IsOk());
        CHECK_THAT(box.m_minLongitude, WithinAbs(-90.0, 1e-9));
        CHECK_THAT(box.m_maxLongitude, WithinAbs(-80.0, 1e-9));
        CHECK_THAT(box.m_minLatitude, WithinAbs(30.0, 1e-9));
        CHECK_THAT(box.m_maxLatitude, WithinAbs(41.0, 1e-9));
        }

    SECTION("Out-of-range coordinates are held to the valid globe")
        {
        const wxString kml = LR"(<?xml version="1.0"?>
<kml><Document><Placemark><name>Wild</name>
<Polygon><outerBoundaryIs><LinearRing><coordinates>
999,999 -999,40 -84,999 -84,40 999,999
</coordinates></LinearRing></outerBoundaryIs></Polygon>
</Placemark></Document></kml>)";
        KmlReader reader;
        REQUIRE(reader.LoadText(kml));
        const auto wildRow = reader.FindRegion(L"Wild");
        REQUIRE(wildRow.has_value());
        const GeoRegion& wild = reader.GetRegions()[*wildRow];
        REQUIRE(wild.m_polygons.size() == 1);
        for (const auto& coordinate : wild.m_polygons[0].m_outerBoundary)
            {
            CHECK(coordinate.m_longitude >= -180.0);
            CHECK(coordinate.m_longitude <= 180.0);
            CHECK(coordinate.m_latitude >= -90.0);
            CHECK(coordinate.m_latitude <= 90.0);
            }
        CHECK(reader.GetBoundingBox().IsOk());
        }

    SECTION("Non-numeric coordinate tuples are skipped")
        {
        const wxString kml = LR"(<?xml version="1.0"?>
<kml><Document><Placemark><name>Partial</name>
<Polygon><outerBoundaryIs><LinearRing><coordinates>
-84,40 bogus,tuple -83,40 -83,41 -84,41 -84,40
</coordinates></LinearRing></outerBoundaryIs></Polygon>
</Placemark></Document></kml>)";
        KmlReader reader;
        REQUIRE(reader.LoadText(kml));
        const auto partialRow = reader.FindRegion(L"Partial");
        REQUIRE(partialRow.has_value());
        const GeoRegion& partial = reader.GetRegions()[*partialRow];
        REQUIRE(partial.m_polygons.size() == 1);
        // the five well-formed tuples are kept, the garbage one dropped
        CHECK(partial.m_polygons[0].m_outerBoundary.size() == 5);
        }

    SECTION("Deeply nested folders do not overflow the stack")
        {
        wxString kml{ LR"(<?xml version="1.0"?><kml><Document>)" };
        constexpr int nestingDepth{ 4000 };
        for (int level = 0; level < nestingDepth; ++level)
            {
            kml += L"<Folder>";
            }
        kml += LR"(<Placemark><name>Deep</name>
<Polygon><outerBoundaryIs><LinearRing><coordinates>
-84,40 -83,40 -83,41 -84,41 -84,40
</coordinates></LinearRing></outerBoundaryIs></Polygon></Placemark>)";
        for (int level = 0; level < nestingDepth; ++level)
            {
            kml += L"</Folder>";
            }
        kml += L"</Document></kml>";

        // the tree walk stops descending past its depth cap, so a placemark buried
        // deeper than that is not found; the point of the test is that the reader
        // returns instead of crashing
        KmlReader reader;
        reader.LoadText(kml);
        SUCCEED("nested folder walk returned without a stack overflow");
        }
    }

TEST_CASE("GeoDataset import", "[kml][geodataset]")
    {
    SECTION("Rows and geometry stay aligned")
        {
        auto geoData = std::make_shared<GeoDataset>();
        REQUIRE(geoData->ImportRegionsFromText(SAMPLE_KML, GeoImportInfo().IdField(L"GEOID")));

        REQUIRE(geoData->GetRowCount() == 3);
        REQUIRE(geoData->GetGeometries().size() == 3);
        for (size_t row = 0; row < geoData->GetRowCount(); ++row)
            {
            CHECK(geoData->GetRegionGeometry(row).m_name == geoData->GetGeometries()[row].m_name);
            }
        }

    SECTION("ID column comes from the requested field")
        {
        auto geoData = std::make_shared<GeoDataset>();
        REQUIRE(geoData->ImportRegionsFromText(SAMPLE_KML, GeoImportInfo().IdField(L"GEOID")));

        CHECK(geoData->GetIdColumn().GetValue(0) == wxString(L"3900001"));
        const auto row = geoData->FindRegionRow(L"3900002");
        REQUIRE(row.has_value());
        CHECK(geoData->GetRegionGeometry(row.value()).m_name == wxString(L"Beta"));
        }

    SECTION("Numeric fields become continuous columns")
        {
        auto geoData = std::make_shared<GeoDataset>();
        REQUIRE(geoData->ImportRegionsFromText(SAMPLE_KML, GeoImportInfo().IdField(L"GEOID")));

        const auto alandColumn = geoData->GetContinuousColumn(L"ALAND");
        REQUIRE(alandColumn != geoData->GetContinuousColumns().cend());
        CHECK_THAT(alandColumn->GetValue(0), WithinAbs(1000.0, 1e-9));
        CHECK_THAT(alandColumn->GetValue(2), WithinAbs(3000.0, 1e-9));

        // NAME is text, so it is categorical
        CHECK(geoData->GetCategoricalColumn(L"NAME") != geoData->GetCategoricalColumns().cend());
        }

    SECTION("Centroid columns are added by default")
        {
        auto geoData = std::make_shared<GeoDataset>();
        REQUIRE(geoData->ImportRegionsFromText(SAMPLE_KML, GeoImportInfo()));

        const auto lonColumn = geoData->GetContinuousColumn(L"Centroid Longitude");
        const auto latColumn = geoData->GetContinuousColumn(L"Centroid Latitude");
        REQUIRE(lonColumn != geoData->GetContinuousColumns().cend());
        REQUIRE(latColumn != geoData->GetContinuousColumns().cend());
        // Alpha spans -84..-83 lon, 40..41 lat
        CHECK_THAT(lonColumn->GetValue(0), WithinAbs(-83.5, 1e-9));
        CHECK_THAT(latColumn->GetValue(0), WithinAbs(40.5, 1e-9));
        }

    SECTION("A forced categorical field is not treated as numeric")
        {
        auto geoData = std::make_shared<GeoDataset>();
        REQUIRE(geoData->ImportRegionsFromText(
            SAMPLE_KML, GeoImportInfo().IdField(L"GEOID").CategoricalFields({ L"ALAND" })));

        CHECK(geoData->GetContinuousColumn(L"ALAND") == geoData->GetContinuousColumns().cend());
        CHECK(geoData->GetCategoricalColumn(L"ALAND") != geoData->GetCategoricalColumns().cend());
        }

    SECTION("Importing twice into the same object fails")
        {
        auto geoData = std::make_shared<GeoDataset>();
        REQUIRE(geoData->ImportRegionsFromText(SAMPLE_KML, GeoImportInfo()));
        CHECK_FALSE(geoData->ImportRegionsFromText(SAMPLE_KML, GeoImportInfo()));
        CHECK_FALSE(geoData->GetLastError().empty());
        }
    }

TEST_CASE("GeoDataset::CopyContinuousColumnFrom", "[kml][geodataset]")
    {
    auto geoData = std::make_shared<GeoDataset>();
    REQUIRE(geoData->ImportRegionsFromText(SAMPLE_KML, GeoImportInfo().IdField(L"GEOID")));

    // a plain metric table keyed by the same GEOID identifier
    Dataset metrics;
    metrics.GetIdColumn().SetName(L"county_id");
    metrics.AddContinuousColumn(L"grad_rate");
    metrics.AddRow(RowInfo().Id(L"3900001").Continuous({ 0.91 }));
    metrics.AddRow(RowInfo().Id(L"3900003").Continuous({ 0.75 }));

    SECTION("Values are matched on the ID column")
        {
        REQUIRE(geoData->CopyContinuousColumnFrom(metrics, L"county_id", L"grad_rate",
                                                  L"Graduation Rate"));

        const auto gradColumn = geoData->GetContinuousColumn(L"Graduation Rate");
        REQUIRE(gradColumn != geoData->GetContinuousColumns().cend());
        CHECK_THAT(gradColumn->GetValue(0), WithinAbs(0.91, 1e-9));
        // Beta (row 1) has no match, so it is missing data
        CHECK(std::isnan(gradColumn->GetValue(1)));
        CHECK_THAT(gradColumn->GetValue(2), WithinAbs(0.75, 1e-9));
        }

    SECTION("Unknown columns are reported")
        {
        CHECK_FALSE(geoData->CopyContinuousColumnFrom(metrics, L"county_id", L"nope"));
        CHECK_FALSE(geoData->GetLastError().empty());
        CHECK_FALSE(geoData->CopyContinuousColumnFrom(metrics, L"nope", L"grad_rate"));
        }
    }

TEST_CASE("GeoDataset::CopyCategoricalColumnFrom", "[kml][geodataset]")
    {
    auto geoData = std::make_shared<GeoDataset>();
    REQUIRE(geoData->ImportRegionsFromText(SAMPLE_KML, GeoImportInfo().IdField(L"GEOID")));

    // a category table keyed by the same GEOID identifier
    Dataset ratings;
    ratings.GetIdColumn().SetName(L"county_id");
    ratings.AddCategoricalColumn(L"rating",
                                 ColumnWithStringTable::StringTableType{
                                     { 0, wxString{} }, { 1, L"Excellent" }, { 2, L"Fair" } });
    ratings.AddRow(RowInfo().Id(L"3900001").Categoricals({ 1 }));
    ratings.AddRow(RowInfo().Id(L"3900003").Categoricals({ 2 }));

    SECTION("Labels are matched on the ID column")
        {
        REQUIRE(geoData->CopyCategoricalColumnFrom(ratings, L"county_id", L"rating", L"Rating"));

        const auto ratingColumn = geoData->GetCategoricalColumn(L"Rating");
        REQUIRE(ratingColumn != geoData->GetCategoricalColumns().cend());
        CHECK(ratingColumn->GetLabelFromID(ratingColumn->GetValue(0)) == wxString(L"Excellent"));
        // Beta (row 1) has no match, so it is missing data (empty label)
        CHECK(ratingColumn->GetLabelFromID(ratingColumn->GetValue(1)).empty());
        CHECK(ratingColumn->GetLabelFromID(ratingColumn->GetValue(2)) == wxString(L"Fair"));
        }

    SECTION("Unknown columns are reported")
        {
        CHECK_FALSE(geoData->CopyCategoricalColumnFrom(ratings, L"county_id", L"nope"));
        CHECK_FALSE(geoData->GetLastError().empty());
        CHECK_FALSE(geoData->CopyCategoricalColumnFrom(ratings, L"nope", L"rating"));
        }
    }

TEST_CASE("GeoJsonReader parsing", "[geojson][geodataset]")
    {
    SECTION("Load from text")
        {
        GeoJsonReader reader;
        REQUIRE(reader.LoadText(SAMPLE_GEOJSON));
        CHECK(reader.IsOk());
        CHECK(reader.GetLastError().empty());
        CHECK(reader.GetName() == wxString(L"Test Counties"));
        // the point-only feature is dropped
        REQUIRE(reader.GetRegions().size() == 3);
        }

    SECTION("Region names come from the properties")
        {
        GeoJsonReader reader;
        REQUIRE(reader.LoadText(SAMPLE_GEOJSON));
        CHECK(reader.FindRegion(L"Alpha").has_value());
        CHECK(reader.FindRegion(L"Beta").has_value());
        CHECK(reader.FindRegion(L"Gamma").has_value());
        }

    SECTION("A MultiPolygon yields several polygons")
        {
        GeoJsonReader reader;
        REQUIRE(reader.LoadText(SAMPLE_GEOJSON));
        const auto betaRow = reader.FindRegion(L"Beta");
        REQUIRE(betaRow.has_value());
        CHECK(reader.GetRegions()[*betaRow].m_polygons.size() == 2);
        }

    SECTION("An inner ring becomes a hole")
        {
        GeoJsonReader reader;
        REQUIRE(reader.LoadText(SAMPLE_GEOJSON));
        const auto gammaRow = reader.FindRegion(L"Gamma");
        REQUIRE(gammaRow.has_value());
        const GeoRegion& gamma = reader.GetRegions()[*gammaRow];
        REQUIRE(gamma.m_polygons.size() == 1);
        CHECK(gamma.m_polygons.front().m_innerBoundaries.size() == 1);
        CHECK(gamma.m_polygons.front().m_outerBoundary.size() == 5);
        }

    SECTION("A numeric property is stored without a fractional part")
        {
        GeoJsonReader reader;
        REQUIRE(reader.LoadText(SAMPLE_GEOJSON));
        const auto alphaRow = reader.FindRegion(L"Alpha");
        REQUIRE(alphaRow.has_value());
        const GeoRegion& alpha = reader.GetRegions()[*alphaRow];
        CHECK(alpha.GetAttribute(L"GEOID") == wxString(L"3900001"));
        CHECK(alpha.GetAttribute(L"NAME") == wxString(L"Alpha"));
        }

    SECTION("Bounding box spans every ring")
        {
        GeoJsonReader reader;
        REQUIRE(reader.LoadText(SAMPLE_GEOJSON));
        const auto& box = reader.GetBoundingBox();
        REQUIRE(box.IsOk());
        CHECK_THAT(box.m_minLongitude, WithinAbs(-90.0, 1e-9));
        CHECK_THAT(box.m_maxLongitude, WithinAbs(-80.0, 1e-9));
        CHECK_THAT(box.m_minLatitude, WithinAbs(30.0, 1e-9));
        CHECK_THAT(box.m_maxLatitude, WithinAbs(41.0, 1e-9));
        }

    SECTION("A bare geometry object is accepted")
        {
        GeoJsonReader reader;
        REQUIRE(reader.LoadText(LR"({ "type": "Polygon",
                  "coordinates": [ [ [0,0], [1,0], [1,1], [0,1], [0,0] ] ] })"));
        REQUIRE(reader.GetRegions().size() == 1);
        CHECK(reader.GetRegions().front().m_polygons.size() == 1);
        }

    SECTION("The name field can be overridden")
        {
        GeoJsonReader reader;
        reader.SetNameField(L"GEOID");
        REQUIRE(reader.LoadText(SAMPLE_GEOJSON));
        CHECK(reader.FindRegion(L"3900001").has_value());
        }

    SECTION("Malformed input fails cleanly")
        {
        GeoJsonReader reader;
        CHECK_FALSE(reader.LoadText(L"{ this is not json"));
        CHECK_FALSE(reader.GetLastError().empty());
        CHECK_FALSE(reader.IsOk());
        }

    SECTION("An unrecognized type fails")
        {
        GeoJsonReader reader;
        CHECK_FALSE(reader.LoadText(LR"({ "type": "Nonsense" })"));
        CHECK_FALSE(reader.GetLastError().empty());
        }

    SECTION("A features array with only points produces no regions")
        {
        GeoJsonReader reader;
        CHECK_FALSE(reader.LoadText(LR"({ "type": "FeatureCollection", "features": [
                   { "type": "Feature", "properties": {},
                     "geometry": { "type": "Point", "coordinates": [0,0] } } ] })"));
        CHECK_FALSE(reader.GetLastError().empty());
        }
    }

TEST_CASE("GeoDataset GeoJSON import", "[geojson][geodataset]")
    {
    SECTION("Rows and geometry are built from the features")
        {
        auto geoData = std::make_shared<GeoDataset>();
        REQUIRE(geoData->ImportGeoJSONFromText(SAMPLE_GEOJSON, GeoImportInfo().IdField(L"GEOID")));
        CHECK(geoData->GetRowCount() == 3);
        CHECK(geoData->GetGeometries().size() == 3);
        REQUIRE(geoData->FindRegionRow(L"3900002").has_value());
        CHECK(geoData->FindRegionRow(L"3900002").value() == 1);
        }

    SECTION("A numeric attribute column is imported as continuous")
        {
        auto geoData = std::make_shared<GeoDataset>();
        REQUIRE(geoData->ImportGeoJSONFromText(SAMPLE_GEOJSON, GeoImportInfo().IdField(L"GEOID")));
        const auto alandColumn = geoData->GetContinuousColumn(L"ALAND");
        REQUIRE(alandColumn != geoData->GetContinuousColumns().cend());
        CHECK_THAT(alandColumn->GetValue(0), WithinAbs(1000.0, 1e-9));
        }

    SECTION("Centroid columns are added by default")
        {
        auto geoData = std::make_shared<GeoDataset>();
        REQUIRE(geoData->ImportGeoJSONFromText(SAMPLE_GEOJSON, GeoImportInfo()));
        CHECK(geoData->GetContinuousColumn(L"Centroid Longitude") !=
              geoData->GetContinuousColumns().cend());
        CHECK(geoData->GetContinuousColumn(L"Centroid Latitude") !=
              geoData->GetContinuousColumns().cend());
        }

    SECTION("A merged metric is matched on the GeoJSON ID column")
        {
        auto geoData = std::make_shared<GeoDataset>();
        REQUIRE(geoData->ImportGeoJSONFromText(SAMPLE_GEOJSON, GeoImportInfo().IdField(L"GEOID")));

        Dataset metrics;
        metrics.GetIdColumn().SetName(L"county_id");
        metrics.AddContinuousColumn(L"grad_rate");
        metrics.AddRow(RowInfo().Id(L"3900001").Continuous({ 0.91 }));
        metrics.AddRow(RowInfo().Id(L"3900003").Continuous({ 0.75 }));

        REQUIRE(geoData->CopyContinuousColumnFrom(metrics, L"county_id", L"grad_rate",
                                                  L"Graduation Rate"));
        const auto gradColumn = geoData->GetContinuousColumn(L"Graduation Rate");
        REQUIRE(gradColumn != geoData->GetContinuousColumns().cend());
        CHECK_THAT(gradColumn->GetValue(0), WithinAbs(0.91, 1e-9));
        CHECK(std::isnan(gradColumn->GetValue(1)));
        CHECK_THAT(gradColumn->GetValue(2), WithinAbs(0.75, 1e-9));
        }
    }

TEST_CASE("GeoDataset region-file dispatch", "[geojson][kml][geodataset]")
    {
    CHECK(GeoDataset::IsGeoJsonFile(L"counties.geojson"));
    CHECK(GeoDataset::IsGeoJsonFile(L"counties.json"));
    CHECK(GeoDataset::IsGeoJsonFile(L"path/to/Counties.GeoJSON"));
    CHECK_FALSE(GeoDataset::IsGeoJsonFile(L"counties.kml"));
    CHECK_FALSE(GeoDataset::IsGeoJsonFile(L"counties.KML"));
    CHECK_FALSE(GeoDataset::IsGeoJsonFile(L"counties"));
    }

TEST_CASE("GeoDataset geometry-only import for a background layer", "[geojson][kml][geodataset]")
    {
    SECTION("From GeoJSON")
        {
        auto backgroundData = std::make_shared<GeoDataset>();
        REQUIRE(backgroundData->ImportGeoJSONFromText(SAMPLE_GEOJSON, GeoImportInfo()));
        REQUIRE_FALSE(backgroundData->GetGeometries().empty());
        for (const auto& region : backgroundData->GetGeometries())
            {
            CHECK(region.m_boundingBox.IsOk());
            REQUIRE_FALSE(region.m_polygons.empty());
            CHECK(region.m_polygons.front().m_outerBoundary.size() >= 3);
            }
        }

    SECTION("From KML")
        {
        auto backgroundData = std::make_shared<GeoDataset>();
        REQUIRE(backgroundData->ImportRegionsFromText(SAMPLE_KML, GeoImportInfo()));
        REQUIRE_FALSE(backgroundData->GetGeometries().empty());
        for (const auto& region : backgroundData->GetGeometries())
            {
            CHECK(region.m_boundingBox.IsOk());
            REQUIRE_FALSE(region.m_polygons.empty());
            CHECK(region.m_polygons.front().m_outerBoundary.size() >= 3);
            }
        }
    }
