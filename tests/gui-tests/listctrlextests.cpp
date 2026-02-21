#include "../../src/ui/controls/listctrlex.h"
#include "../../src/ui/controls/listctrlexcelexporter.h"
#include "../../src/ui/controls/listctrlexdataprovider.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <wx/sstream.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

using namespace Catch::Matchers;
using namespace Wisteria::UI;
using namespace Wisteria;

TEST_CASE("ListCtrlExNumericDataProvider", "[listctrlexnumericdataprovider]")
    {
    ListCtrlExNumericDataProvider dataProvider;

    SECTION("Set items")
        {
        class SimpleFormat : public Wisteria::NumberFormat<wxString>
            {
          public:
            wxString GetFormattedValue(const wxString& value,
                                       const Wisteria::NumberFormatInfo&) const
                {
                return value;
                }

            wxString GetFormattedValue(const double value,
                                       const Wisteria::NumberFormatInfo& format) const
                {
                return wxNumberFormatter::ToString(value, format.m_precision, 1);
                }
            };

        SimpleFormat numForm;
        dataProvider.SetNumberFormatter(&numForm);
        dataProvider.SetSize(5, 2);
        dataProvider.SetItemValue(0, 0, 1);
        dataProvider.SetItemText(
            0, 1, L"first",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(1, 0, 2);
        dataProvider.SetItemText(
            1, 1, L"second",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(2, 0, 3);
        dataProvider.SetItemText(
            2, 1, L"third",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(
            3, 0, 76,
            Wisteria::NumberFormatInfo(
                Wisteria::NumberFormatInfo::NumberFormatType::PercentageFormatting, 1, true));
        dataProvider.SetItemValue(
            4, 0, 76.25,
            Wisteria::NumberFormatInfo(
                Wisteria::NumberFormatInfo::NumberFormatType::CustomFormatting, 2));
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"first");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"second");
        CHECK(dataProvider.GetItemText(2, 0) == L"3");
        CHECK(dataProvider.GetItemText(2, 1) == L"third");
        CHECK(dataProvider.GetItemText(3, 0) == L"76%");
        CHECK(dataProvider.GetItemText(4, 0) == L"76.25");
        CHECK(dataProvider.GetItemCount() == 5);
        CHECK(dataProvider.GetColumnCount() == 2);
        }
    SECTION("Delete items")
        {
        dataProvider.SetSize(3, 2);
        dataProvider.SetItemValue(0, 0, 1);
        dataProvider.SetItemText(
            0, 1, L"first",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(1, 0, 2);
        dataProvider.SetItemText(
            1, 1, L"second",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(2, 0, 3);
        dataProvider.SetItemText(
            2, 1, L"third",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.DeleteItem(1);
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"first");
        CHECK(dataProvider.GetItemText(1, 0) == L"3");
        CHECK(dataProvider.GetItemText(1, 1) == L"third");
        CHECK(dataProvider.GetItemCount() == 2);
        CHECK(dataProvider.GetColumnCount() == 2);
        dataProvider.DeleteAllItems();
        CHECK(dataProvider.GetItemCount() == 0);
        }
    SECTION("Sort items")
        {
        dataProvider.SetSize(3, 2);
        dataProvider.SetItemValue(0, 0, 1);
        dataProvider.SetItemText(
            0, 1, L"a", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(1, 0, 2);
        dataProvider.SetItemText(
            1, 1, L"c", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(2, 0, 3);
        dataProvider.SetItemText(
            2, 1, L"b", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.Sort(1, Wisteria::SortDirection::SortDescending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"2");
        CHECK(dataProvider.GetItemText(0, 1) == L"c");
        CHECK(dataProvider.GetItemText(1, 0) == L"3");
        CHECK(dataProvider.GetItemText(1, 1) == L"b");
        CHECK(dataProvider.GetItemText(2, 0) == L"1");
        CHECK(dataProvider.GetItemText(2, 1) == L"a");
        dataProvider.Sort(1, Wisteria::SortDirection::SortAscending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(2, 0) == L"2");
        CHECK(dataProvider.GetItemText(2, 1) == L"c");
        CHECK(dataProvider.GetItemText(1, 0) == L"3");
        CHECK(dataProvider.GetItemText(1, 1) == L"b");
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"a");
        dataProvider.Sort(0, Wisteria::SortDirection::SortDescending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(2, 0) == L"1");
        CHECK(dataProvider.GetItemText(2, 1) == L"a");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"c");
        CHECK(dataProvider.GetItemText(0, 0) == L"3");
        CHECK(dataProvider.GetItemText(0, 1) == L"b");
        dataProvider.Sort(0, Wisteria::SortDirection::SortAscending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"a");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"c");
        CHECK(dataProvider.GetItemText(2, 0) == L"3");
        CHECK(dataProvider.GetItemText(2, 1) == L"b");
        // bogus column, should silently fail
        dataProvider.Sort(2, Wisteria::SortDirection::SortAscending, 0, (size_t)-1);
        }
    SECTION("Sort items mixed data")
        {
        dataProvider.SetSize(10, 1);
        dataProvider.SetItemValue(0, 0, 1);
        dataProvider.SetItemText(
            1, 0, L"a", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(2, 0, 2);
        dataProvider.SetItemText(
            3, 0, L"c", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(4, 0, 11);
        dataProvider.SetItemText(
            5, 0, L"", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(6, 0, 2);
        dataProvider.SetItemText(
            7, 0, L"a", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            8, 0, L"B", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            9, 0, L"Z",
            Wisteria::NumberFormatInfo{
                Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting },
            4); // should actually sort as 4
        dataProvider.Sort(0, Wisteria::SortDirection::SortDescending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"c");
        CHECK(dataProvider.GetItemText(1, 0) == L"B");
        CHECK(dataProvider.GetItemText(2, 0) == L"a");
        CHECK(dataProvider.GetItemText(3, 0) == L"a");
        CHECK(dataProvider.GetItemText(4, 0) == L"11");
        CHECK(dataProvider.GetItemText(5, 0) == L"Z");
        CHECK(dataProvider.GetItemText(6, 0) == L"2");
        CHECK(dataProvider.GetItemText(7, 0) == L"2");
        CHECK(dataProvider.GetItemText(8, 0) == L"1");
        CHECK(dataProvider.GetItemText(9, 0) == L"");
        dataProvider.Sort(0, Wisteria::SortDirection::SortAscending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"");
        CHECK(dataProvider.GetItemText(1, 0) == L"1");
        CHECK(dataProvider.GetItemText(2, 0) == L"2");
        CHECK(dataProvider.GetItemText(3, 0) == L"2");
        CHECK(dataProvider.GetItemText(4, 0) == L"Z");
        CHECK(dataProvider.GetItemText(5, 0) == L"11");
        CHECK(dataProvider.GetItemText(6, 0) == L"a");
        CHECK(dataProvider.GetItemText(7, 0) == L"a");
        CHECK(dataProvider.GetItemText(8, 0) == L"B");
        CHECK(dataProvider.GetItemText(9, 0) == L"c");
        }
    SECTION("Sort items multicolumn first")
        {
        dataProvider.SetSize(4, 2);
        dataProvider.SetItemValue(0, 0, 1);
        dataProvider.SetItemText(
            0, 1, L"first",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(1, 0, 2);
        dataProvider.SetItemText(
            1, 1, L"second",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(2, 0, 1);
        dataProvider.SetItemText(
            2, 1, L"third",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(3, 0, 2);
        dataProvider.SetItemText(
            3, 1, L"fourth",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"2");
        CHECK(dataProvider.GetItemText(0, 1) == L"second");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"fourth");
        CHECK(dataProvider.GetItemText(2, 0) == L"1");
        CHECK(dataProvider.GetItemText(2, 1) == L"third");
        CHECK(dataProvider.GetItemText(3, 0) == L"1");
        CHECK(dataProvider.GetItemText(3, 1) == L"first");

        columns.clear();
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"first");
        CHECK(dataProvider.GetItemText(1, 0) == L"1");
        CHECK(dataProvider.GetItemText(1, 1) == L"third");
        CHECK(dataProvider.GetItemText(2, 0) == L"2");
        CHECK(dataProvider.GetItemText(2, 1) == L"fourth");
        CHECK(dataProvider.GetItemText(3, 0) == L"2");
        CHECK(dataProvider.GetItemText(3, 1) == L"second");

        // bogus column, should silently fail
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(2, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        }
    SECTION("Sort items multicolumn second")
        {
        dataProvider.SetSize(5, 2);
        dataProvider.SetItemValue(0, 0, 1);
        dataProvider.SetItemText(
            0, 1, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(1, 0, 2);
        dataProvider.SetItemText(
            1, 1, L"text2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(2, 0, 3);
        dataProvider.SetItemText(
            2, 1, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(3, 0, 4);
        dataProvider.SetItemText(
            3, 1, L"text2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemValue(4, 0, 5);
        dataProvider.SetItemText(
            4, 1, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"text");
        CHECK(dataProvider.GetItemText(1, 0) == L"3");
        CHECK(dataProvider.GetItemText(1, 1) == L"text");
        CHECK(dataProvider.GetItemText(2, 0) == L"5");
        CHECK(dataProvider.GetItemText(2, 1) == L"text");
        CHECK(dataProvider.GetItemText(3, 0) == L"2");
        CHECK(dataProvider.GetItemText(3, 1) == L"text2");
        CHECK(dataProvider.GetItemText(4, 0) == L"4");
        CHECK(dataProvider.GetItemText(4, 1) == L"text2");

        columns.clear();
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"4");
        CHECK(dataProvider.GetItemText(0, 1) == L"text2");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"text2");
        CHECK(dataProvider.GetItemText(2, 0) == L"5");
        CHECK(dataProvider.GetItemText(2, 1) == L"text");
        CHECK(dataProvider.GetItemText(3, 0) == L"3");
        CHECK(dataProvider.GetItemText(3, 1) == L"text");
        CHECK(dataProvider.GetItemText(4, 0) == L"1");
        CHECK(dataProvider.GetItemText(4, 1) == L"text");

        // bogus column, should silently fail
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(2, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        }
    SECTION("Sort items multicolumn nothing to sort second column descending numeric")
        {
        dataProvider.SetSize(4, 2);
        dataProvider.SetItemValue(0, 0, 2);
        dataProvider.SetItemValue(0, 1, 2);
        dataProvider.SetItemValue(1, 0, 2);
        dataProvider.SetItemValue(1, 1, 9);
        dataProvider.SetItemValue(2, 0, 2);
        dataProvider.SetItemValue(2, 1, 2);
        dataProvider.SetItemValue(3, 0, 2);
        dataProvider.SetItemValue(3, 1, 2);
        std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"2");
        CHECK(dataProvider.GetItemText(0, 1) == L"9");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"2");
        CHECK(dataProvider.GetItemText(2, 0) == L"2");
        CHECK(dataProvider.GetItemText(2, 1) == L"2");
        CHECK(dataProvider.GetItemText(3, 0) == L"2");
        CHECK(dataProvider.GetItemText(3, 1) == L"2");
        }
    SECTION("Sort items multicolumn nothing to sort second column ascending numeric")
        {
        dataProvider.SetSize(4, 2);
        dataProvider.SetItemValue(0, 0, 2);
        dataProvider.SetItemValue(0, 1, 2);
        dataProvider.SetItemValue(1, 0, 2);
        dataProvider.SetItemValue(1, 1, 9);
        dataProvider.SetItemValue(2, 0, 2);
        dataProvider.SetItemValue(2, 1, 2);
        dataProvider.SetItemValue(3, 0, 2);
        dataProvider.SetItemValue(3, 1, 2);
        std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"2");
        CHECK(dataProvider.GetItemText(0, 1) == L"2");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"2");
        CHECK(dataProvider.GetItemText(2, 0) == L"2");
        CHECK(dataProvider.GetItemText(2, 1) == L"2");
        CHECK(dataProvider.GetItemText(3, 0) == L"2");
        CHECK(dataProvider.GetItemText(3, 1) == L"9");
        }
    SECTION("Empty data")
        {
        dataProvider.SetSize(10, 1);
        for (size_t i = 0; i < 10; ++i)
            {
            CHECK(dataProvider.GetItemText(i, 0) == L"");
            }
        }
    SECTION("Find item")
        {
        dataProvider.SetSize(3, 2);
        dataProvider.SetItemValue(0, 0, 1);
        dataProvider.SetItemText(
            0, 1, L"first2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 0, L"second",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 1, L"second2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 0, L"third",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 1, L"third2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        CHECK(dataProvider.Find(L"second", 0) == 1);
        }
    }

TEST_CASE("ListCtrlExDataProvider", "[listctrlexdataprovider]")
    {
    ListCtrlExDataProvider dataProvider;

    SECTION("Set items")
        {
        dataProvider.SetSize(3, 2);
        dataProvider.SetItemText(
            0, 0, L"first",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            0, 1, L"first2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 0, L"second",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 1, L"second2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 0, L"third",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 1, L"third2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        CHECK(dataProvider.GetItemText(0, 0) == L"first");
        CHECK(dataProvider.GetItemText(0, 1) == L"first2");
        CHECK(dataProvider.GetItemText(1, 0) == L"second");
        CHECK(dataProvider.GetItemText(1, 1) == L"second2");
        CHECK(dataProvider.GetItemText(2, 0) == L"third");
        CHECK(dataProvider.GetItemText(2, 1) == L"third2");
        CHECK(dataProvider.GetItemCount() == 3);
        CHECK(dataProvider.GetColumnCount() == 2);
        }
    SECTION("Delete items")
        {
        dataProvider.SetSize(3, 2);
        dataProvider.SetItemText(
            0, 0, L"first",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            0, 1, L"first2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 0, L"second",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 1, L"second2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 0, L"third",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 1, L"third2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.DeleteItem(1);
        CHECK(dataProvider.GetItemText(0, 0) == L"first");
        CHECK(dataProvider.GetItemText(0, 1) == L"first2");
        CHECK(dataProvider.GetItemText(1, 0) == L"third");
        CHECK(dataProvider.GetItemText(1, 1) == L"third2");
        CHECK(dataProvider.GetItemCount() == 2);
        CHECK(dataProvider.GetColumnCount() == 2);
        dataProvider.DeleteAllItems();
        CHECK(dataProvider.GetItemCount() == 0);
        }
    SECTION("Sort items")
        {
        dataProvider.SetSize(3, 2);
        dataProvider.SetItemText(
            0, 0, L"1", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            0, 1, L"first2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 1, L"second2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 0, L"11", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 1, L"third2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.Sort(0, Wisteria::SortDirection::SortDescending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"11");
        CHECK(dataProvider.GetItemText(0, 1) == L"third2");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"second2");
        CHECK(dataProvider.GetItemText(2, 0) == L"1");
        CHECK(dataProvider.GetItemText(2, 1) == L"first2");
        dataProvider.Sort(0, Wisteria::SortDirection::SortAscending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"first2");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"second2");
        CHECK(dataProvider.GetItemText(2, 0) == L"11");
        CHECK(dataProvider.GetItemText(2, 1) == L"third2");
        dataProvider.Sort(1, Wisteria::SortDirection::SortDescending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(2, 0) == L"1");
        CHECK(dataProvider.GetItemText(2, 1) == L"first2");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"second2");
        CHECK(dataProvider.GetItemText(0, 0) == L"11");
        CHECK(dataProvider.GetItemText(0, 1) == L"third2");
        dataProvider.Sort(1, Wisteria::SortDirection::SortAscending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"first2");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"second2");
        CHECK(dataProvider.GetItemText(2, 0) == L"11");
        CHECK(dataProvider.GetItemText(2, 1) == L"third2");
        // bogus column, should silently fail
        dataProvider.Sort(2, Wisteria::SortDirection::SortAscending, 0, (size_t)-1);
        }
    SECTION("Sort items multicolumn nothing to sort second column descending")
        {
        dataProvider.SetSize(4, 2);
        dataProvider.SetItemText(
            0, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            0, 1, L"two",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 1, L"zzz",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 1, L"two",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            3, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            3, 1, L"two",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"2");
        CHECK(dataProvider.GetItemText(0, 1) == L"zzz");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"two");
        CHECK(dataProvider.GetItemText(2, 0) == L"2");
        CHECK(dataProvider.GetItemText(2, 1) == L"two");
        CHECK(dataProvider.GetItemText(3, 0) == L"2");
        CHECK(dataProvider.GetItemText(3, 1) == L"two");
        dataProvider.Sort(0, Wisteria::SortDirection::SortDescending, 0, (size_t)-1);
        dataProvider.Sort(1, Wisteria::SortDirection::SortDescending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"2");
        CHECK(dataProvider.GetItemText(0, 1) == L"zzz");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"two");
        CHECK(dataProvider.GetItemText(2, 0) == L"2");
        CHECK(dataProvider.GetItemText(2, 1) == L"two");
        CHECK(dataProvider.GetItemText(3, 0) == L"2");
        CHECK(dataProvider.GetItemText(3, 1) == L"two");
        }
    SECTION("Sort items multicolumn nothing to sort second column ascending")
        {
        dataProvider.SetSize(4, 2);
        dataProvider.SetItemText(
            0, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            0, 1, L"two",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 1, L"zzz",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 1, L"two",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            3, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            3, 1, L"two",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"2");
        CHECK(dataProvider.GetItemText(0, 1) == L"two");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"two");
        CHECK(dataProvider.GetItemText(2, 0) == L"2");
        CHECK(dataProvider.GetItemText(2, 1) == L"two");
        CHECK(dataProvider.GetItemText(3, 0) == L"2");
        CHECK(dataProvider.GetItemText(3, 1) == L"zzz");
        dataProvider.Sort(0, Wisteria::SortDirection::SortAscending, 0, (size_t)-1);
        dataProvider.Sort(1, Wisteria::SortDirection::SortAscending, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"2");
        CHECK(dataProvider.GetItemText(0, 1) == L"two");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"two");
        CHECK(dataProvider.GetItemText(2, 0) == L"2");
        CHECK(dataProvider.GetItemText(2, 1) == L"two");
        CHECK(dataProvider.GetItemText(3, 0) == L"2");
        CHECK(dataProvider.GetItemText(3, 1) == L"zzz");
        }
    SECTION("Sort items multicolumn first")
        {
        dataProvider.SetSize(4, 2);
        dataProvider.SetItemText(
            0, 0, L"1", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            0, 1, L"first",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 1, L"second",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 0, L"1", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 1, L"third",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            3, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            3, 1, L"fourth",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"2");
        CHECK(dataProvider.GetItemText(0, 1) == L"second");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"fourth");
        CHECK(dataProvider.GetItemText(2, 0) == L"1");
        CHECK(dataProvider.GetItemText(2, 1) == L"third");
        CHECK(dataProvider.GetItemText(3, 0) == L"1");
        CHECK(dataProvider.GetItemText(3, 1) == L"first");

        columns.clear();
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"first");
        CHECK(dataProvider.GetItemText(1, 0) == L"1");
        CHECK(dataProvider.GetItemText(1, 1) == L"third");
        CHECK(dataProvider.GetItemText(2, 0) == L"2");
        CHECK(dataProvider.GetItemText(2, 1) == L"fourth");
        CHECK(dataProvider.GetItemText(3, 0) == L"2");
        CHECK(dataProvider.GetItemText(3, 1) == L"second");

        // bogus column, should silently fail
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(2, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        }
    SECTION("Sort items multicolumn second")
        {
        dataProvider.SetSize(5, 2);
        dataProvider.SetItemText(
            0, 0, L"1", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            0, 1, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 0, L"2", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 1, L"text2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 0, L"3", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 1, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            3, 0, L"4", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            3, 1, L"text2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            4, 0, L"5", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            4, 1, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"text");
        CHECK(dataProvider.GetItemText(1, 0) == L"3");
        CHECK(dataProvider.GetItemText(1, 1) == L"text");
        CHECK(dataProvider.GetItemText(2, 0) == L"5");
        CHECK(dataProvider.GetItemText(2, 1) == L"text");
        CHECK(dataProvider.GetItemText(3, 0) == L"2");
        CHECK(dataProvider.GetItemText(3, 1) == L"text2");
        CHECK(dataProvider.GetItemText(4, 0) == L"4");
        CHECK(dataProvider.GetItemText(4, 1) == L"text2");

        columns.clear();
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"4");
        CHECK(dataProvider.GetItemText(0, 1) == L"text2");
        CHECK(dataProvider.GetItemText(1, 0) == L"2");
        CHECK(dataProvider.GetItemText(1, 1) == L"text2");
        CHECK(dataProvider.GetItemText(2, 0) == L"5");
        CHECK(dataProvider.GetItemText(2, 1) == L"text");
        CHECK(dataProvider.GetItemText(3, 0) == L"3");
        CHECK(dataProvider.GetItemText(3, 1) == L"text");
        CHECK(dataProvider.GetItemText(4, 0) == L"1");
        CHECK(dataProvider.GetItemText(4, 1) == L"text");

        // bogus column, should silently fail
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(2, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        }
    SECTION("Sort items multicolumn mixed")
        {
        dataProvider.SetSize(3, 2);
        dataProvider.SetItemText(
            0, 0, L"1", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            0, 1, L"text333",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 0, L"1", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 1, L"text33",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 0, L"1", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 1, L"text3",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"text3");
        CHECK(dataProvider.GetItemText(1, 0) == L"1");
        CHECK(dataProvider.GetItemText(1, 1) == L"text33");
        CHECK(dataProvider.GetItemText(2, 0) == L"1");
        CHECK(dataProvider.GetItemText(2, 1) == L"text333");
        }
    SECTION("Sort items multicolumn mixed numeric")
        {
        dataProvider.SetSize(3, 2);
        dataProvider.SetItemText(
            0, 0, L"1", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            0, 1, L"text333",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 0, L"1", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 1, L"text33",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 0, L"1", NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 1, L"text3",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns, 0, (size_t)-1);
        CHECK(dataProvider.GetItemText(0, 0) == L"1");
        CHECK(dataProvider.GetItemText(0, 1) == L"text3");
        CHECK(dataProvider.GetItemText(1, 0) == L"1");
        CHECK(dataProvider.GetItemText(1, 1) == L"text33");
        CHECK(dataProvider.GetItemText(2, 0) == L"1");
        CHECK(dataProvider.GetItemText(2, 1) == L"text333");
        }
    SECTION("Find item")
        {
        dataProvider.SetSize(3, 2);
        dataProvider.SetItemText(
            0, 0, L"first",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            0, 1, L"first2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 0, L"second",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            1, 1, L"second2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 0, L"third",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        dataProvider.SetItemText(
            2, 1, L"third2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        CHECK(dataProvider.Find(L"second", 0) == 1);
        }
    }

TEST_CASE("ListCtrlEx to LaTeX", "[listctrlex]")
    {
    auto m_dataProvider = std::make_shared<ListCtrlExNumericDataProvider>();
    auto m_list = new ListCtrlEx(wxTheApp->GetTopWindow(), wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, wxLC_VIRTUAL | wxLC_REPORT | wxBORDER_SUNKEN);
    m_list->Hide();

    const auto Reset2Columns = [&m_dataProvider, &m_list]()
    {
        m_dataProvider->SetSize(7, 2);
        m_dataProvider->SetItemText(
            0, 0, L"Text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 0, L"tExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 0, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 0, L"teXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 0, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 0, 72);
        m_dataProvider->SetItemValue(6, 0, 7);
        // other column
        m_dataProvider->SetItemText(
            0, 1, L"Sext",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 1, L"sExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 1, L"sext",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 1, L"seXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 1, L"sext",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 1, 82);
        m_dataProvider->SetItemValue(6, 1, 8);
        m_list->SetVirtualDataProvider(m_dataProvider);
        m_list->SetVirtualDataSize(7, 2);
        m_list->InsertColumn(0, L"NAME");
        m_list->InsertColumn(1, L"OTHER");
    };

    Reset2Columns();

    SECTION("Format to LaTEx all rows")
        {
        m_list->Select(0);
        m_list->Select(3);
        m_list->Select(6);
        wxString outputText = m_list->FormatToLaTeX(ListCtrlEx::ExportRowSelection::ExportAll, 0,
                                                    -1, 0, -1, true, L"My Table Caption");
        CHECK(outputText == LR"(\begin{longtable}{|l|l|}
\caption{My Table Caption} \label{tab:long} \\
\hline \multicolumn{1}{|c|}{\textbf{NAME}} & \multicolumn{1}{|c|}{\textbf{OTHER}} \\ \hline
\endfirsthead

\multicolumn{2}{c}%
{{\bfseries \tablename\ \thetable{} -- continued from previous page}} \\
\hline \multicolumn{1}{|c|}{\textbf{NAME}} & \multicolumn{1}{|c|}{\textbf{OTHER}} \\ \hline
\endhead

\hline \multicolumn{2}{|r|}{{Continued on next page}} \\ \hline
\endfoot

\hline
\endlastfoot

Text & Sext \\
tExt2 & sExt2 \\
text & sext \\
teXt2 & seXt2 \\
text & sext \\
72 & 82 \\
7 & 8 \\

\end{longtable})");
        }

    SECTION("Format to LaTEx only selected rows")
        {
        m_list->Select(0);
        m_list->Select(3);
        m_list->Select(6);
        wxString outputText = m_list->FormatToLaTeX(ListCtrlEx::ExportRowSelection::ExportSelected,
                                                    0, -1, 0, -1, true, L"My Table Caption");
        CHECK(outputText == LR"(\begin{longtable}{|l|l|}
\caption{My Table Caption} \label{tab:long} \\
\hline \multicolumn{1}{|c|}{\textbf{NAME}} & \multicolumn{1}{|c|}{\textbf{OTHER}} \\ \hline
\endfirsthead

\multicolumn{2}{c}%
{{\bfseries \tablename\ \thetable{} -- continued from previous page}} \\
\hline \multicolumn{1}{|c|}{\textbf{NAME}} & \multicolumn{1}{|c|}{\textbf{OTHER}} \\ \hline
\endhead

\hline \multicolumn{2}{|r|}{{Continued on next page}} \\ \hline
\endfoot

\hline
\endlastfoot

Text & Sext \\
teXt2 & seXt2 \\
7 & 8 \\

\end{longtable})");
        }

    SECTION("Format to LaTeX custom row range")
        {
        wxString outputText =
            m_list->FormatToLaTeX(ListCtrlEx::ExportRowSelection::ExportRange, 3, 5, 0, 0);
        CHECK(outputText == LR"(\begin{longtable}{|l|l|}
\hline \multicolumn{1}{|c|}{\textbf{NAME}} \\ \hline
\endfirsthead

\multicolumn{2}{c}%
{{\bfseries \tablename\ \thetable{} -- continued from previous page}} \\
\hline \multicolumn{1}{|c|}{\textbf{NAME}} \\ \hline
\endhead

\hline \multicolumn{2}{|r|}{{Continued on next page}} \\ \hline
\endfoot

\hline
\endlastfoot

teXt2 \\
text \\
72 \\

\end{longtable})");
        }

    SECTION("Format to LaTeX custom column range")
        {
        m_dataProvider->SetSize(7, 2);
        m_dataProvider->SetItemText(
            0, 1, L"2Text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 1, L"2tExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 1, L"2text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 1, L"2teXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 1, L"2text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 1, 272);
        m_dataProvider->SetItemValue(6, 1, 27);
        m_list->SetVirtualDataSize(7, 2);
        m_list->DeleteAllColumns();
        m_list->InsertColumn(0, L"NAME");
        m_list->InsertColumn(1, wxString("NAME2"));
        wxString outputText =
            m_list->FormatToLaTeX(ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 0, -1);
        CHECK(outputText == LR"(\begin{longtable}{|l|l|}
\hline \multicolumn{1}{|c|}{\textbf{NAME}} & \multicolumn{1}{|c|}{\textbf{NAME2}} \\ \hline
\endfirsthead

\multicolumn{2}{c}%
{{\bfseries \tablename\ \thetable{} -- continued from previous page}} \\
\hline \multicolumn{1}{|c|}{\textbf{NAME}} & \multicolumn{1}{|c|}{\textbf{NAME2}} \\ \hline
\endhead

\hline \multicolumn{2}{|r|}{{Continued on next page}} \\ \hline
\endfoot

\hline
\endlastfoot

Text & 2Text \\
tExt2 & 2tExt2 \\
text & 2text \\
teXt2 & 2teXt2 \\
text & 2text \\
72 & 272 \\
7 & 27 \\

\end{longtable})");
        // just get the first column
        outputText =
            m_list->FormatToLaTeX(ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 0, 0, true);
        CHECK(outputText == LR"(\begin{longtable}{|l|l|}
\hline \multicolumn{1}{|c|}{\textbf{NAME}} \\ \hline
\endfirsthead

\multicolumn{2}{c}%
{{\bfseries \tablename\ \thetable{} -- continued from previous page}} \\
\hline \multicolumn{1}{|c|}{\textbf{NAME}} \\ \hline
\endhead

\hline \multicolumn{2}{|r|}{{Continued on next page}} \\ \hline
\endfoot

\hline
\endlastfoot

Text \\
tExt2 \\
text \\
teXt2 \\
text \\
72 \\
7 \\

\end{longtable})");
        // get last column
        outputText =
            m_list->FormatToLaTeX(ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 1, 1, true);
        CHECK(outputText == LR"(\begin{longtable}{|l|l|}
\hline \multicolumn{1}{|c|}{\textbf{NAME2}} \\ \hline
\endfirsthead

\multicolumn{2}{c}%
{{\bfseries \tablename\ \thetable{} -- continued from previous page}} \\
\hline \multicolumn{1}{|c|}{\textbf{NAME2}} \\ \hline
\endhead

\hline \multicolumn{2}{|r|}{{Continued on next page}} \\ \hline
\endfoot

\hline
\endlastfoot

2Text \\
2tExt2 \\
2text \\
2teXt2 \\
2text \\
272 \\
27 \\

\end{longtable})");
        }
    }

// platforms will have different system font defaults, so remove that from the exported HTML
[[nodiscard]]
wxString RemoveFontStyleSection(const wxString& value)
    {
    wxString str = value;
    const std::wstring_view styleSection{ L"style='font-family:" };
    auto startPos = str.find(styleSection.data());
    if (startPos != wxString::npos)
        {
        startPos += 7;
        const auto endPos = str.find("'", startPos + styleSection.length());
        if (endPos != wxString::npos)
            {
            str.erase(startPos, endPos - startPos);
            }
        }
    return str;
    }

TEST_CASE("ListCtrlEx Format", "[listctrlex]")
    {
    auto m_dataProvider = std::make_shared<ListCtrlExNumericDataProvider>();
    auto m_list = new ListCtrlEx(wxTheApp->GetTopWindow(), wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, wxLC_VIRTUAL | wxLC_REPORT | wxBORDER_SUNKEN);
    m_list->Hide();

    const auto Reset = [&m_dataProvider, &m_list]()
    {
        m_dataProvider->SetSize(7, 1);
        m_dataProvider->SetItemText(
            0, 0, L"Text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 0, L"tExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 0, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 0, L"teXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 0, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 0, 72);
        m_dataProvider->SetItemValue(6, 0, 7);
        m_list->SetVirtualDataProvider(m_dataProvider);
        m_list->SetVirtualDataSize(7, 1);
        m_list->InsertColumn(0, L"NAME");
    };

    Reset();
    SECTION("Format to html only selected rows")
        {
        wxString outputText;
        m_list->Select(0);
        m_list->Select(3);
        m_list->Select(6);
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportSelected);
        CHECK(RemoveFontStyleSection(outputText) ==
              wxString("<table border='1' style=''>\n"
                       "    <thead><tr style='background:#337BC4; "
                       "color:white;'><td>NAME</td></tr></thead>\n"
                       "    <tr><td>Text</td></tr>\n"
                       "    <tr><td>teXt2</td></tr>\n"
                       "    <tr><td>7</td></tr>\n"
                       "</table>"));
        }
    SECTION("Format to html no header")
        {
        wxString outputText;
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportAll, 0, -1, 0,
                             -1, false);
        CHECK(RemoveFontStyleSection(outputText) == wxString("<table border='1' style=''>\n"
                                                             "    <tr><td>Text</td></tr>\n"
                                                             "    <tr><td>tExt2</td></tr>\n"
                                                             "    <tr><td>text</td></tr>\n"
                                                             "    <tr><td>teXt2</td></tr>\n"
                                                             "    <tr><td>text</td></tr>\n"
                                                             "    <tr><td>72</td></tr>\n"
                                                             "    <tr><td>7</td></tr>\n"
                                                             "</table>"));
        }
    SECTION("Format to html custom row range")
        {
        wxString outputText;
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 3, 5,
                             0, -1, true);
        CHECK(RemoveFontStyleSection(outputText) ==
              wxString("<table border='1' style=''>\n"
                       "    <thead><tr style='background:#337BC4; "
                       "color:white;'><td>NAME</td></tr></thead>\n"
                       "    <tr><td>teXt2</td></tr>\n"
                       "    <tr><td>text</td></tr>\n"
                       "    <tr><td>72</td></tr>\n"
                       "</table>"));
        }
    SECTION("Format to html custom row range bad")
        {
        wxString outputText;
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 99, 5,
                             0, -1, true);
        CHECK(outputText.empty());
        // starting point after ending point is nonsense
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 5, 4,
                             0, -1, true);
        CHECK(outputText.empty());
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, 99,
                             0, -1, true);
        CHECK(RemoveFontStyleSection(outputText) ==
              wxString("<table border='1' style=''>\n"
                       "    <thead><tr style='background:#337BC4; "
                       "color:white;'><td>NAME</td></tr></thead>\n"
                       "    <tr><td>Text</td></tr>\n"
                       "    <tr><td>tExt2</td></tr>\n"
                       "    <tr><td>text</td></tr>\n"
                       "    <tr><td>teXt2</td></tr>\n"
                       "    <tr><td>text</td></tr>\n"
                       "    <tr><td>72</td></tr>\n"
                       "    <tr><td>7</td></tr>\n"
                       "</table>"));
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, -10,
                             -1, 0, -1, true);
        CHECK(RemoveFontStyleSection(outputText) ==
              wxString("<table border='1' style=''>\n"
                       "    <thead><tr style='background:#337BC4; "
                       "color:white;'><td>NAME</td></tr></thead>\n"
                       "    <tr><td>Text</td></tr>\n"
                       "    <tr><td>tExt2</td></tr>\n"
                       "    <tr><td>text</td></tr>\n"
                       "    <tr><td>teXt2</td></tr>\n"
                       "    <tr><td>text</td></tr>\n"
                       "    <tr><td>72</td></tr>\n"
                       "    <tr><td>7</td></tr>\n"
                       "</table>"));
        }
    SECTION("Format to html custom column range")
        {
        m_dataProvider->SetSize(7, 2);
        m_dataProvider->SetItemText(
            0, 1, L"2Text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 1, L"2tExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 1, L"2text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 1, L"2teXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 1, L"2text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 1, 272);
        m_dataProvider->SetItemValue(6, 1, 27);
        m_list->SetVirtualDataSize(7, 2);
        m_list->InsertColumn(1, L"NAME2");
        wxString outputText;
        // get both columns
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1,
                             0, -1, true);
        CHECK(wxString("<table border='1' style=''>\n"
                       "    <thead><tr style='background:#337BC4; "
                       "color:white;'><td>NAME</td><td>NAME2</td></tr></thead>\n"
                       "    <tr><td>Text</td><td>2Text</td></tr>\n"
                       "    <tr><td>tExt2</td><td>2tExt2</td></tr>\n"
                       "    <tr><td>text</td><td>2text</td></tr>\n"
                       "    <tr><td>teXt2</td><td>2teXt2</td></tr>\n"
                       "    <tr><td>text</td><td>2text</td></tr>\n"
                       "    <tr><td>72</td><td>272</td></tr>\n"
                       "    <tr><td>7</td><td>27</td></tr>\n"
                       "</table>") == RemoveFontStyleSection(outputText));
        // just get the first column
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1,
                             0, 0, true);
        CHECK(wxString("<table border='1' style=''>\n"
                       "    <thead><tr style='background:#337BC4; "
                       "color:white;'><td>NAME</td></tr></thead>\n"
                       "    <tr><td>Text</td></tr>\n"
                       "    <tr><td>tExt2</td></tr>\n"
                       "    <tr><td>text</td></tr>\n"
                       "    <tr><td>teXt2</td></tr>\n"
                       "    <tr><td>text</td></tr>\n"
                       "    <tr><td>72</td></tr>\n"
                       "    <tr><td>7</td></tr>\n"
                       "</table>") == RemoveFontStyleSection(outputText));
        // get last column
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1,
                             1, 1, true);
        CHECK(wxString("<table border='1' style=''>\n"
                       "    <thead><tr style='background:#337BC4; "
                       "color:white;'><td>NAME2</td></tr></thead>\n"
                       "    <tr><td>2Text</td></tr>\n"
                       "    <tr><td>2tExt2</td></tr>\n"
                       "    <tr><td>2text</td></tr>\n"
                       "    <tr><td>2teXt2</td></tr>\n"
                       "    <tr><td>2text</td></tr>\n"
                       "    <tr><td>272</td></tr>\n"
                       "    <tr><td>27</td></tr>\n"
                       "</table>") == RemoveFontStyleSection(outputText));
        }
    SECTION("Format to html custom column range bad")
        {
        m_dataProvider->SetSize(7, 2);
        m_dataProvider->SetItemText(
            0, 1, L"2Text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 1, L"2tExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 1, L"2text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 1, L"2teXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 1, L"2text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 1, 272);
        m_dataProvider->SetItemValue(6, 1, 27);
        m_list->SetVirtualDataSize(7, 2);
        m_list->InsertColumn(1, L"NAME2");
        wxString outputText;
        // start bigger then end is nonsense
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1,
                             1, 0, true);
        CHECK(outputText.empty());
        // bogus negative start should be reset to first column
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1,
                             -10, 0, true);
        CHECK(wxString("<table border='1' style=''>\n"
                       "    <thead><tr style='background:#337BC4; "
                       "color:white;'><td>NAME</td></tr></thead>\n"
                       "    <tr><td>Text</td></tr>\n"
                       "    <tr><td>tExt2</td></tr>\n"
                       "    <tr><td>text</td></tr>\n"
                       "    <tr><td>teXt2</td></tr>\n"
                       "    <tr><td>text</td></tr>\n"
                       "    <tr><td>72</td></tr>\n"
                       "    <tr><td>7</td></tr>\n"
                       "</table>") == RemoveFontStyleSection(outputText));
        // bogus (too large) is nonsense
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1,
                             99, 1, true);
        CHECK(outputText.empty());
        // bogus negative end should be reset to last column
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1,
                             1, -10, true);
        CHECK(wxString("<table border='1' style=''>\n"
                       "    <thead><tr style='background:#337BC4; "
                       "color:white;'><td>NAME2</td></tr></thead>\n"
                       "    <tr><td>2Text</td></tr>\n"
                       "    <tr><td>2tExt2</td></tr>\n"
                       "    <tr><td>2text</td></tr>\n"
                       "    <tr><td>2teXt2</td></tr>\n"
                       "    <tr><td>2text</td></tr>\n"
                       "    <tr><td>272</td></tr>\n"
                       "    <tr><td>27</td></tr>\n"
                       "</table>") == RemoveFontStyleSection(outputText));
        // bogus (too big) end should be reset to last column
        m_list->FormatToHtml(outputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1,
                             1, 10, true);
        CHECK(wxString("<table border='1' style=''>\n"
                       "    <thead><tr style='background:#337BC4; "
                       "color:white;'><td>NAME2</td></tr></thead>\n"
                       "    <tr><td>2Text</td></tr>\n"
                       "    <tr><td>2tExt2</td></tr>\n"
                       "    <tr><td>2text</td></tr>\n"
                       "    <tr><td>2teXt2</td></tr>\n"
                       "    <tr><td>2text</td></tr>\n"
                       "    <tr><td>272</td></tr>\n"
                       "    <tr><td>27</td></tr>\n"
                       "</table>") == RemoveFontStyleSection(outputText));
        }
    SECTION("Format to text only selected rows")
        {
        wxString outputText;
        m_list->Select(0);
        m_list->Select(3);
        m_list->Select(6);
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportSelected);
        CHECK(wxString("NAME\nText\nteXt2\n7") == outputText);
        }
    SECTION("Format to text no header")
        {
        wxString outputText;
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 0, -1,
                             false);
        CHECK(wxString("Text\ntExt2\ntext\nteXt2\ntext\n72\n7") == outputText);
        }
    SECTION("Format to text custom row range")
        {
        wxString outputText;
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 3, 5, 0, -1,
                             true);
        CHECK(wxString("NAME\nteXt2\ntext\n72") == outputText);
        }
    SECTION("Format to text custom row range bad")
        {
        wxString outputText;
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 99, 5, 0, -1,
                             true);
        CHECK(outputText.empty());
        // starting point after ending point is nonsense
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 5, 4, 0, -1,
                             true);
        CHECK(outputText.empty());
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 0, 99, 0, -1,
                             true);
        CHECK(outputText == wxString("NAME\nText\ntExt2\ntext\nteXt2\ntext\n72\n7"));
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, -10, -1, 0,
                             -1, true);
        CHECK(outputText == wxString("NAME\nText\ntExt2\ntext\nteXt2\ntext\n72\n7"));
        }
    SECTION("Format to text custom column range")
        {
        m_dataProvider->SetSize(7, 2);
        m_dataProvider->SetItemText(
            0, 1, L"2Text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 1, L"2tExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 1, L"2text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 1, L"2teXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 1, L"2text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 1, 272);
        m_dataProvider->SetItemValue(6, 1, 27);
        m_list->SetVirtualDataSize(7, 2);
        m_list->InsertColumn(1, wxString("NAME2"));
        wxString outputText;
        // get both columns
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 0, -1,
                             true);
        CHECK(outputText == wxString("NAME\tNAME2\nText\t2Text\ntExt2\t2tExt2\ntext\t2text\nteXt2\t"
                                     "2teXt2\ntext\t2text\n72\t272\n7\t27"));
        // just get the first column
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 0, 0,
                             true);
        CHECK(outputText == wxString("NAME\nText\ntExt2\ntext\nteXt2\ntext\n72\n7"));
        // get last column
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 1, 1,
                             true);
        CHECK(outputText == wxString("NAME2\n2Text\n2tExt2\n2text\n2teXt2\n2text\n272\n27"));
        }
    SECTION("Format to text custom column range bad")
        {
        m_dataProvider->SetSize(7, 2);
        m_dataProvider->SetItemText(
            0, 1, L"2Text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 1, L"2tExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 1, L"2text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 1, L"2teXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 1, L"2text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 1, 272);
        m_dataProvider->SetItemValue(6, 1, 27);
        m_list->SetVirtualDataSize(7, 2);
        m_list->InsertColumn(1, L"NAME2");
        wxString outputText;
        // start bigger then end is nonsense
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 1, 0,
                             true);
        CHECK(outputText.empty());
        // bogus negative start should be reset to first column
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, -10, 0,
                             true);
        CHECK(outputText == wxString("NAME\nText\ntExt2\ntext\nteXt2\ntext\n72\n7"));
        // bogus (too large) is nonsense
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 99, 1,
                             true);
        CHECK(outputText.empty());
        // bogus negative end should be reset to last column
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 1, -10,
                             true);
        CHECK(outputText == wxString("NAME2\n2Text\n2tExt2\n2text\n2teXt2\n2text\n272\n27"));
        // bogus (too big) end should be reset to last column
        m_list->FormatToText(outputText, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 1, 10,
                             true);
        CHECK(outputText == wxString("NAME2\n2Text\n2tExt2\n2text\n2teXt2\n2text\n272\n27"));
        }
    }

