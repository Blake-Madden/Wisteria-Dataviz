#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/util/frequencymap.h"
#include "../src/math/mathematics.h"
#include "../src/util/stringutil.h"

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

TEST_CASE("Frequency sets", "[frequencymaps]")
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
    SECTION("Aggregate frequency set")
        {
        aggregate_frequency_set<std::wstring> theSet;
        theSet.insert(L"Wasps", 2);
        theSet.insert(L"Bees", 5);
        theSet.insert(L"Wasps", 0);
        theSet.insert(L"Bees", 1);
        theSet.insert(L"Bees", 2);
        theSet.insert(L"Wasps", -8);
        theSet.insert(L"Wasps", 1);
        CHECK(2 == theSet.get_data().size());
        CHECK(theSet.get_data().find(L"Bees")->second.first == 3);
        CHECK(theSet.get_data().find(L"Bees")->second.second == 8);
        CHECK(theSet.get_data().find(L"Wasps")->second.first == 4);
        CHECK(theSet.get_data().find(L"Wasps")->second.second == -5);
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
    SECTION("Double frequency set insert")
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
    SECTION("Double frequency set insert custom increment")
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
        // custom count of 1 + the 1 already in there
        CHECK(otherSet.get_data().find(L"Bees")->second.second == 2);
        // custom increment of 1 for new item added
        CHECK(otherSet.get_data().find(L"Wasps")->second.second == 1);
        CHECK(otherSet.get_data().find(L"Bees") != otherSet.get_data().end());
        CHECK(otherSet.get_data().find(L"Wasps") != otherSet.get_data().end());
        CHECK(otherSet.get_data().find(L"Yellow jacket")->second.first == 1);
        }
    SECTION("Frequency map")
        {
        frequency_map<std::wstring, std::wstring> theMap;
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        // will fail to be added, since Bees/Honey is already in there
        theMap.insert(L"Bees", L"Wax");
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        theMap.insert(L"Wasps", L"Paper");
        CHECK(2 == theMap.get_data().size());
        CHECK(3 == theMap.get_data().find(L"Bees")->second.second);
        CHECK(theMap.get_data().find(L"Bees")->second.first == L"Honey");
        CHECK(3 == theMap.get_data().find(L"Wasps")->second.second);
        CHECK(theMap.get_data().find(L"Wasps")->second.first == L"Paper");
        }
    SECTION("Multi value aggregate map test max size")
        {
        multi_value_aggregate_map<std::wstring, std::wstring> theMap;
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        // Bees is already in there, so Bees gets incremented, Wax gets added
        theMap.insert(L"Bees", L"Wax");
        theMap.insert(L"Bees", L"Paper");
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        theMap.insert(L"Wasps", L"Paper");
        theMap.set_values_list_max_size(2);
        theMap.insert(L"Bees", L"Pollen");
        CHECK(2 == theMap.get_data().size());
        CHECK(5 == theMap.get_data().find(L"Bees")->second.second);
        CHECK(2 == theMap.get_data().find(L"Bees")->second.first.size());
        CHECK(*theMap.get_data().find(L"Bees")->second.first.begin() == std::wstring(L"Honey"));
        CHECK(*(++theMap.get_data().find(L"Bees")->second.first.begin()) == std::wstring(L"Paper"));
        CHECK(3 == theMap.get_data().find(L"Wasps")->second.second);
        CHECK(1 == theMap.get_data().find(L"Wasps")->second.first.size());
        CHECK(*theMap.get_data().find(L"Wasps")->second.first.begin() == std::wstring(L"Paper"));
        // reset, allow any number of items to be added now
        theMap.set_values_list_max_size((-1));
        theMap.insert(L"Bees", L"Pollen");
        theMap.insert(L"Bees", L"Wax");
        CHECK(7 == theMap.get_data().find(L"Bees")->second.second);
        CHECK(4 == theMap.get_data().find(L"Bees")->second.first.size());
        CHECK(*theMap.get_data().find(L"Bees")->second.first.begin() == std::wstring(L"Honey"));
        CHECK(*(++theMap.get_data().find(L"Bees")->second.first.begin()) == std::wstring(L"Paper"));
        CHECK(*(++(++theMap.get_data().find(L"Bees")->second.first.begin())) == std::wstring(L"Pollen"));
        CHECK(*(++(++(++theMap.get_data().find(L"Bees")->second.first.begin()))) == std::wstring(L"Wax"));
        }
    SECTION("Multi value aggregate map case insensitive")
        {
        multi_value_aggregate_map<std::wstring, std::wstring,
            string_util::less_basic_string_i_compare<std::wstring>,
            string_util::less_basic_string_i_compare<std::wstring>> theMap;
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        // Bees is already in there, so Bees gets incremented, Wax gets added
        theMap.insert(L"BEES", L"Wax");
        theMap.insert(L"Bees", L"HONEY");
        theMap.insert(L"WASPS", L"Paper");
        theMap.insert(L"Wasps", L"PAPER");
        CHECK(2 == theMap.get_data().size());
        CHECK(3 == theMap.get_data().find(L"Bees")->second.second);
        CHECK(2 == theMap.get_data().find(L"BEES")->second.first.size());
        CHECK(*theMap.get_data().find(L"Bees")->second.first.begin() == std::wstring(L"Honey"));
        CHECK(*(++theMap.get_data().find(L"Bees")->second.first.begin()) == std::wstring(L"Wax"));
        CHECK(3 == theMap.get_data().find(L"WASPS")->second.second);
        CHECK(1 == theMap.get_data().find(L"Wasps")->second.first.size());
        CHECK(*theMap.get_data().find(L"Wasps")->second.first.begin() == std::wstring(L"Paper"));
        }
    SECTION("Multi value aggregate map")
        {
        multi_value_aggregate_map<std::wstring, std::wstring> theMap;
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        // Bees is already in there, so Bees gets incremented, Wax gets added
        theMap.insert(L"Bees", L"Wax");
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        theMap.insert(L"Wasps", L"Paper");
        CHECK(2 == theMap.get_data().size());
        CHECK(3 == theMap.get_data().find(L"Bees")->second.second);
        CHECK(2 == theMap.get_data().find(L"Bees")->second.first.size());
        CHECK(*theMap.get_data().find(L"Bees")->second.first.begin() == std::wstring(L"Honey"));
        CHECK(*(++theMap.get_data().find(L"Bees")->second.first.begin()) == std::wstring(L"Wax"));
        CHECK(3 == theMap.get_data().find(L"Wasps")->second.second);
        CHECK(1 == theMap.get_data().find(L"Wasps")->second.first.size());
        CHECK(*theMap.get_data().find(L"Wasps")->second.first.begin() == std::wstring(L"Paper"));
        }
    SECTION("Multi value frequency & aggregate map")
        {
        multi_value_frequency_aggregate_map<std::wstring, std::wstring> theMap;
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        // Bees is already in there, so Bees gets incremented, Wax gets added
        theMap.insert(L"Bees", L"Wax");
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        theMap.insert(L"Wasps", L"Paper");
        theMap.insert(L"Wasps", L"Papyrus");
        theMap.insert(L"Wasps", L"Papyrus");
        CHECK(theMap.get_data().size() == 2);
        CHECK(theMap.get_data().find(L"Bees")->second.second == 3);
        CHECK(theMap.get_data().find(L"Bees")->second.first.get_data().size() == 2);
        // "bees" has two "honey" values
        auto beeValues = theMap.get_data().find(L"Bees")->second.first.get_data().cbegin();
        CHECK(beeValues->first == L"Honey");
        CHECK(beeValues->second == 2);
        // ...and one "wax"
        std::advance(beeValues, 1);
        CHECK(beeValues->first == L"Wax");
        CHECK(beeValues->second == 1);

        // "wasps" has three "paper" values
        auto waspsValues = theMap.get_data().find(L"Wasps")->second.first.get_data().cbegin();
        CHECK(waspsValues->first == L"Paper");
        CHECK(waspsValues->second == 3);
        // ...and two "Papyrus"
        std::advance(waspsValues, 1);
        CHECK(waspsValues->first == L"Papyrus");
        CHECK(waspsValues->second == 2);
        }
    SECTION("Multi value frequency & aggregate map case insensitive")
        {
        multi_value_frequency_aggregate_map<std::wstring, std::wstring,
                                            string_util::less_basic_string_i_compare<std::wstring>,
                                            string_util::less_basic_string_i_compare<std::wstring>> theMap;
        theMap.insert(L"Bees", L"Honey");
        theMap.insert(L"Wasps", L"Paper");
        // Bees is already in there, so Bees gets incremented, Wax gets added
        theMap.insert(L"BEES", L"Wax");
        theMap.insert(L"bees", L"HONEY");
        theMap.insert(L"WASPS", L"Paper");
        theMap.insert(L"wasps", L"PAPER");
        theMap.insert(L"WaSps", L"Papyrus");
        theMap.insert(L"waspS", L"PAPYRUS");
        CHECK(theMap.get_data().size() == 2);
        CHECK(theMap.get_data().find(L"Bees")->second.second == 3);
        CHECK(theMap.get_data().find(L"Bees")->second.first.get_data().size() == 2);
        // "bees" has two "honey" values
        auto beeValues = theMap.get_data().find(L"BEES")->second.first.get_data().cbegin();
        CHECK(beeValues->first == L"Honey");
        CHECK(beeValues->second == 2);
        // ...and one "wax"
        std::advance(beeValues, 1);
        CHECK(beeValues->first == L"Wax");
        CHECK(beeValues->second == 1);

        // "wasps" has three "paper" values
        auto waspsValues = theMap.get_data().find(L"WASPS")->second.first.get_data().cbegin();
        CHECK(waspsValues->first == L"Paper");
        CHECK(waspsValues->second == 3);
        // ...and two "Papyrus"
        std::advance(waspsValues, 1);
        CHECK(waspsValues->first == L"Papyrus");
        CHECK(waspsValues->second == 2);
        }
    }
