#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../src/ui/controls/listctrlexdataprovider.h"
#include "../../src/ui/controls/listctrlex.h"

using namespace Catch::Matchers;

TEST_CASE("ListCtrlExNumericDataProvider", "[listctrlexnumericdataprovider]")
    {
    ListCtrlExNumericDataProvider dataProvider;

    SECTION("Set items")
        {
        class SimpleFormat : public Wisteria::NumberFormat<wxString>
            {
        public:
            wxString GetFormattedValue(const wxString& value, const Wisteria::NumberFormatInfo&) const
                { return value; }
            wxString GetFormattedValue(const double value, const Wisteria::NumberFormatInfo& format) const
                { return wxNumberFormatter::ToString(value, format.m_precision, 1); }
            };

        SimpleFormat numForm;
        dataProvider.SetNumberFormatter(&numForm);
        dataProvider.SetSize(5,2);
        dataProvider.SetItemValue(0,0,1);
        dataProvider.SetItemText(0,1,L"first");
        dataProvider.SetItemValue(1,0,2);
        dataProvider.SetItemText(1,1,L"second");
        dataProvider.SetItemValue(2,0,3);
        dataProvider.SetItemText(2,1,L"third");
        dataProvider.SetItemValue(3,0,76,Wisteria::NumberFormatInfo(Wisteria::NumberFormatInfo::NumberFormatType::PercentageFormatting,1,true));
        dataProvider.SetItemValue(4,0,76.25,Wisteria::NumberFormatInfo(Wisteria::NumberFormatInfo::NumberFormatType::CustomFormatting, 2));
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"first");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"second");
        CHECK(dataProvider.GetItemText(2,0) == L"3");
        CHECK(dataProvider.GetItemText(2,1) == L"third");
        CHECK(dataProvider.GetItemText(3,0) == L"76%");
        CHECK(dataProvider.GetItemText(4,0) == L"76.25");
        CHECK(dataProvider.GetItemCount() == 5);
        CHECK(dataProvider.GetColumnCount() == 2);
        }
    SECTION("Delete items")
        {
        dataProvider.SetSize(3,2);
        dataProvider.SetItemValue(0,0,1);
        dataProvider.SetItemText(0,1,L"first");
        dataProvider.SetItemValue(1,0,2);
        dataProvider.SetItemText(1,1,L"second");
        dataProvider.SetItemValue(2,0,3);
        dataProvider.SetItemText(2,1,L"third");
        dataProvider.DeleteItem(1);
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"first");
        CHECK(dataProvider.GetItemText(1,0) == L"3");
        CHECK(dataProvider.GetItemText(1,1) == L"third");
        CHECK(dataProvider.GetItemCount() == 2);
        CHECK(dataProvider.GetColumnCount() == 2);
        dataProvider.DeleteAllItems();
        CHECK(dataProvider.GetItemCount() == 0);
        }
    SECTION("Sort items")
        {
        dataProvider.SetSize(3,2);
        dataProvider.SetItemValue(0,0,1);
        dataProvider.SetItemText(0,1,L"a");
        dataProvider.SetItemValue(1,0,2);
        dataProvider.SetItemText(1,1,L"c");
        dataProvider.SetItemValue(2,0,3);
        dataProvider.SetItemText(2,1,L"b");
        dataProvider.Sort(1, Wisteria::SortDirection::SortDescending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"2");
        CHECK(dataProvider.GetItemText(0,1) == L"c");
        CHECK(dataProvider.GetItemText(1,0) == L"3");
        CHECK(dataProvider.GetItemText(1,1) == L"b");
        CHECK(dataProvider.GetItemText(2,0) == L"1");
        CHECK(dataProvider.GetItemText(2,1) == L"a");
        dataProvider.Sort(1, Wisteria::SortDirection::SortAscending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(2,0) == L"2");
        CHECK(dataProvider.GetItemText(2,1) == L"c");
        CHECK(dataProvider.GetItemText(1,0) == L"3");
        CHECK(dataProvider.GetItemText(1,1) == L"b");
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"a");
        dataProvider.Sort(0, Wisteria::SortDirection::SortDescending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(2,0) == L"1");
        CHECK(dataProvider.GetItemText(2,1) == L"a");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"c");
        CHECK(dataProvider.GetItemText(0,0) == L"3");
        CHECK(dataProvider.GetItemText(0,1) == L"b");
        dataProvider.Sort(0, Wisteria::SortDirection::SortAscending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"a");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"c");
        CHECK(dataProvider.GetItemText(2,0) == L"3");
        CHECK(dataProvider.GetItemText(2,1) == L"b");
        // bogus column, should silently fail
        dataProvider.Sort(2, Wisteria::SortDirection::SortAscending,0,(size_t)-1);
        }
    SECTION("Sort items mixed data")
        {
        dataProvider.SetSize(10,1);
        dataProvider.SetItemValue(0,0,1);
        dataProvider.SetItemText(1,0,L"a");
        dataProvider.SetItemValue(2,0,2);
        dataProvider.SetItemText(3,0,L"c");
        dataProvider.SetItemValue(4,0,11);
        dataProvider.SetItemText(5,0,L"");
        dataProvider.SetItemValue(6,0,2);
        dataProvider.SetItemText(7,0,L"a");
        dataProvider.SetItemText(8,0,L"B");
        dataProvider.SetItemText(9,0,L"Z", Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting, 4);//should actually sort as 4
        dataProvider.Sort(0, Wisteria::SortDirection::SortDescending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"c");
        CHECK(dataProvider.GetItemText(1,0) == L"B");
        CHECK(dataProvider.GetItemText(2,0) == L"a");
        CHECK(dataProvider.GetItemText(3,0) == L"a");
        CHECK(dataProvider.GetItemText(4,0) == L"11");
        CHECK(dataProvider.GetItemText(5,0) == L"Z");
        CHECK(dataProvider.GetItemText(6,0) == L"2");
        CHECK(dataProvider.GetItemText(7,0) == L"2");
        CHECK(dataProvider.GetItemText(8,0) == L"1");
        CHECK(dataProvider.GetItemText(9,0) == L"");
        dataProvider.Sort(0, Wisteria::SortDirection::SortAscending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"");
        CHECK(dataProvider.GetItemText(1,0) == L"1");
        CHECK(dataProvider.GetItemText(2,0) == L"2");
        CHECK(dataProvider.GetItemText(3,0) == L"2");
        CHECK(dataProvider.GetItemText(4,0) == L"Z");
        CHECK(dataProvider.GetItemText(5,0) == L"11");
        CHECK(dataProvider.GetItemText(6,0) == L"a");
        CHECK(dataProvider.GetItemText(7,0) == L"a");
        CHECK(dataProvider.GetItemText(8,0) == L"B");
        CHECK(dataProvider.GetItemText(9,0) == L"c");
        }
    SECTION("Sort items multicolumn first")
        {
        dataProvider.SetSize(4,2);
        dataProvider.SetItemValue(0,0,1);
        dataProvider.SetItemText(0,1,L"first");
        dataProvider.SetItemValue(1,0,2);
        dataProvider.SetItemText(1,1,L"second");
        dataProvider.SetItemValue(2,0,1);
        dataProvider.SetItemText(2,1,L"third");
        dataProvider.SetItemValue(3,0,2);
        dataProvider.SetItemText(3,1,L"fourth");
        std::vector<std::pair<size_t,Wisteria::SortDirection> > columns;
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"2");
        CHECK(dataProvider.GetItemText(0,1) == L"second");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"fourth");
        CHECK(dataProvider.GetItemText(2,0) == L"1");
        CHECK(dataProvider.GetItemText(2,1) == L"third");
        CHECK(dataProvider.GetItemText(3,0) == L"1");
        CHECK(dataProvider.GetItemText(3,1) == L"first");

        columns.clear();
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"first");
        CHECK(dataProvider.GetItemText(1,0) == L"1");
        CHECK(dataProvider.GetItemText(1,1) == L"third");
        CHECK(dataProvider.GetItemText(2,0) == L"2");
        CHECK(dataProvider.GetItemText(2,1) == L"fourth");
        CHECK(dataProvider.GetItemText(3,0) == L"2");
        CHECK(dataProvider.GetItemText(3,1) == L"second");

        // bogus column, should silently fail
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(2, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns,0,(size_t)-1);
        }
    SECTION("Sort items multicolumn second")
        {
        dataProvider.SetSize(5,2);
        dataProvider.SetItemValue(0,0,1);
        dataProvider.SetItemText(0,1,L"text");
        dataProvider.SetItemValue(1,0,2);
        dataProvider.SetItemText(1,1,L"text2");
        dataProvider.SetItemValue(2,0,3);
        dataProvider.SetItemText(2,1,L"text");
        dataProvider.SetItemValue(3,0,4);
        dataProvider.SetItemText(3,1,L"text2");
        dataProvider.SetItemValue(4,0,5);
        dataProvider.SetItemText(4,1,L"text");
        std::vector<std::pair<size_t,Wisteria::SortDirection> > columns;
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"text");
        CHECK(dataProvider.GetItemText(1,0) == L"3");
        CHECK(dataProvider.GetItemText(1,1) == L"text");
        CHECK(dataProvider.GetItemText(2,0) == L"5");
        CHECK(dataProvider.GetItemText(2,1) == L"text");
        CHECK(dataProvider.GetItemText(3,0) == L"2");
        CHECK(dataProvider.GetItemText(3,1) == L"text2");
        CHECK(dataProvider.GetItemText(4,0) == L"4");
        CHECK(dataProvider.GetItemText(4,1) == L"text2");

        columns.clear();
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"4");
        CHECK(dataProvider.GetItemText(0,1) == L"text2");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"text2");
        CHECK(dataProvider.GetItemText(2,0) == L"5");
        CHECK(dataProvider.GetItemText(2,1) == L"text");
        CHECK(dataProvider.GetItemText(3,0) == L"3");
        CHECK(dataProvider.GetItemText(3,1) == L"text");
        CHECK(dataProvider.GetItemText(4,0) == L"1");
        CHECK(dataProvider.GetItemText(4,1) == L"text");

        // bogus column, should silently fail
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(2, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns,0,(size_t)-1);
        }
    SECTION("Sort items multicolumn nothing to sort second column descending numeric")
        {
        dataProvider.SetSize(4,2);
        dataProvider.SetItemValue(0,0,2);
        dataProvider.SetItemValue(0,1,2);
        dataProvider.SetItemValue(1,0,2);
        dataProvider.SetItemValue(1,1,9);
        dataProvider.SetItemValue(2,0,2);
        dataProvider.SetItemValue(2,1,2);
        dataProvider.SetItemValue(3,0,2);
        dataProvider.SetItemValue(3,1,2);
        std::vector<std::pair<size_t,Wisteria::SortDirection> > columns;
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"2");
        CHECK(dataProvider.GetItemText(0,1) == L"9");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"2");
        CHECK(dataProvider.GetItemText(2,0) == L"2");
        CHECK(dataProvider.GetItemText(2,1) == L"2");
        CHECK(dataProvider.GetItemText(3,0) == L"2");
        CHECK(dataProvider.GetItemText(3,1) == L"2");
        }
    SECTION("Sort items multicolumn nothing to sort second column ascending numeric")
        {
        dataProvider.SetSize(4,2);
        dataProvider.SetItemValue(0,0,2);
        dataProvider.SetItemValue(0,1,2);
        dataProvider.SetItemValue(1,0,2);
        dataProvider.SetItemValue(1,1,9);
        dataProvider.SetItemValue(2,0,2);
        dataProvider.SetItemValue(2,1,2);
        dataProvider.SetItemValue(3,0,2);
        dataProvider.SetItemValue(3,1,2);
        std::vector<std::pair<size_t,Wisteria::SortDirection> > columns;
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"2");
        CHECK(dataProvider.GetItemText(0,1) == L"2");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"2");
        CHECK(dataProvider.GetItemText(2,0) == L"2");
        CHECK(dataProvider.GetItemText(2,1) == L"2");
        CHECK(dataProvider.GetItemText(3,0) == L"2");
        CHECK(dataProvider.GetItemText(3,1) == L"9");
        }
    SECTION("Empty data")
        {
        dataProvider.SetSize(10,1);
        for (size_t i = 0; i < 10; ++i)
            { CHECK(dataProvider.GetItemText(i,0) == L""); }
        }
    SECTION("Find item")
        {
        dataProvider.SetSize(3,2);
        dataProvider.SetItemValue(0,0,1);
        dataProvider.SetItemText(0,1,L"first2");
        dataProvider.SetItemText(1,0,L"second");
        dataProvider.SetItemText(1,1,L"second2");
        dataProvider.SetItemText(2,0,L"third");
        dataProvider.SetItemText(2,1,L"third2");
        CHECK(dataProvider.Find(L"second") == 1);
        }
    }