TEST_CASE("ListCtrlEx", "[listctrlex]")
    {
    auto m_dataProvider = std::make_shared<ListCtrlExNumericDataProvider>();
    auto m_list = new ListCtrlEx(wxTheApp->GetTopWindow(), wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, wxLC_VIRTUAL | wxLC_REPORT | wxBORDER_SUNKEN);
    m_list->Hide();

    const auto Reset = [&m_dataProvider, &m_list]()
    {
        m_dataProvider->SetSize(7, 1);
        m_dataProvider->SetItemText(
            0, 0, L"Text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 0, L"tExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 0, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 0, L"teXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 0, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 0, 72);
        m_dataProvider->SetItemValue(6, 0, 7);
        m_list->SetVirtualDataProvider(m_dataProvider);
        m_list->SetVirtualDataSize(7, 1);
        m_list->InsertColumn(0, L"NAME");
    };
    const auto Reset2Columns = [&m_dataProvider, &m_list]()
    {
        m_dataProvider->SetSize(7, 2);
        m_dataProvider->SetItemText(
            0, 0, L"Text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 0, L"tExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 0, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 0, L"teXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 0, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 0, 72);
        m_dataProvider->SetItemValue(6, 0, 7);
        // other column
        m_dataProvider->SetItemText(
            0, 1, L"Sext",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 1, L"sExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 1, L"sext",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 1, L"seXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 1, L"sext",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 1, 82);
        m_dataProvider->SetItemValue(6, 1, 8);
        m_list->SetVirtualDataProvider(m_dataProvider);
        m_list->SetVirtualDataSize(7, 2);
        m_list->InsertColumn(0, L"NAME");
        m_list->InsertColumn(1, L"OTHER");
    };

    Reset();

    SECTION("Add row")
        {
        CHECK(m_list->AddRow() == 7);
        CHECK(m_list->GetItemCount() == 8);
        CHECK(m_list->AddRow(L"NewItem") == 8);
        CHECK(m_list->GetItemCount() == 9);
        CHECK(m_list->GetItemTextEx(7, 0) == _(L""));
        CHECK(m_list->GetItemTextEx(8, 0) == L"NewItem");
        }

    SECTION("Set sortable range")
        {
        m_list->SetSortableRange(1, 4);
        CHECK(m_list->GetSortableRange().first == 1);
        CHECK(m_list->GetSortableRange().second == 4);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(1, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(2, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(3, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(4, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(5, 0) == L"72");
        CHECK(m_list->GetItemTextEx(6, 0) == L"7");
        m_list->SetSortableRange(0, -1);
        m_list->SortColumn(0, Wisteria::SortDirection::SortDescending);
        CHECK(m_list->GetItemTextEx(0, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(1, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(2, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(3, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(4, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(5, 0) == L"72");
        CHECK(m_list->GetItemTextEx(6, 0) == L"7");

        Reset2Columns();
        m_list->SetSortableRange(0, -1);
        std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        m_list->SortColumns(columns);
        CHECK(m_list->GetItemTextEx(0, 0) == L"7");
        CHECK(m_list->GetItemTextEx(1, 0) == L"72");
        CHECK(m_list->GetItemTextEx(2, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(3, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(4, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(5, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(6, 0).CmpNoCase(L"teXt2") == 0);

        // test multicolumn sorting
        Reset();
        m_list->SetSortableRange(1, 4);
        CHECK(m_list->GetSortableRange().first == 1);
        CHECK(m_list->GetSortableRange().second == 4);
        columns.clear();
        columns.push_back(
            std::pair<size_t, Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        m_list->SortColumns(columns);
        CHECK(m_list->GetItemTextEx(0, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(1, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(2, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(3, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(4, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(5, 0) == L"72");
        CHECK(m_list->GetItemTextEx(6, 0) == L"7");

        Reset();
        m_list->SetSortableRange(5, 6);
        CHECK(m_list->GetSortableRange().first == 5);
        CHECK(m_list->GetSortableRange().second == 6);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(1, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(2, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(3, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(4, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(5, 0) == L"7");
        CHECK(m_list->GetItemTextEx(6, 0) == L"72");

        Reset();
        // -1 as the end of the range should make everything sortable
        m_list->SetSortableRange(0, -1);
        CHECK(m_list->GetSortableRange().first == 0);
        CHECK(m_list->GetSortableRange().second == -1);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0, 0) == L"7");
        CHECK(m_list->GetItemTextEx(1, 0) == L"72");
        CHECK(m_list->GetItemTextEx(2, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(3, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(4, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(5, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(6, 0).CmpNoCase(L"teXt2") == 0);

        Reset();
        // bogus range, should just make everything sortable
        m_list->SetSortableRange(-10, 10);
        CHECK(m_list->GetSortableRange().first == 0);
        CHECK(m_list->GetSortableRange().second == 10);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0, 0) == L"7");
        CHECK(m_list->GetItemTextEx(1, 0) == L"72");
        CHECK(m_list->GetItemTextEx(2, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(3, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(4, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(5, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(6, 0).CmpNoCase(L"teXt2") == 0);

        Reset();
        // bogus range, should just make everything sortable
        m_list->SetSortableRange(0, -10);
        CHECK(m_list->GetSortableRange().first == 0);
        CHECK(m_list->GetSortableRange().second == -1);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0, 0) == L"7");
        CHECK(m_list->GetItemTextEx(1, 0) == L"72");
        CHECK(m_list->GetItemTextEx(2, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(3, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(4, 0).CmpNoCase(L"text") == 0);
        CHECK(m_list->GetItemTextEx(5, 0).CmpNoCase(L"teXt2") == 0);
        CHECK(m_list->GetItemTextEx(6, 0).CmpNoCase(L"teXt2") == 0);

        Reset();
        // bogus range, should make nothing sortable
        m_list->SetSortableRange(10, -1);
        CHECK(m_list->GetSortableRange().first == 10);
        CHECK(m_list->GetSortableRange().second == -1);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0, 0) == L"Text");
        CHECK(m_list->GetItemTextEx(1, 0) == L"tExt2");
        CHECK(m_list->GetItemTextEx(2, 0) == L"text");
        CHECK(m_list->GetItemTextEx(3, 0) == L"teXt2");
        CHECK(m_list->GetItemTextEx(4, 0) == L"text");
        CHECK(m_list->GetItemTextEx(5, 0) == L"72");
        CHECK(m_list->GetItemTextEx(6, 0) == L"7");

        Reset();
        // bogus range, should make nothing sortable
        m_list->SetSortableRange(10, -1);
        CHECK(m_list->GetSortableRange().first == 10);
        CHECK(m_list->GetSortableRange().second == -1);
        m_list->SortColumns(columns);
        CHECK(m_list->GetItemTextEx(0, 0) == L"Text");
        CHECK(m_list->GetItemTextEx(1, 0) == L"tExt2");
        CHECK(m_list->GetItemTextEx(2, 0) == L"text");
        CHECK(m_list->GetItemTextEx(3, 0) == L"teXt2");
        CHECK(m_list->GetItemTextEx(4, 0) == L"text");
        CHECK(m_list->GetItemTextEx(5, 0) == L"72");
        CHECK(m_list->GetItemTextEx(6, 0) == L"7");
        }
    SECTION("Delete item")
        {
        CHECK(m_list->GetItemCount() == 7);
        m_list->DeleteItem(5);
        CHECK(m_list->GetItemCount() == 6);
        }
    SECTION("Delete all items")
        {
        CHECK(m_list->GetItemCount() == 7);
        wxListEvent evt;
        m_list->OnDeleteAllItems(evt);
        CHECK(m_list->GetItemCount() == 0);
        }
    SECTION("Delete selected items")
        {
        CHECK(m_list->GetItemCount() == 7);
        m_list->Select(1);
        m_list->Select(2);
        m_list->Select(3);
        m_list->Select(4);
        m_list->Select(6);
        m_list->DeleteSelectedItems();
        CHECK(m_list->GetItemCount() == 2);
        CHECK(m_list->GetItemTextEx(0, 0) == L"Text");
        CHECK(m_list->GetItemTextEx(1, 0) == L"72");
        }
    SECTION("Select all")
        {
        m_list->SelectAll();
        for (long i = 0; i < m_list->GetItemCount(); ++i)
            {
            CHECK(m_list->IsSelected(i));
            }
        }
    SECTION("Deselect all")
        {
        m_list->SelectAll();
        m_list->DeselectAll();
        for (long i = 0; i < m_list->GetItemCount(); ++i)
            {
            CHECK(m_list->IsSelected(i) == false);
            }
        }
    SECTION("Get column name")
        {
        CHECK(m_list->GetColumnName(0) == L"NAME");
        // bogus values
        CHECK(m_list->GetColumnName(-1) == _(L""));
        CHECK(m_list->GetColumnName(1) == _(L""));
        }
    SECTION("Get selected text")
        {
        m_list->DeselectAll();
        m_list->Select(1);
        CHECK(m_list->GetSelectedText() == L"tExt2");
        m_list->DeselectAll();
        m_list->Select(5);
        CHECK(m_list->GetSelectedText() == L"72");
        }
    SECTION("GetItemTextEx")
        {
        CHECK(m_list->GetItemTextEx(0, 0) == L"Text");
        CHECK(m_list->GetItemTextEx(1, 0) == L"tExt2");
        CHECK(m_list->GetItemTextEx(2, 0) == L"text");
        CHECK(m_list->GetItemTextEx(3, 0) == L"teXt2");
        CHECK(m_list->GetItemTextEx(4, 0) == L"text");
        CHECK(m_list->GetItemTextEx(5, 0) == L"72");
        CHECK(m_list->GetItemTextEx(6, 0) == L"7");

        CHECK(m_list->GetItemTextFormatted(0, 0) == L"Text");
        CHECK(m_list->GetItemTextFormatted(1, 0) == L"tExt2");
        CHECK(m_list->GetItemTextFormatted(2, 0) == L"text");
        CHECK(m_list->GetItemTextFormatted(3, 0) == L"teXt2");
        CHECK(m_list->GetItemTextFormatted(4, 0) == L"text");
        CHECK(m_list->GetItemTextFormatted(5, 0) == L"72");
        CHECK(m_list->GetItemTextFormatted(6, 0) == L"7");
        // assertions are in place to handle out-of-boundary issues.
        // it's too slow to have boundary checks in this function.
        }
    }

TEST_CASE("ListCtrlEx Find", "[listctrlex]")
    {
    auto m_dataProvider = std::make_shared<ListCtrlExNumericDataProvider>();
    auto m_list = new ListCtrlEx(wxTheApp->GetTopWindow(), wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, wxLC_VIRTUAL | wxLC_REPORT | wxBORDER_SUNKEN);
    m_list->Hide();

    const auto Reset = [&m_dataProvider, &m_list]()
    {
        m_dataProvider->SetSize(7, 1);
        m_dataProvider->SetItemText(
            0, 0, L"Text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            1, 0, L"tExt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            2, 0, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            3, 0, L"teXt2",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemText(
            4, 0, L"text",
            NumberFormatInfo{ NumberFormatInfo::NumberFormatType::StandardFormatting },
            std::numeric_limits<double>::quiet_NaN());
        m_dataProvider->SetItemValue(5, 0, 72);
        m_dataProvider->SetItemValue(6, 0, 7);
        m_list->SetVirtualDataProvider(m_dataProvider);
        m_list->SetVirtualDataSize(7, 1);
        m_list->InsertColumn(0, L"NAME");
    };

    Reset();

    SECTION("FindEx")
        {
        m_list->Select(0);
        CHECK(m_list->FindEx(L"text", 0) == 0);
        CHECK(m_list->FindEx(L"text2", 0) == 1);
        CHECK(m_list->FindEx(L"text2", 2) == 3);
        CHECK(m_list->FindEx(L"bogus") == wxNOT_FOUND);
        }
    SECTION("Find column")
        {
        m_list->InsertColumn(1, L"Second");
        CHECK(m_list->FindColumn(L"SeCOnd") == 1);
        CHECK(m_list->FindColumn(L"Name") == 0);
        CHECK(m_list->FindColumn(L"bogus") == wxNOT_FOUND);
        }

    SECTION("On find up case insensitive partial match")
        {
        m_list->Select(6);
        wxFindDialogEvent event;
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFindString(L"Text");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 4);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 3);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 2);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 1);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 0);
        m_list->OnFind(event);
        // wrap around to the beginning
        CHECK(m_list->GetFocusedItem() == 4);
        event.SetFindString(L"7");
        m_list->OnFind(event);
        // wrap around
        CHECK(m_list->GetFocusedItem() == 6);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 5);
        m_list->OnFind(event);
        // wrap around
        CHECK(m_list->GetFocusedItem() == 6);
        }
    SECTION("On find up case sensitive partial match")
        {
        m_list->Select(6);
        wxFindDialogEvent event;
        event.SetFlags(wxFR_MATCHCASE);
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFindString(L"text");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 4);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 2);
        m_list->OnFind(event);
        // wrap around to the beginning
        CHECK(m_list->GetFocusedItem() == 4);
        }
    SECTION("On find up case insensitive full match")
        {
        m_list->Select(6);
        wxFindDialogEvent event;
        event.SetFlags(wxFR_WHOLEWORD);
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFindString(L"Text");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 4);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 2);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 0);
        m_list->OnFind(event);
        // wrap around to the beginning
        CHECK(m_list->GetFocusedItem() == 4);
        event.SetFindString(L"7");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 6);
        }
    SECTION("On find up case sensitive full match")
        {
        m_list->Select(6);
        wxFindDialogEvent event;
        event.SetFlags(wxFR_WHOLEWORD | wxFR_MATCHCASE);
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFindString(L"text");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 4);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 2);
        m_list->OnFind(event);
        // wrap around to the beginning
        CHECK(m_list->GetFocusedItem() == 4);
        }
    SECTION("On find down case insensitive partial match")
        {
        m_list->Select(0);
        wxFindDialogEvent event;
        event.SetFlags(wxFR_DOWN);
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFindString(L"Text");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 1);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 2);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 3);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 4);
        m_list->OnFind(event);
        // wrap around to the beginning
        CHECK(m_list->GetFocusedItem() == 0);
        event.SetFindString(L"7");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 5);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 6);
        m_list->OnFind(event);
        // wrap around
        CHECK(m_list->GetFocusedItem() == 5);
        }
    SECTION("On find down case sensitive partial match")
        {
        m_list->Select(0);
        wxFindDialogEvent event;
        event.SetFlags(wxFR_DOWN | wxFR_MATCHCASE);
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFindString(L"text");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 2);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 4);
        m_list->OnFind(event);
        // wrap around to the beginning
        CHECK(m_list->GetFocusedItem() == 2);
        }
    SECTION("On find down case insensitive full match")
        {
        m_list->Select(0);
        wxFindDialogEvent event;
        event.SetFlags(wxFR_DOWN | wxFR_WHOLEWORD);
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFindString(L"Text");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 2);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 4);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 0);
        m_list->OnFind(event);
        // wrap around to the beginning
        CHECK(m_list->GetFocusedItem() == 2);
        event.SetFindString(L"7");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 6);
        }
    SECTION("On find down case sensitive full match")
        {
        m_list->Select(0);
        wxFindDialogEvent event;
        event.SetFlags(wxFR_DOWN | wxFR_WHOLEWORD | wxFR_MATCHCASE);
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFindString(L"text");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 2);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 4);
        m_list->OnFind(event);
        // wrap around to the beginning
        CHECK(m_list->GetFocusedItem() == 2);
        }
    }

