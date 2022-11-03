#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include "../src/data/dataset.h"

using namespace Wisteria;
using namespace Wisteria::Data;

TEST_CASE("Data range", "[data]")
	{
    Data::Dataset col;

    col.AddRow(Data::RowInfo().Continuous({ 5, 6 }).Id(L"label1"));
    col.AddRow(Data::RowInfo().Continuous({ 5, 6 }).Id(L"label2"));
    CHECK(col.GetIdColumn().GetValue(0) == L"label1");
    CHECK(col.GetIdColumn().GetValue(1) == L"label2");
	}