TEST_CASE("ListCtrlExDataProvider", "[listctrlexdataprovider]")
    {
    ListCtrlExDataProvider dataProvider;

    SECTION("Set items")
        {
        dataProvider.SetSize(3,2);
        dataProvider.SetItemText(0,0,L"first");
        dataProvider.SetItemText(0,1,L"first2");
        dataProvider.SetItemText(1,0,L"second");
        dataProvider.SetItemText(1,1,L"second2");
        dataProvider.SetItemText(2,0,L"third");
        dataProvider.SetItemText(2,1,L"third2");
        CHECK(dataProvider.GetItemText(0,0) == L"first");
        CHECK(dataProvider.GetItemText(0,1) == L"first2");
        CHECK(dataProvider.GetItemText(1,0) == L"second");
        CHECK(dataProvider.GetItemText(1,1) == L"second2");
        CHECK(dataProvider.GetItemText(2,0) == L"third");
        CHECK(dataProvider.GetItemText(2,1) == L"third2");
        CHECK(dataProvider.GetItemCount() == 3);
        CHECK(dataProvider.GetColumnCount() == 2);
        }
    SECTION("Delete items")
        {
        dataProvider.SetSize(3,2);
        dataProvider.SetItemText(0,0,L"first");
        dataProvider.SetItemText(0,1,L"first2");
        dataProvider.SetItemText(1,0,L"second");
        dataProvider.SetItemText(1,1,L"second2");
        dataProvider.SetItemText(2,0,L"third");
        dataProvider.SetItemText(2,1,L"third2");
        dataProvider.DeleteItem(1);
        CHECK(dataProvider.GetItemText(0,0) == L"first");
        CHECK(dataProvider.GetItemText(0,1) == L"first2");
        CHECK(dataProvider.GetItemText(1,0) == L"third");
        CHECK(dataProvider.GetItemText(1,1) == L"third2");
        CHECK(dataProvider.GetItemCount() == 2);
        CHECK(dataProvider.GetColumnCount() == 2);
        dataProvider.DeleteAllItems();
        CHECK(dataProvider.GetItemCount() == 0);
        }
    SECTION("Sort items")
        {
        dataProvider.SetSize(3,2);
        dataProvider.SetItemText(0,0,L"1");
        dataProvider.SetItemText(0,1,L"first2");
        dataProvider.SetItemText(1,0,L"2");
        dataProvider.SetItemText(1,1,L"second2");
        dataProvider.SetItemText(2,0,L"11");
        dataProvider.SetItemText(2,1,L"third2");
        dataProvider.Sort(0,Wisteria::SortDirection::SortDescending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"11");
        CHECK(dataProvider.GetItemText(0,1) == L"third2");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"second2");
        CHECK(dataProvider.GetItemText(2,0) == L"1");
        CHECK(dataProvider.GetItemText(2,1) == L"first2");
        dataProvider.Sort(0, Wisteria::SortDirection::SortAscending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"first2");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"second2");
        CHECK(dataProvider.GetItemText(2,0) == L"11");
        CHECK(dataProvider.GetItemText(2,1) == L"third2");
        dataProvider.Sort(1, Wisteria::SortDirection::SortDescending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(2,0) == L"1");
        CHECK(dataProvider.GetItemText(2,1) == L"first2");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"second2");
        CHECK(dataProvider.GetItemText(0,0) == L"11");
        CHECK(dataProvider.GetItemText(0,1) == L"third2");
        dataProvider.Sort(1, Wisteria::SortDirection::SortAscending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"first2");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"second2");
        CHECK(dataProvider.GetItemText(2,0) == L"11");
        CHECK(dataProvider.GetItemText(2,1) == L"third2");
        // bogus column, should silently fail
        dataProvider.Sort(2, Wisteria::SortDirection::SortAscending,0,(size_t)-1);
        }
    SECTION("Sort items multicolumn nothing to sort second column descending")
        {
        dataProvider.SetSize(4,2);
        dataProvider.SetItemText(0,0,L"2");
        dataProvider.SetItemText(0,1,L"two");
        dataProvider.SetItemText(1,0,L"2");
        dataProvider.SetItemText(1,1,L"zzz");
        dataProvider.SetItemText(2,0,L"2");
        dataProvider.SetItemText(2,1,L"two");
        dataProvider.SetItemText(3,0,L"2");
        dataProvider.SetItemText(3,1,L"two");
        std::vector<std::pair<size_t,Wisteria::SortDirection> > columns;
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"2");
        CHECK(dataProvider.GetItemText(0,1) == L"zzz");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"two");
        CHECK(dataProvider.GetItemText(2,0) == L"2");
        CHECK(dataProvider.GetItemText(2,1) == L"two");
        CHECK(dataProvider.GetItemText(3,0) == L"2");
        CHECK(dataProvider.GetItemText(3,1) == L"two");
        dataProvider.Sort(0, Wisteria::SortDirection::SortDescending,0,(size_t)-1);
        dataProvider.Sort(1, Wisteria::SortDirection::SortDescending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"2");
        CHECK(dataProvider.GetItemText(0,1) == L"zzz");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"two");
        CHECK(dataProvider.GetItemText(2,0) == L"2");
        CHECK(dataProvider.GetItemText(2,1) == L"two");
        CHECK(dataProvider.GetItemText(3,0) == L"2");
        CHECK(dataProvider.GetItemText(3,1) == L"two");
        }
    SECTION("Sort items multicolumn nothing to sort second column ascending")
        {
        dataProvider.SetSize(4,2);
        dataProvider.SetItemText(0,0,L"2");
        dataProvider.SetItemText(0,1,L"two");
        dataProvider.SetItemText(1,0,L"2");
        dataProvider.SetItemText(1,1,L"zzz");
        dataProvider.SetItemText(2,0,L"2");
        dataProvider.SetItemText(2,1,L"two");
        dataProvider.SetItemText(3,0,L"2");
        dataProvider.SetItemText(3,1,L"two");
        std::vector<std::pair<size_t,Wisteria::SortDirection> > columns;
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"2");
        CHECK(dataProvider.GetItemText(0,1) == L"two");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"two");
        CHECK(dataProvider.GetItemText(2,0) == L"2");
        CHECK(dataProvider.GetItemText(2,1) == L"two");
        CHECK(dataProvider.GetItemText(3,0) == L"2");
        CHECK(dataProvider.GetItemText(3,1) == L"zzz");
        dataProvider.Sort(0, Wisteria::SortDirection::SortAscending,0,(size_t)-1);
        dataProvider.Sort(1, Wisteria::SortDirection::SortAscending,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"2");
        CHECK(dataProvider.GetItemText(0,1) == L"two");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"two");
        CHECK(dataProvider.GetItemText(2,0) == L"2");
        CHECK(dataProvider.GetItemText(2,1) == L"two");
        CHECK(dataProvider.GetItemText(3,0) == L"2");
        CHECK(dataProvider.GetItemText(3,1) == L"zzz");
        }
    SECTION("Sort items multicolumn first")
        {
        dataProvider.SetSize(4,2);
        dataProvider.SetItemText(0,0,L"1");
        dataProvider.SetItemText(0,1,L"first");
        dataProvider.SetItemText(1,0,L"2");
        dataProvider.SetItemText(1,1,L"second");
        dataProvider.SetItemText(2,0,L"1");
        dataProvider.SetItemText(2,1,L"third");
        dataProvider.SetItemText(3,0,L"2");
        dataProvider.SetItemText(3,1,L"fourth");
        std::vector<std::pair<size_t,Wisteria::SortDirection> > columns;
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"2");
        CHECK(dataProvider.GetItemText(0,1) == L"second");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"fourth");
        CHECK(dataProvider.GetItemText(2,0) == L"1");
        CHECK(dataProvider.GetItemText(2,1) == L"third");
        CHECK(dataProvider.GetItemText(3,0) == L"1");
        CHECK(dataProvider.GetItemText(3,1) == L"first");

        columns.clear();
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"first");
        CHECK(dataProvider.GetItemText(1,0) == L"1");
        CHECK(dataProvider.GetItemText(1,1) == L"third");
        CHECK(dataProvider.GetItemText(2,0) == L"2");
        CHECK(dataProvider.GetItemText(2,1) == L"fourth");
        CHECK(dataProvider.GetItemText(3,0) == L"2");
        CHECK(dataProvider.GetItemText(3,1) == L"second");

        // bogus column, should silently fail
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(2, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns,0,(size_t)-1);
        }
    SECTION("Sort items multicolumn second")
        {
        dataProvider.SetSize(5,2);
        dataProvider.SetItemText(0,0,L"1");
        dataProvider.SetItemText(0,1,L"text");
        dataProvider.SetItemText(1,0,L"2");
        dataProvider.SetItemText(1,1,L"text2");
        dataProvider.SetItemText(2,0,L"3");
        dataProvider.SetItemText(2,1,L"text");
        dataProvider.SetItemText(3,0,L"4");
        dataProvider.SetItemText(3,1,L"text2");
        dataProvider.SetItemText(4,0,L"5");
        dataProvider.SetItemText(4,1,L"text");
        std::vector<std::pair<size_t,Wisteria::SortDirection> > columns;
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"text");
        CHECK(dataProvider.GetItemText(1,0) == L"3");
        CHECK(dataProvider.GetItemText(1,1) == L"text");
        CHECK(dataProvider.GetItemText(2,0) == L"5");
        CHECK(dataProvider.GetItemText(2,1) == L"text");
        CHECK(dataProvider.GetItemText(3,0) == L"2");
        CHECK(dataProvider.GetItemText(3,1) == L"text2");
        CHECK(dataProvider.GetItemText(4,0) == L"4");
        CHECK(dataProvider.GetItemText(4,1) == L"text2");

        columns.clear();
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortDescending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"4");
        CHECK(dataProvider.GetItemText(0,1) == L"text2");
        CHECK(dataProvider.GetItemText(1,0) == L"2");
        CHECK(dataProvider.GetItemText(1,1) == L"text2");
        CHECK(dataProvider.GetItemText(2,0) == L"5");
        CHECK(dataProvider.GetItemText(2,1) == L"text");
        CHECK(dataProvider.GetItemText(3,0) == L"3");
        CHECK(dataProvider.GetItemText(3,1) == L"text");
        CHECK(dataProvider.GetItemText(4,0) == L"1");
        CHECK(dataProvider.GetItemText(4,1) == L"text");

        // bogus column, should silently fail
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(2, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns,0,(size_t)-1);
        }
    SECTION("Sort items multicolumn mixed")
        {
        dataProvider.SetSize(3,2);
        dataProvider.SetItemText(0,0,L"1");
        dataProvider.SetItemText(0,1,L"text333");
        dataProvider.SetItemText(1,0,L"1");
        dataProvider.SetItemText(1,1,L"text33");
        dataProvider.SetItemText(2,0,L"1");
        dataProvider.SetItemText(2,1,L"text3");
        std::vector<std::pair<size_t,Wisteria::SortDirection> > columns;
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"text3");
        CHECK(dataProvider.GetItemText(1,0) == L"1");
        CHECK(dataProvider.GetItemText(1,1) == L"text33");
        CHECK(dataProvider.GetItemText(2,0) == L"1");
        CHECK(dataProvider.GetItemText(2,1) == L"text333");
        }
    SECTION("Sort items multicolumn mixed numeric")
        {
        dataProvider.SetSize(3,2);
        dataProvider.SetItemText(0,0,L"1");
        dataProvider.SetItemText(0,1,L"text333");
        dataProvider.SetItemText(1,0,L"1");
        dataProvider.SetItemText(1,1,L"text33");
        dataProvider.SetItemText(2,0,L"1");
        dataProvider.SetItemText(2,1,L"text3");
        std::vector<std::pair<size_t,Wisteria::SortDirection> > columns;
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(1, Wisteria::SortDirection::SortAscending));
        dataProvider.Sort(columns,0,(size_t)-1);
        CHECK(dataProvider.GetItemText(0,0) == L"1");
        CHECK(dataProvider.GetItemText(0,1) == L"text3");
        CHECK(dataProvider.GetItemText(1,0) == L"1");
        CHECK(dataProvider.GetItemText(1,1) == L"text33");
        CHECK(dataProvider.GetItemText(2,0) == L"1");
        CHECK(dataProvider.GetItemText(2,1) == L"text333");
        }
    SECTION("Find item")
        {
        dataProvider.SetSize(3,2);
        dataProvider.SetItemText(0,0,L"first");
        dataProvider.SetItemText(0,1,L"first2");
        dataProvider.SetItemText(1,0,L"second");
        dataProvider.SetItemText(1,1,L"second2");
        dataProvider.SetItemText(2,0,L"third");
        dataProvider.SetItemText(2,1,L"third2");
        CHECK(dataProvider.Find(L"second") == 1);
        }
    }