// ParseFormattedNumber uses the system locale to resolve ambiguous cases.
// When both period and comma are present, the last one is the decimal separator.
// These tests assume a US/English locale (decimal separator = period).
TEST_CASE("ListCtrlExcelExporter ParseFormattedNumber",
          "[listctrlexcelexporter][parseformattednumber]")
    {
    double value = 0.0;

    SECTION("Simple integers")
        {
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"123", value));
        CHECK_THAT(value, WithinAbs(123.0, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"0", value));
        CHECK_THAT(value, WithinAbs(0.0, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"-456", value));
        CHECK_THAT(value, WithinAbs(-456.0, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"+789", value));
        CHECK_THAT(value, WithinAbs(789.0, 0.0001));
        }

    SECTION("Simple decimals")
        {
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"123.45", value));
        CHECK_THAT(value, WithinAbs(123.45, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"0.5", value));
        CHECK_THAT(value, WithinAbs(0.5, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"-0.123", value));
        CHECK_THAT(value, WithinAbs(-0.123, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L".5", value));
        CHECK_THAT(value, WithinAbs(0.5, 0.0001));
        }

    SECTION("US format - comma as thousands separator, period as decimal")
        {
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1,234", value));
        CHECK_THAT(value, WithinAbs(1234.0, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1,234.56", value));
        CHECK_THAT(value, WithinAbs(1234.56, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1,234,567.89", value));
        CHECK_THAT(value, WithinAbs(1234567.89, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"-1,234.56", value));
        CHECK_THAT(value, WithinAbs(-1234.56, 0.0001));
        }

    SECTION("European format - both separators present, last one is decimal")
        {
        // both present - last one is decimal separator (unambiguous, locale-independent)
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1.234,56", value));
        CHECK_THAT(value, WithinAbs(1234.56, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1.234.567,89", value));
        CHECK_THAT(value, WithinAbs(1234567.89, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"-1.234,56", value));
        CHECK_THAT(value, WithinAbs(-1234.56, 0.0001));
        }

    SECTION("Currency symbols rejected (exported as text)")
        {
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"$1234.56", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"$1,234.56", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"\u20AC1.234,56", value)); // Euro
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"\u00A31,234.56", value)); // Pound
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"\u00A51234", value));     // Yen
        }

    SECTION("Percent signs rejected (exported as text)")
        {
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"50%", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"12.5%", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"-3.14%", value));
        }

    SECTION("Spaces stripped")
        {
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L" 123 ", value));
        CHECK_THAT(value, WithinAbs(123.0, 0.0001));

        // spaces as thousands separator
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1 234 567", value));
        CHECK_THAT(value, WithinAbs(1234567.0, 0.0001));
        }

    SECTION("Invalid inputs return false")
        {
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"abc", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"hello world", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"-", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"+", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"$", value));
        }

    SECTION("Text labels with embedded numbers should not parse as numbers")
        {
        // labels with numbers in parentheses
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(
            L"Number of difficult sentences (more than 20 words)", value));

        // percentage text descriptions
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"33.3% of all words", value));

        // date/time strings
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(
            L"Last modified    Friday, February 24, 2006 at 06:16 PM", value));

        // common label patterns
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"Row 1", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"Item #42", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"Page 5 of 10", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"Score: 95", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"Test123", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"123Test", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"v2.0", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"2nd place", value));

        // non-Latin alphabetic characters
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"日本語 123", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"Ελληνικά 456", value));
        CHECK_FALSE(ListCtrlExcelExporter::ParseFormattedNumber(L"Кириллица 789", value));
        }

    SECTION("Edge cases")
        {
        // leading zeros
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"007", value));
        CHECK_THAT(value, WithinAbs(7.0, 0.0001));

        // scientific notation (handled by wxString::ToDouble)
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1.5e3", value));
        CHECK_THAT(value, WithinAbs(1500.0, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1.5E-2", value));
        CHECK_THAT(value, WithinAbs(0.015, 0.0001));

        // very large numbers
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"999999999999", value));
        CHECK_THAT(value, WithinAbs(999999999999.0, 1.0));

        // very small decimals
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"0.0001", value));
        CHECK_THAT(value, WithinAbs(0.0001, 0.00001));
        }

    SECTION("Ambiguous single comma - uses locale (US: comma is thousands)")
        {
        // on US locale, lone comma is treated as thousands separator
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1,5", value));
        CHECK_THAT(value, WithinAbs(15.0, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1,234", value));
        CHECK_THAT(value, WithinAbs(1234.0, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1,234,567", value));
        CHECK_THAT(value, WithinAbs(1234567.0, 0.0001));
        }

    SECTION("Ambiguous single period - uses locale (US: period is decimal)")
        {
        // on US locale, lone period is treated as decimal separator
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1.5", value));
        CHECK_THAT(value, WithinAbs(1.5, 0.0001));

        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1.234", value));
        CHECK_THAT(value, WithinAbs(1.234, 0.0001));

        // multiple periods = thousands separators (unambiguous)
        CHECK(ListCtrlExcelExporter::ParseFormattedNumber(L"1.234.567", value));
        CHECK_THAT(value, WithinAbs(1234567.0, 0.0001));
        }
    }

TEST_CASE("ListCtrlExcelExporter preserves text labels with embedded numbers",
          "[listctrlexcelexporter]")
    {
    // create list control
    auto listCtrl = new ListCtrlEx(wxTheApp->GetTopWindow(), wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxLC_REPORT | wxBORDER_SUNKEN);
    listCtrl->Hide();
    listCtrl->InsertColumn(0, L"Label");
    listCtrl->InsertColumn(1, L"Value");

    // add rows with text that contains numbers but should NOT be parsed as numbers
    const std::vector<wxString> testLabels = {
        L"Number of difficult sentences (more than 20 words)",
        L"33.3% of all words",
        L"Last modified    Friday, February 24, 2006 at 06:16 PM",
        L"Row 1",
        L"Page 5 of 10",
        L"v2.0"
    };

    for (size_t i = 0; i < testLabels.size(); ++i)
        {
        const long idx = listCtrl->InsertItem(static_cast<long>(i), testLabels[i]);
        listCtrl->SetItem(idx, 1, wxString::Format(L"%zu", i + 1));
        }

    // export to a temp file
    wxFileName tempFile(wxFileName::GetTempDir(), L"test_export.xlsx");
    ListCtrlExcelExporter exporter;
    REQUIRE(exporter.Export(listCtrl, tempFile, true));

    // read the XLSX (ZIP) and extract sharedStrings.xml
    wxFFileInputStream fileStream(tempFile.GetFullPath());
    REQUIRE(fileStream.IsOk());

    wxZipInputStream zipStream(fileStream);
    REQUIRE(zipStream.IsOk());

    wxString sharedStringsContent;
    wxString allEntryNames;
    wxZipEntry* entry = nullptr;
    while ((entry = zipStream.GetNextEntry()) != nullptr)
        {
        allEntryNames += L"[" + entry->GetName() + L"] ";
        // use Contains for path matching (handles path separator differences)
        if (entry->GetName().Contains(L"sharedStrings.xml"))
            {
            wxStringOutputStream strStream(&sharedStringsContent);
            zipStream.Read(strStream);
            }
        delete entry;
        }

    // debug info
    INFO("All entries: " << allEntryNames.ToStdString());
    INFO("sharedStringsContent length: " << sharedStringsContent.length());

    // verify that each original label appears in the shared strings
    for (const auto& label : testLabels)
        {
        INFO("Checking label: " << label.ToStdString());
        CHECK(sharedStringsContent.Contains(label));
        }

    // clean up
    wxRemoveFile(tempFile.GetFullPath());
    listCtrl->Destroy();
    }

TEST_CASE("ListCtrlExcelExporter exports percentages with format", "[listctrlexcelexporter]")
    {
    // create virtual list control with numeric data provider
    auto dataProvider = std::make_shared<ListCtrlExNumericDataProvider>();
    auto listCtrl = new ListCtrlEx(wxTheApp->GetTopWindow(), wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxLC_VIRTUAL | wxLC_REPORT | wxBORDER_SUNKEN);
    listCtrl->Hide();

    // set up numeric data provider
    dataProvider->SetSize(3, 2);

    // row 0: regular number and percentage
    dataProvider->SetItemValue(0, 0, 100);
    dataProvider->SetItemValue(
        0, 1, 75,
        Wisteria::NumberFormatInfo(
            Wisteria::NumberFormatInfo::NumberFormatType::PercentageFormatting, 0));

    // row 1: decimal percentage
    dataProvider->SetItemValue(1, 0, 200);
    dataProvider->SetItemValue(
        1, 1, 12.5,
        Wisteria::NumberFormatInfo(
            Wisteria::NumberFormatInfo::NumberFormatType::PercentageFormatting, 1));

    // row 2: another percentage
    dataProvider->SetItemValue(2, 0, 300);
    dataProvider->SetItemValue(
        2, 1, 33.33,
        Wisteria::NumberFormatInfo(
            Wisteria::NumberFormatInfo::NumberFormatType::PercentageFormatting, 2));

    listCtrl->SetVirtualDataProvider(dataProvider);
    listCtrl->SetVirtualDataSize(3, 2);
    listCtrl->InsertColumn(0, L"Value");
    listCtrl->InsertColumn(1, L"Percentage");

    // export to a temp file
    wxFileName tempFile(wxFileName::GetTempDir(), L"test_percent_export.xlsx");
    ListCtrlExcelExporter exporter;
    REQUIRE(exporter.Export(listCtrl, tempFile, true));

    // read the XLSX and extract sheet1.xml
    wxFFileInputStream fileStream(tempFile.GetFullPath());
    REQUIRE(fileStream.IsOk());

    wxZipInputStream zipStream(fileStream);
    REQUIRE(zipStream.IsOk());

    wxString sheetContent;
    wxString allEntryNames;
    wxZipEntry* entry = nullptr;
    while ((entry = zipStream.GetNextEntry()) != nullptr)
        {
        allEntryNames += L"[" + entry->GetName() + L"] ";
        // use Contains for path matching
        if (entry->GetName().Contains(L"sheet1.xml"))
            {
            wxStringOutputStream strStream(&sheetContent);
            zipStream.Read(strStream);
            }
        delete entry;
        }

    // debug info
    INFO("All entries: " << allEntryNames.ToStdString());
    INFO("sheetContent length: " << sheetContent.length());

    // verify the percentage values are stored as decimals (divided by 100)
    // 75% should be stored as 0.75
    CHECK(sheetContent.Contains(L"<v>0.75</v>"));
    // 12.5% should be stored as 0.125
    CHECK(sheetContent.Contains(L"<v>0.125</v>"));
    // 33.33% should be stored as 0.3333
    CHECK(sheetContent.Contains(L"<v>0.3333</v>"));

    // verify the regular numbers are stored as-is
    CHECK(sheetContent.Contains(L"<v>100</v>"));
    CHECK(sheetContent.Contains(L"<v>200</v>"));
    CHECK(sheetContent.Contains(L"<v>300</v>"));

    // clean up
    wxRemoveFile(tempFile.GetFullPath());
    listCtrl->Destroy();
    }

TEST_CASE("ListCtrlExcelExporter exports string percentages with format", "[listctrlexcelexporter]")
    {
    // create virtual list control with string data provider
    auto dataProvider = std::make_shared<ListCtrlExDataProvider>();
    auto listCtrl = new ListCtrlEx(wxTheApp->GetTopWindow(), wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxLC_VIRTUAL | wxLC_REPORT | wxBORDER_SUNKEN);
    listCtrl->Hide();

    // set up string data provider with percentage formatting
    dataProvider->SetSize(2, 2);

    // row 0: text "12.5%" with PercentageFormatting
    dataProvider->SetItemText(
        0, 0, L"12.5%",
        Wisteria::NumberFormatInfo(
            Wisteria::NumberFormatInfo::NumberFormatType::PercentageFormatting, 1),
        std::numeric_limits<double>::quiet_NaN());
    dataProvider->SetItemText(0, 1, L"Label",
                              Wisteria::NumberFormatInfo(
                                  Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting),
                              std::numeric_limits<double>::quiet_NaN());

    // row 1: text "50%" with StandardFormatting (should stay as text)
    dataProvider->SetItemText(1, 0, L"50%",
                              Wisteria::NumberFormatInfo(
                                  Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting),
                              std::numeric_limits<double>::quiet_NaN());
    dataProvider->SetItemText(1, 1, L"Another label",
                              Wisteria::NumberFormatInfo(
                                  Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting),
                              std::numeric_limits<double>::quiet_NaN());

    listCtrl->SetVirtualDataProvider(dataProvider);
    listCtrl->SetVirtualDataSize(2, 2);
    listCtrl->InsertColumn(0, L"Percent");
    listCtrl->InsertColumn(1, L"Text");

    // export to a temp file
    wxFileName tempFile(wxFileName::GetTempDir(), L"test_string_percent_export.xlsx");
    ListCtrlExcelExporter exporter;
    REQUIRE(exporter.Export(listCtrl, tempFile, true));

    // read the XLSX and extract sheet and sharedStrings
    wxFFileInputStream fileStream(tempFile.GetFullPath());
    REQUIRE(fileStream.IsOk());

    wxZipInputStream zipStream(fileStream);
    REQUIRE(zipStream.IsOk());

    wxString sheetContent;
    wxString sharedStringsContent;
    wxZipEntry* entry = nullptr;
    while ((entry = zipStream.GetNextEntry()) != nullptr)
        {
        // use Contains for path matching
        if (entry->GetName().Contains(L"sheet1.xml"))
            {
            wxStringOutputStream strStream(&sheetContent);
            zipStream.Read(strStream);
            }
        else if (entry->GetName().Contains(L"sharedStrings.xml"))
            {
            wxStringOutputStream strStream(&sharedStringsContent);
            zipStream.Read(strStream);
            }
        delete entry;
        }

    INFO("sheetContent length: " << sheetContent.length());
    INFO("sharedStringsContent length: " << sharedStringsContent.length());

    // "12.5%" with PercentageFormatting should be stored as 0.125
    CHECK(sheetContent.Contains(L"<v>0.125</v>"));

    // "50%" with StandardFormatting should be in shared strings (as text)
    CHECK(sharedStringsContent.Contains(L"50%"));

    // clean up
    wxRemoveFile(tempFile.GetFullPath());
    listCtrl->Destroy();
    }

TEST_CASE("ListCtrlExcelExporter exports pure numbers as numeric", "[listctrlexcelexporter]")
    {
    // create non-virtual list control
    auto listCtrl = new ListCtrlEx(wxTheApp->GetTopWindow(), wxID_ANY, wxDefaultPosition,
                                   wxDefaultSize, wxLC_REPORT | wxBORDER_SUNKEN);
    listCtrl->Hide();
    listCtrl->InsertColumn(0, L"Number");
    listCtrl->InsertColumn(1, L"Text");

    // add rows with pure numbers and text with numbers
    listCtrl->InsertItem(0, L"123.45");
    listCtrl->SetItem(0, 1, L"Not a number: 123");

    listCtrl->InsertItem(1, L"-456.78");
    listCtrl->SetItem(1, 1, L"Also text: -456");

    listCtrl->InsertItem(2, L"0.5");
    listCtrl->SetItem(2, 1, L"50%"); // contains %, should be text

    // export to a temp file
    wxFileName tempFile(wxFileName::GetTempDir(), L"test_numeric_export.xlsx");
    ListCtrlExcelExporter exporter;
    REQUIRE(exporter.Export(listCtrl, tempFile, true));

    // read the XLSX and extract both sheet and sharedStrings
    wxFFileInputStream fileStream(tempFile.GetFullPath());
    REQUIRE(fileStream.IsOk());

    wxZipInputStream zipStream(fileStream);
    REQUIRE(zipStream.IsOk());

    wxString sheetContent;
    wxString sharedStringsContent;
    wxZipEntry* entry = nullptr;
    while ((entry = zipStream.GetNextEntry()) != nullptr)
        {
        // use Contains for path matching
        if (entry->GetName().Contains(L"sheet1.xml"))
            {
            wxStringOutputStream strStream(&sheetContent);
            zipStream.Read(strStream);
            }
        else if (entry->GetName().Contains(L"sharedStrings.xml"))
            {
            wxStringOutputStream strStream(&sharedStringsContent);
            zipStream.Read(strStream);
            }
        delete entry;
        }

    INFO("sheetContent length: " << sheetContent.length());
    INFO("sharedStringsContent length: " << sharedStringsContent.length());

    // verify pure numbers are in the sheet as numeric values (not in shared strings)
    CHECK(sheetContent.Contains(L"<v>123.45</v>"));
    CHECK(sheetContent.Contains(L"<v>-456.78</v>"));
    CHECK(sheetContent.Contains(L"<v>0.5</v>"));

    // verify text with numbers is in shared strings
    CHECK(sharedStringsContent.Contains(L"Not a number: 123"));
    CHECK(sharedStringsContent.Contains(L"Also text: -456"));
    CHECK(sharedStringsContent.Contains(L"50%")); // preserved as text

    // clean up
    wxRemoveFile(tempFile.GetFullPath());
    listCtrl->Destroy();
    }
