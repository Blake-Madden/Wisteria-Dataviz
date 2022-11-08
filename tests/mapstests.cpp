#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/util/frequencymap.h"
#include "../src/math/mathematics.h"

using namespace Catch::Matchers;

TEST_CASE("Within", "[within]")
    {
    SECTION("IsWithin")
        {
        CHECK(is_within<double>(19.1, 0, 19) == false);
        CHECK(is_within(19, 0, 19));
        CHECK(is_within(0, 0, 19));
        CHECK(is_within(12, 0, 19));
        CHECK(is_within<double>(-0.1, 0, 19) == false);

        CHECK(is_within<double>(std::make_pair(0, 19), 19.1) == false);
        CHECK(is_within(std::make_pair(0, 19), 19));
        CHECK(is_within(std::make_pair(0, 19), 0));
        CHECK(is_within(std::make_pair(0, 19), 12));
        CHECK(is_within<double>(std::make_pair(0, 19), -0.1) == false);
        }
    SECTION("Within")
        {
        within<double> wt(0, 19);
        CHECK(wt(19.1) == false);
        CHECK(wt(19));
        CHECK(wt(0));
        CHECK(wt(12));
        CHECK(wt(-0.1) == false);
        }
    SECTION("IsWithin functor")
        {
        CHECK(is_within(5.0, 1.5, 9.5));
        CHECK(is_within(1.0, 1.5, 9.5) == false);
        CHECK(is_within(9.0, 1.5, 9.5));
        CHECK(is_within(9.5, 1.5, 9.5));
        CHECK(is_within(10.0, 1.5, 9.5) == false);
        CHECK(is_within(0.0, 1.5, 9.5) == false);
        }
    SECTION("Within functor")
        {
        within<double> wt(1.5,9.5);
        CHECK(wt(5.0));
        CHECK(wt(1.0) == false);
        CHECK(wt(9.0));
        CHECK(wt(9.5));
        CHECK(wt(10.0) == false);
        CHECK(wt(0.0) == false);
        }
    }

TEST_CASE("Coalesce", "[coalesce]")
    {
    CHECK(std::wstring(L"first") == coalesce<wchar_t>({ L"first", L"second", L"third" }));
    CHECK(std::wstring(L"second") == coalesce<wchar_t>({ L"", L"second", L"third" }));
    CHECK(std::wstring(L"third") == coalesce<wchar_t>({ L"", L"", L"third" }));
    CHECK(std::wstring(L"") == coalesce<wchar_t>({ L"", L"", L"" }));
    }