TEST_CASE("ListCtrlEx", "[listctrlex]")
    {
    auto m_dataProvider = new ListCtrlExNumericDataProvider;
    auto m_list = new ListCtrlEx(wxTheApp->GetTopWindow(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_VIRTUAL | wxLC_REPORT | wxBORDER_SUNKEN);
    m_list->Hide();

    const auto Reset = [&m_dataProvider, &m_list]()
        {
        m_dataProvider->SetSize(7,1);
        m_dataProvider->SetItemText(0,0,L"Text");
        m_dataProvider->SetItemText(1,0,L"tExt2");
        m_dataProvider->SetItemText(2,0,L"text");
        m_dataProvider->SetItemText(3,0,L"teXt2");
        m_dataProvider->SetItemText(4,0,L"text");
        m_dataProvider->SetItemValue(5,0,72);
        m_dataProvider->SetItemValue(6,0,7);
        m_list->SetVirtualDataProvider(m_dataProvider);
        m_list->SetVirtualDataSize(7, 1);
        m_list->InsertColumn(0, _("NAME"));
        };
    const auto Reset2Columns = [&m_dataProvider, &m_list]()
        {
        m_dataProvider->SetSize(7,2);
        m_dataProvider->SetItemText(0,0,L"Text");
        m_dataProvider->SetItemText(1,0,L"tExt2");
        m_dataProvider->SetItemText(2,0,L"text");
        m_dataProvider->SetItemText(3,0,L"teXt2");
        m_dataProvider->SetItemText(4,0,L"text");
        m_dataProvider->SetItemValue(5,0,72);
        m_dataProvider->SetItemValue(6,0,7);
        // other column
        m_dataProvider->SetItemText(0,1,L"Sext");
        m_dataProvider->SetItemText(1,1,L"sExt2");
        m_dataProvider->SetItemText(2,1,L"sext");
        m_dataProvider->SetItemText(3,1,L"seXt2");
        m_dataProvider->SetItemText(4,1,L"sext");
        m_dataProvider->SetItemValue(5,1,82);
        m_dataProvider->SetItemValue(6,1,8);
        m_list->SetVirtualDataProvider(m_dataProvider);
        m_list->SetVirtualDataSize(7, 2);
        m_list->InsertColumn(0, _("NAME"));
        m_list->InsertColumn(1, _("OTHER"));
        };

    Reset();

    SECTION("Add row")
        {
        CHECK(m_list->AddRow() == 7);
        CHECK(m_list->GetItemCount() == 8);
        CHECK(m_list->AddRow(wxT("NewItem")) == 8);
        CHECK(m_list->GetItemCount() == 9);
        CHECK(m_list->GetItemTextEx(7,0) == _(""));
        CHECK(m_list->GetItemTextEx(8,0) == _("NewItem"));
        }
    SECTION("Format to html only selected rows")
        {
        wxString ouputText;
        m_list->Select(0);
        m_list->Select(3);
        m_list->Select(6);
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportSelected);
        CHECK(ouputText == wxString(
            "<table border='1' style='font-family:Segoe UI; font-size:9pt; border-collapse:collapse;'>\n"
            "    <thead><tr style='background:#337BC4; color:white;'><td>NAME</td></tr></thead>\n"
            "    <tr><td>Text</td></tr>\n"
            "    <tr><td>teXt2</td></tr>\n"
            "    <tr><td>7</td></tr>\n"
            "</table>"));
        }
    SECTION("Format to html no header")
        {
        wxString ouputText;
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportAll, 0, -1, 0, -1, false);
        CHECK(ouputText == wxString(
            "<table border='1' style='font-family:Segoe UI; font-size:9pt; border-collapse:collapse;'>\n"
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
        wxString ouputText;
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 3, 5, 0, -1, true);
        CHECK(ouputText == wxString(
            "<table border='1' style='font-family:Segoe UI; font-size:9pt; border-collapse:collapse;'>\n"
            "    <thead><tr style='background:#337BC4; color:white;'><td>NAME</td></tr></thead>\n"
            "    <tr><td>teXt2</td></tr>\n"
            "    <tr><td>text</td></tr>\n"
            "    <tr><td>72</td></tr>\n"
            "</table>"));
        }
    SECTION("Format to html custom row range bad")
        {
        wxString ouputText;
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 99, 5, 0, -1, true);
        CHECK(ouputText == wxString(wxT("")));
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 5, 4, 0, -1, true);//starting point after ending point is nonsense
        CHECK(ouputText == wxString(wxT("")));
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, 99, 0, -1, true);
        CHECK(ouputText == wxString(
            "<table border='1' style='font-family:Segoe UI; font-size:9pt; border-collapse:collapse;'>\n"
            "    <thead><tr style='background:#337BC4; color:white;'><td>NAME</td></tr></thead>\n"
            "    <tr><td>Text</td></tr>\n"
            "    <tr><td>tExt2</td></tr>\n"
            "    <tr><td>text</td></tr>\n"
            "    <tr><td>teXt2</td></tr>\n"
            "    <tr><td>text</td></tr>\n"
            "    <tr><td>72</td></tr>\n"
            "    <tr><td>7</td></tr>\n"
            "</table>"));
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, -10, -1, 0, -1, true);
        CHECK(ouputText == wxString(
            "<table border='1' style='font-family:Segoe UI; font-size:9pt; border-collapse:collapse;'>\n"
            "    <thead><tr style='background:#337BC4; color:white;'><td>NAME</td></tr></thead>\n"
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
        m_dataProvider->SetSize(7,2);
        m_dataProvider->SetItemText(0,1,L"2Text");
        m_dataProvider->SetItemText(1,1,L"2tExt2");
        m_dataProvider->SetItemText(2,1,L"2text");
        m_dataProvider->SetItemText(3,1,L"2teXt2");
        m_dataProvider->SetItemText(4,1,L"2text");
        m_dataProvider->SetItemValue(5,1,272);
        m_dataProvider->SetItemValue(6,1,27);
        m_list->SetVirtualDataSize(7, 2);
        m_list->InsertColumn(1, _("NAME2"));
        wxString ouputText;
        // get both columns
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 0, -1, true);
        CHECK(wxString(
            "<table border='1' style='font-family:Segoe UI; font-size:9pt; border-collapse:collapse;'>\n"
            "    <thead><tr style='background:#337BC4; color:white;'><td>NAME</td><td>NAME2</td></tr></thead>\n"
            "    <tr><td>Text</td><td>2Text</td></tr>\n"
            "    <tr><td>tExt2</td><td>2tExt2</td></tr>\n"
            "    <tr><td>text</td><td>2text</td></tr>\n"
            "    <tr><td>teXt2</td><td>2teXt2</td></tr>\n"
            "    <tr><td>text</td><td>2text</td></tr>\n"
            "    <tr><td>72</td><td>272</td></tr>\n"
            "    <tr><td>7</td><td>27</td></tr>\n"
            "</table>") == ouputText);
        // just get the first column
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 0, 0, true);
        CHECK(wxString(
            "<table border='1' style='font-family:Segoe UI; font-size:9pt; border-collapse:collapse;'>\n"
            "    <thead><tr style='background:#337BC4; color:white;'><td>NAME</td></tr></thead>\n"
            "    <tr><td>Text</td></tr>\n"
            "    <tr><td>tExt2</td></tr>\n"
            "    <tr><td>text</td></tr>\n"
            "    <tr><td>teXt2</td></tr>\n"
            "    <tr><td>text</td></tr>\n"
            "    <tr><td>72</td></tr>\n"
            "    <tr><td>7</td></tr>\n"
            "</table>") == ouputText);
        // get last column
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 1, 1, true);
        CHECK(wxString(
            "<table border='1' style='font-family:Segoe UI; font-size:9pt; border-collapse:collapse;'>\n"
            "    <thead><tr style='background:#337BC4; color:white;'><td>NAME2</td></tr></thead>\n"
            "    <tr><td>2Text</td></tr>\n"
            "    <tr><td>2tExt2</td></tr>\n"
            "    <tr><td>2text</td></tr>\n"
            "    <tr><td>2teXt2</td></tr>\n"
            "    <tr><td>2text</td></tr>\n"
            "    <tr><td>272</td></tr>\n"
            "    <tr><td>27</td></tr>\n"
            "</table>") == ouputText);
        }
    SECTION("Format to html custom column range bad")
        {
        m_dataProvider->SetSize(7,2);
        m_dataProvider->SetItemText(0,1,L"2Text");
        m_dataProvider->SetItemText(1,1,L"2tExt2");
        m_dataProvider->SetItemText(2,1,L"2text");
        m_dataProvider->SetItemText(3,1,L"2teXt2");
        m_dataProvider->SetItemText(4,1,L"2text");
        m_dataProvider->SetItemValue(5,1,272);
        m_dataProvider->SetItemValue(6,1,27);
        m_list->SetVirtualDataSize(7, 2);
        m_list->InsertColumn(1, _("NAME2"));
        wxString ouputText;
        // start bigger then end is nonsense
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 1, 0, true);
        CHECK(ouputText == wxString(wxT("")));
        // bogus negative start should be reset to first column
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, -10, 0, true);
        CHECK(wxString(
            "<table border='1' style='font-family:Segoe UI; font-size:9pt; border-collapse:collapse;'>\n"
            "    <thead><tr style='background:#337BC4; color:white;'><td>NAME</td></tr></thead>\n"
            "    <tr><td>Text</td></tr>\n"
            "    <tr><td>tExt2</td></tr>\n"
            "    <tr><td>text</td></tr>\n"
            "    <tr><td>teXt2</td></tr>\n"
            "    <tr><td>text</td></tr>\n"
            "    <tr><td>72</td></tr>\n"
            "    <tr><td>7</td></tr>\n"
            "</table>") == ouputText);
        // bogus (too large) is nonsense
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 99, 1, true);
        CHECK(ouputText == wxString(""));
        // bogus negative end should be reset to last column
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 1, -10, true);
        CHECK(wxString(
            "<table border='1' style='font-family:Segoe UI; font-size:9pt; border-collapse:collapse;'>\n"
            "    <thead><tr style='background:#337BC4; color:white;'><td>NAME2</td></tr></thead>\n"
            "    <tr><td>2Text</td></tr>\n"
            "    <tr><td>2tExt2</td></tr>\n"
            "    <tr><td>2text</td></tr>\n"
            "    <tr><td>2teXt2</td></tr>\n"
            "    <tr><td>2text</td></tr>\n"
            "    <tr><td>272</td></tr>\n"
            "    <tr><td>27</td></tr>\n"
            "</table>") == ouputText);
        // bogus (too big) end should be reset to last column
        m_list->FormatToHtml(ouputText, false, ListCtrlEx::ExportRowSelection::ExportRange, 0, -1, 1, 10, true);
        CHECK(wxString(
            "<table border='1' style='font-family:Segoe UI; font-size:9pt; border-collapse:collapse;'>\n"
            "    <thead><tr style='background:#337BC4; color:white;'><td>NAME2</td></tr></thead>\n"
            "    <tr><td>2Text</td></tr>\n"
            "    <tr><td>2tExt2</td></tr>\n"
            "    <tr><td>2text</td></tr>\n"
            "    <tr><td>2teXt2</td></tr>\n"
            "    <tr><td>2text</td></tr>\n"
            "    <tr><td>272</td></tr>\n"
            "    <tr><td>27</td></tr>\n"
            "</table>") == ouputText);
        }
    SECTION("Format to text only selected rows")
        {
        wxString ouputText;
        m_list->Select(0);
        m_list->Select(3);
        m_list->Select(6);
        m_list->FormatToText(ouputText, true);
        CHECK(wxString("NAME\nText\nteXt2\n7") == ouputText);
        }
    SECTION("Format to text no header")
        {
        wxString ouputText;
        m_list->FormatToText(ouputText, false, 0, -1, 0, -1, false);
        CHECK(wxString("Text\ntExt2\ntext\nteXt2\ntext\n72\n7") == ouputText);
        }
    SECTION("Format to text custom row range")
        {
        wxString ouputText;
        m_list->FormatToText(ouputText, false, 3, 5, 0, -1, true);
        CHECK(wxString("NAME\nteXt2\ntext\n72") == ouputText);
        }
    SECTION("Format to text custom row range bad")
        {
        wxString ouputText;
        m_list->FormatToText(ouputText, false, 99, 5, 0, -1, true);
        CHECK(ouputText == wxString(wxT("")));
        // starting point after ending point is nonsense
        m_list->FormatToText(ouputText, false, 5, 4, 0, -1, true);
        CHECK(ouputText == wxString(wxT("")));
        m_list->FormatToText(ouputText, false, 0, 99, 0, -1, true);
        CHECK(ouputText == wxString("NAME\nText\ntExt2\ntext\nteXt2\ntext\n72\n7"));
        m_list->FormatToText(ouputText, false, -10, -1, 0, -1, true);
        CHECK(ouputText == wxString("NAME\nText\ntExt2\ntext\nteXt2\ntext\n72\n7"));
        }
    SECTION("Format to text custom column range")
        {
        m_dataProvider->SetSize(7,2);
        m_dataProvider->SetItemText(0,1,L"2Text");
        m_dataProvider->SetItemText(1,1,L"2tExt2");
        m_dataProvider->SetItemText(2,1,L"2text");
        m_dataProvider->SetItemText(3,1,L"2teXt2");
        m_dataProvider->SetItemText(4,1,L"2text");
        m_dataProvider->SetItemValue(5,1,272);
        m_dataProvider->SetItemValue(6,1,27);
        m_list->SetVirtualDataSize(7, 2);
        m_list->InsertColumn(1, wxString("NAME2"));
        wxString ouputText;
        // get both columns
        m_list->FormatToText(ouputText, false, 0, -1, 0, -1, true);
        CHECK(ouputText == wxString("NAME\tNAME2\nText\t2Text\ntExt2\t2tExt2\ntext\t2text\nteXt2\t2teXt2\ntext\t2text\n72\t272\n7\t27"));
        // just get the first column
        m_list->FormatToText(ouputText, false, 0, -1, 0, 0, true);
        CHECK(ouputText == wxString("NAME\nText\ntExt2\ntext\nteXt2\ntext\n72\n7"));
        // get last column
        m_list->FormatToText(ouputText, false, 0, -1, 1, 1, true);
        CHECK(ouputText == wxString("NAME2\n2Text\n2tExt2\n2text\n2teXt2\n2text\n272\n27"));
        }
    SECTION("Format to text custom column range bad")
        {
        m_dataProvider->SetSize(7,2);
        m_dataProvider->SetItemText(0,1,L"2Text");
        m_dataProvider->SetItemText(1,1,L"2tExt2");
        m_dataProvider->SetItemText(2,1,L"2text");
        m_dataProvider->SetItemText(3,1,L"2teXt2");
        m_dataProvider->SetItemText(4,1,L"2text");
        m_dataProvider->SetItemValue(5,1,272);
        m_dataProvider->SetItemValue(6,1,27);
        m_list->SetVirtualDataSize(7, 2);
        m_list->InsertColumn(1, _("NAME2"));
        wxString ouputText;
        // start bigger then end is nonsense
        m_list->FormatToText(ouputText, false, 0, -1, 1, 0, true);
        CHECK(ouputText == wxString(wxT("")));
        // bogus negative start should be reset to first column
        m_list->FormatToText(ouputText, false, 0, -1, -10, 0, true);
        CHECK(ouputText == wxString("NAME\nText\ntExt2\ntext\nteXt2\ntext\n72\n7"));
        // bogus (too large) is nonsense
        m_list->FormatToText(ouputText, false, 0, -1, 99, 1, true);
        CHECK(ouputText == wxString(wxT("")));
        // bogus negative end should be reset to last column
        m_list->FormatToText(ouputText, false, 0, -1, 1, -10, true);
        CHECK(ouputText == wxString("NAME2\n2Text\n2tExt2\n2text\n2teXt2\n2text\n272\n27"));
        // bogus (too big) end should be reset to last column
        m_list->FormatToText(ouputText, false, 0, -1, 1, 10, true);
        CHECK(ouputText == wxString("NAME2\n2Text\n2tExt2\n2text\n2teXt2\n2text\n272\n27"));
        }
    SECTION("Set sortable range")
        {
        m_list->SetSortableRange(1, 4);
        CHECK(m_list->GetSortableRange().first == 1);
        CHECK(m_list->GetSortableRange().second == 4);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(1,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(2,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(3,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(4,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(5,0) == _("72"));
        CHECK(m_list->GetItemTextEx(6,0) == _("7"));
        m_list->SetSortableRange(0, -1);
        m_list->SortColumn(0, Wisteria::SortDirection::SortDescending);
        CHECK(m_list->GetItemTextEx(0,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(1,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(2,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(3,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(4,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(5,0) == _("72"));
        CHECK(m_list->GetItemTextEx(6,0) == _("7"));

        Reset2Columns();
        m_list->SetSortableRange(0, -1);
        std::vector<std::pair<size_t,Wisteria::SortDirection> > columns;
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortDescending));
        m_list->SortColumns(columns);
        CHECK(m_list->GetItemTextEx(0,0) == _("7"));
        CHECK(m_list->GetItemTextEx(1,0) == _("72"));
        CHECK(m_list->GetItemTextEx(2,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(3,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(4,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(5,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(6,0).CmpNoCase(_("teXt2")) == 0);

        // test multicolumn sorting
        Reset();
        m_list->SetSortableRange(1, 4);
        CHECK(m_list->GetSortableRange().first == 1);
        CHECK(m_list->GetSortableRange().second == 4);
        columns.clear();
        columns.push_back(std::pair<size_t,Wisteria::SortDirection>(0, Wisteria::SortDirection::SortAscending));
        m_list->SortColumns(columns);
        CHECK(m_list->GetItemTextEx(0,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(1,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(2,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(3,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(4,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(5,0) == _("72"));
        CHECK(m_list->GetItemTextEx(6,0) == _("7"));

        Reset();
        m_list->SetSortableRange(5,6);
        CHECK(m_list->GetSortableRange().first == 5);
        CHECK(m_list->GetSortableRange().second == 6);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(1,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(2,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(3,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(4,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(5,0) == _("7"));
        CHECK(m_list->GetItemTextEx(6,0) == _("72"));

        Reset();
        // -1 as the end of the range should make everything sortable
        m_list->SetSortableRange(0, -1);
        CHECK(m_list->GetSortableRange().first == 0);
        CHECK(m_list->GetSortableRange().second == -1);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0,0) == _("7"));
        CHECK(m_list->GetItemTextEx(1,0) == _("72"));
        CHECK(m_list->GetItemTextEx(2,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(3,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(4,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(5,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(6,0).CmpNoCase(_("teXt2")) == 0);

        Reset();
        // bogus range, should just make everything sortable
        m_list->SetSortableRange(-10, 10);
        CHECK(m_list->GetSortableRange().first == 0);
        CHECK(m_list->GetSortableRange().second == 10);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0,0) == _("7"));
        CHECK(m_list->GetItemTextEx(1,0) == _("72"));
        CHECK(m_list->GetItemTextEx(2,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(3,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(4,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(5,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(6,0).CmpNoCase(_("teXt2")) == 0);

        Reset();
        // bogus range, should just make everything sortable
        m_list->SetSortableRange(0, -10);
        CHECK(m_list->GetSortableRange().first == 0);
        CHECK(m_list->GetSortableRange().second == -1);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0,0) == _("7"));
        CHECK(m_list->GetItemTextEx(1,0) == _("72"));
        CHECK(m_list->GetItemTextEx(2,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(3,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(4,0).CmpNoCase(_("text")) == 0);
        CHECK(m_list->GetItemTextEx(5,0).CmpNoCase(_("teXt2")) == 0);
        CHECK(m_list->GetItemTextEx(6,0).CmpNoCase(_("teXt2")) == 0);

        Reset();
        // bogus range, should make nothing sortable
        m_list->SetSortableRange(10, -1);
        CHECK(m_list->GetSortableRange().first == 10);
        CHECK(m_list->GetSortableRange().second == -1);
        m_list->SortColumn(0, Wisteria::SortDirection::SortAscending);
        CHECK(m_list->GetItemTextEx(0,0) == _("Text"));
        CHECK(m_list->GetItemTextEx(1,0) == _("tExt2"));
        CHECK(m_list->GetItemTextEx(2,0) == _("text"));
        CHECK(m_list->GetItemTextEx(3,0) == _("teXt2"));
        CHECK(m_list->GetItemTextEx(4,0) == _("text"));
        CHECK(m_list->GetItemTextEx(5,0) == _("72"));
        CHECK(m_list->GetItemTextEx(6,0) == _("7"));

        Reset();
        // bogus range, should make nothing sortable
        m_list->SetSortableRange(10, -1);
        CHECK(m_list->GetSortableRange().first == 10);
        CHECK(m_list->GetSortableRange().second == -1);
        m_list->SortColumns(columns);
        CHECK(m_list->GetItemTextEx(0,0) == _("Text"));
        CHECK(m_list->GetItemTextEx(1,0) == _("tExt2"));
        CHECK(m_list->GetItemTextEx(2,0) == _("text"));
        CHECK(m_list->GetItemTextEx(3,0) == _("teXt2"));
        CHECK(m_list->GetItemTextEx(4,0) == _("text"));
        CHECK(m_list->GetItemTextEx(5,0) == _("72"));
        CHECK(m_list->GetItemTextEx(6,0) == _("7"));
        }
    SECTION("FindEx")
        {
        m_list->Select(0);
        CHECK(m_list->FindEx(_("text"),0) == 0);
        CHECK(m_list->FindEx(_("text2"),0) == 1);
        CHECK(m_list->FindEx(_("text2"),2) == 3);
        CHECK(m_list->FindEx(_("bogus")) == wxNOT_FOUND);
        }
    SECTION("Find column")
        {
        m_list->InsertColumn(1, _("Second"));
        CHECK(m_list->FindColumn(_("SeCOnd")) == 1);
        CHECK(m_list->FindColumn(_("Name")) == 0);
        CHECK(m_list->FindColumn(_("bogus")) == wxNOT_FOUND);
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
        CHECK(m_list->GetItemTextEx(0,0) == L"Text");
        CHECK(m_list->GetItemTextEx(1,0) == L"72");
        }
    SECTION("Select all")
        {
        m_list->SelectAll();
        for (long i = 0; i < m_list->GetItemCount(); ++i)
            { CHECK(m_list->IsSelected(i)); }
        }
    SECTION("Deselect all")
        {
        m_list->SelectAll();
        m_list->DeselectAll();
        for (long i = 0; i < m_list->GetItemCount(); ++i)
            { CHECK(m_list->IsSelected(i) == false); }
        }
    SECTION("Get column name")
        {
        CHECK(m_list->GetColumnName(0) == _("NAME"));
        // bogus values
        CHECK(m_list->GetColumnName(-1) == _(""));
        CHECK(m_list->GetColumnName(1) == _(""));
        }
    SECTION("Get selected text")
        {
        m_list->DeselectAll();
        m_list->Select(1);
        CHECK(m_list->GetSelectedText() == _("tExt2"));
        m_list->DeselectAll();
        m_list->Select(5);
        CHECK(m_list->GetSelectedText() == _("72"));
        }
    SECTION("GetItemTextEx")
        {
        CHECK(m_list->GetItemTextEx(0,0) == _("Text"));
        CHECK(m_list->GetItemTextEx(1,0) == _("tExt2"));
        CHECK(m_list->GetItemTextEx(2,0) == _("text"));
        CHECK(m_list->GetItemTextEx(3,0) == _("teXt2"));
        CHECK(m_list->GetItemTextEx(4,0) == _("text"));
        CHECK(m_list->GetItemTextEx(5,0) == _("72"));
        CHECK(m_list->GetItemTextEx(6,0) == _("7"));

        CHECK(m_list->GetItemTextFormatted(0,0) == _("Text"));
        CHECK(m_list->GetItemTextFormatted(1,0) == _("tExt2"));
        CHECK(m_list->GetItemTextFormatted(2,0) == _("text"));
        CHECK(m_list->GetItemTextFormatted(3,0) == _("teXt2"));
        CHECK(m_list->GetItemTextFormatted(4,0) == _("text"));
        CHECK(m_list->GetItemTextFormatted(5,0) == _("72"));
        CHECK(m_list->GetItemTextFormatted(6,0) == _("7"));
        // assertions are in place to handle out-of-boundary issues.
        // it's too slow to have boundary checks in this function.
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
    SECTION("On find up case insensitiv efull match")
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
        event.SetFlags(wxFR_WHOLEWORD|wxFR_MATCHCASE);
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
        event.SetFlags(wxFR_DOWN|wxFR_MATCHCASE);
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
        event.SetFlags(wxFR_DOWN|wxFR_WHOLEWORD);
        event.SetEventType(wxEVT_COMMAND_FIND);
        event.SetFindString(L"Text");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 2);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 4);
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 0);
        m_list->OnFind(event);
        //wrap around to the beginning
        CHECK(m_list->GetFocusedItem() == 2);
        event.SetFindString(L"7");
        m_list->OnFind(event);
        CHECK(m_list->GetFocusedItem() == 6);
        }
    SECTION("On find down case sensitive full match")
        {
        m_list->Select(0);
        wxFindDialogEvent event;
        event.SetFlags(wxFR_DOWN|wxFR_WHOLEWORD|wxFR_MATCHCASE);
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

    wxDELETE(m_dataProvider);
    }