TEST_CASE("Frequency Sets", "[frequencymaps]")
    {
    SECTION("Frequency set")
        {
        frequency_set<std::wstring> theSet;
        theSet.insert(L"Wasps");
        theSet.insert(L"Bees");
        theSet.insert(L"Wasps");
        theSet.insert(L"Bees");
        theSet.insert(L"Bees");
        theSet.insert(L"Wasps");
        theSet.insert(L"Wasps");
        CHECK(2 == theSet.get_data().size());
        CHECK(theSet.get_data().find(L"Bees")->second == 3);
        CHECK(theSet.get_data().find(L"Wasps")->second == 4);
        }
    SECTION("Double frequency set")
        {
        double_frequency_set<std::wstring> theSet;
        theSet.insert(L"Wasps", false);
        theSet.insert(L"Bees", true);
        theSet.insert(L"Wasps", true);
        theSet.insert(L"Bees", true);
        theSet.insert(L"Bees", false);
        theSet.insert(L"Wasps", true);
        theSet.insert(L"Wasps", true);
        CHECK(2 == theSet.get_data().size());
        CHECK(theSet.get_data().find(L"Bees")->second.first == 3);
        CHECK(theSet.get_data().find(L"Wasps")->second.first == 4);
        CHECK(theSet.get_data().find(L"Bees")->second.second == 2);
        CHECK(theSet.get_data().find(L"Wasps")->second.second == 3);
        CHECK(theSet.get_data().find(L"Bees") != theSet.get_data().end());
        CHECK(theSet.get_data().find(L"Wasps") != theSet.get_data().end());
        CHECK(theSet.get_data().find(L"Yellow jacket") == theSet.get_data().end());
        }
    SECTION("Double Frequency Set Insert")
        {
        double_frequency_set<std::wstring> theSet;
        theSet.insert(L"Wasps", false);
        theSet.insert(L"Bees", true);
        theSet.insert(L"Wasps", true);
        theSet.insert(L"Bees", true);
        theSet.insert(L"Bees", false);
        theSet.insert(L"Wasps", true);
        theSet.insert(L"Wasps", true);
        CHECK(2 == theSet.get_data().size());
        CHECK(theSet.get_data().find(L"Bees")->second.first == 3);
        CHECK(theSet.get_data().find(L"Wasps")->second.first == 4);
        CHECK(theSet.get_data().find(L"Bees")->second.second == 2);
        CHECK(theSet.get_data().find(L"Wasps")->second.second == 3);
        CHECK(theSet.get_data().find(L"Bees") != theSet.get_data().end());
        CHECK(theSet.get_data().find(L"Wasps") != theSet.get_data().end());
        CHECK(theSet.get_data().find(L"Yellow jacket") == theSet.get_data().end());

        double_frequency_set<std::wstring> otherSet;
        otherSet.insert(L"Yellow jacket", false);
        otherSet.insert(L"Bees", true);
        otherSet.insert(L"Bees", false);
        otherSet.insert(L"Bees", true);
        otherSet += theSet;
        CHECK(6 == otherSet.get_data().find(L"Bees")->second.first);
        CHECK(otherSet.get_data().find(L"Wasps")->second.first == 4);
        CHECK(otherSet.get_data().find(L"Bees")->second.second == 4);
        CHECK(otherSet.get_data().find(L"Wasps")->second.second == 3);
        CHECK(otherSet.get_data().find(L"Bees") != otherSet.get_data().end());
        CHECK(otherSet.get_data().find(L"Wasps") != otherSet.get_data().end());
        CHECK(otherSet.get_data().find(L"Yellow jacket")->second.first == 1);
        }
    SECTION("DoubleFrequencySetInsertCustomIncrement")
        {
        double_frequency_set<std::wstring> theSet;
        theSet.insert(L"Wasps", true);
        theSet.insert(L"Bees", true);
        theSet.insert(L"Wasps", true);
        theSet.insert(L"Bees", true);
        theSet.insert(L"Bees", true);
        theSet.insert(L"Wasps", true);
        theSet.insert(L"Wasps", true);

        double_frequency_set<std::wstring> otherSet;
        otherSet.insert(L"Yellow jacket", false);
        otherSet.insert(L"Bees", true);
        otherSet.insert(L"Bees", false);
        otherSet.insert(L"Bees", false);
        otherSet.insert_with_custom_increment(theSet);
        CHECK(6 == otherSet.get_data().find(L"Bees")->second.first);
        CHECK(otherSet.get_data().find(L"Wasps")->second.first == 4);
        CHECK(otherSet.get_data().find(L"Bees")->second.second == 2);//custom count of 1 + the 1 already in there
        CHECK(otherSet.get_data().find(L"Wasps")->second.second == 1);//custom increment of 1 for new item added
        CHECK(otherSet.get_data().find(L"Bees") != otherSet.get_data().end());
        CHECK(otherSet.get_data().find(L"Wasps") != otherSet.get_data().end());
        CHECK(otherSet.get_data().find(L"Yellow jacket")->second.first == 1);
        }
    SECTION("Frequency map")
        {
        frequency_map<std::wstring, std::wstring> theMap;
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        theMap.insert(L"Bees", L"Wax");//will fail to be added, since Bees/Honey is already in there
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        theMap.insert(L"Wasps", L"Paper");
        CHECK(2 == theMap.get_data().size());
        CHECK(3 == theMap.get_data().find(L"Bees")->second.second);
        CHECK(theMap.get_data().find(L"Bees")->second.first == L"Honey");
        CHECK(3 == theMap.get_data().find(L"Wasps")->second.second);
        CHECK(theMap.get_data().find(L"Wasps")->second.first == L"Paper");
        }
    SECTION("Multi Frequency Map Test Max Size")
        {
        multi_value_aggregate_map<std::wstring, std::wstring> theMap;
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        theMap.insert(L"Bees", L"Wax");//Bees is already in there, so Bees gets incremented, Wax gets added
        theMap.insert(L"Bees", L"Paper");
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        theMap.insert(L"Wasps", L"Paper");
        theMap.set_values_list_max_size(2);
        theMap.insert(L"Bees", L"Pollen");
        CHECK((2) == theMap.get_data().size());
        CHECK((5) == theMap.get_data().find(L"Bees")->second.second);
        CHECK((2) == theMap.get_data().find(L"Bees")->second.first.size());
        CHECK(*theMap.get_data().find(L"Bees")->second.first.begin() == std::wstring(L"Honey"));
        CHECK(*(++theMap.get_data().find(L"Bees")->second.first.begin()) == std::wstring(L"Paper"));
        CHECK((3) == theMap.get_data().find(L"Wasps")->second.second);
        CHECK((1) == theMap.get_data().find(L"Wasps")->second.first.size());
        CHECK(*theMap.get_data().find(L"Wasps")->second.first.begin() == std::wstring(L"Paper"));
        //reset, allow any number of items to be added now
        theMap.set_values_list_max_size((-1));
        theMap.insert(L"Bees", L"Pollen");
        theMap.insert(L"Bees", L"Wax");
        CHECK((7) == theMap.get_data().find(L"Bees")->second.second);
        CHECK((4) == theMap.get_data().find(L"Bees")->second.first.size());
        CHECK(*theMap.get_data().find(L"Bees")->second.first.begin() == std::wstring(L"Honey"));
        CHECK(*(++theMap.get_data().find(L"Bees")->second.first.begin()) == std::wstring(L"Paper"));
        CHECK(*(++(++theMap.get_data().find(L"Bees")->second.first.begin())) == std::wstring(L"Pollen"));
        CHECK(*(++(++(++theMap.get_data().find(L"Bees")->second.first.begin()))) == std::wstring(L"Wax"));
        }
    SECTION("Multi Frequency Map")
        {
        multi_value_aggregate_map<std::wstring, std::wstring> theMap;
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        theMap.insert(L"Bees", L"Wax");//Bees is already in there, so Bees gets incremented, Wax gets added
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        theMap.insert(L"Wasps", L"Paper");
        CHECK((2) == theMap.get_data().size());
        CHECK((3) == theMap.get_data().find(L"Bees")->second.second);
        CHECK((2) == theMap.get_data().find(L"Bees")->second.first.size());
        CHECK(*theMap.get_data().find(L"Bees")->second.first.begin() == std::wstring(L"Honey"));
        CHECK(*(++theMap.get_data().find(L"Bees")->second.first.begin()) == std::wstring(L"Wax"));
        CHECK((3) == theMap.get_data().find(L"Wasps")->second.second);
        CHECK((1) == theMap.get_data().find(L"Wasps")->second.first.size());
        CHECK(*theMap.get_data().find(L"Wasps")->second.first.begin() == std::wstring(L"Paper"));
        }
    }