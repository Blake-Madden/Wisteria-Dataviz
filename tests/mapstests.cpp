// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/util/frequencymap.h"
#include "../src/math/mathematics.h"
#include "../src/util/string_util.h"

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
        // just force temp construction
        theSet.insert(std::wstring{ L"Bees"} );
        theSet.insert(std::wstring{ L"Wasps" });
        theSet.insert(std::wstring{ L"Wasps" });
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
        // override SSO, test move semantics with perfect forwarding
        theSet.insert(std::wstring{ L"Wasps with paper nests" }, false);
        theSet.insert(L"Bees", true);
        theSet.insert(L"Wasps with paper nests", true);
        theSet.insert(L"Bees", true);
        theSet.insert(L"Bees", false);
        theSet.insert(L"Wasps with paper nests", true);
        theSet.insert(L"Wasps with paper nests", true);
        CHECK(2 == theSet.get_data().size());
        CHECK(theSet.get_data().find(L"Bees")->second.first == 3);
        CHECK(theSet.get_data().find(L"Wasps with paper nests")->second.first == 4);
        CHECK(theSet.get_data().find(L"Bees")->second.second == 2);
        CHECK(theSet.get_data().find(L"Wasps with paper nests")->second.second == 3);
        CHECK(theSet.get_data().find(L"Bees") != theSet.get_data().end());
        CHECK(theSet.get_data().find(L"Wasps with paper nests") != theSet.get_data().end());
        CHECK(theSet.get_data().find(L"Yellow jacket") == theSet.get_data().end());

        double_frequency_set<std::wstring> otherSet;
        otherSet.insert(L"Yellow jacket", false);
        otherSet.insert(L"Bees", true);
        otherSet.insert(L"Bees", false);
        otherSet.insert(L"Bees", true);
        otherSet += theSet;
        CHECK(6 == otherSet.get_data().find(L"Bees")->second.first);
        CHECK(otherSet.get_data().find(L"Wasps with paper nests")->second.first == 4);
        CHECK(otherSet.get_data().find(L"Bees")->second.second == 4);
        CHECK(otherSet.get_data().find(L"Wasps with paper nests")->second.second == 3);
        CHECK(otherSet.get_data().find(L"Bees") != otherSet.get_data().end());
        CHECK(otherSet.get_data().find(L"Wasps with paper nests") != otherSet.get_data().end());
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
    SECTION("Multi value frequency double aggregate map")
        {
        // key aggregate is the pair.second; subvalue counts/aggregates are in the values_set
        multi_value_frequency_double_aggregate_map<std::wstring, std::wstring> theMap;

        // Bees
        theMap.insert(L"Bees", L"Honey", /*key agg*/ 2, /*sub agg*/ 5);   // Bees:2 | Honey:(1,5)
        theMap.insert(L"Bees", L"Wax",   /*key agg*/ 1, /*sub agg*/ 2);   // Bees:3 | Wax:(1,2)
        theMap.insert(L"Bees", L"Honey", /*key agg*/ 3, /*sub agg*/ 7);   // Bees:6 | Honey:(2,12)

        // Wasps
        theMap.insert(L"Wasps", L"Paper",   /*key agg*/ 1, /*sub agg*/ 3); // Wasps:1 | Paper:(1,3)
        theMap.insert(L"Wasps", L"Papyrus", /*key agg*/ 4, /*sub agg*/10); // Wasps:5 | Papyrus:(1,10)
        theMap.insert(L"Wasps", L"Paper",   /*key agg*/ 2, /*sub agg*/ 1); // Wasps:7 | Paper:(2,4)

        CHECK(theMap.get_data().size() == 2);

        // Bees totals
        {
        const auto it = theMap.get_data().find(L"Bees");
        REQUIRE(it != theMap.get_data().cend());
        // key aggregate
        CHECK(it->second.second == 6);
        // subitems
        const auto& subs = it->second.first.get_data();
        REQUIRE(subs.size() == 2);

        auto sub = subs.find(L"Honey");
        REQUIRE(sub != subs.cend());
        CHECK(sub->second.first  == 2);   // count
        CHECK(sub->second.second == 12);  // aggregate 5 + 7

        sub = subs.find(L"Wax");
        REQUIRE(sub != subs.cend());
        CHECK(sub->second.first  == 1);
        CHECK(sub->second.second == 2);
        }

        // Wasps totals
        {
        const auto it = theMap.get_data().find(L"Wasps");
        REQUIRE(it != theMap.get_data().cend());
        CHECK(it->second.second == 7);

        const auto& subs = it->second.first.get_data();
        REQUIRE(subs.size() == 2);

        auto sub = subs.find(L"Paper");
        REQUIRE(sub != subs.cend());
        CHECK(sub->second.first  == 2);
        CHECK(sub->second.second == 4);   // 3 + 1

        sub = subs.find(L"Papyrus");
        REQUIRE(sub != subs.cend());
        CHECK(sub->second.first  == 1);
        CHECK(sub->second.second == 10);
        }

        // erase returns next iterator
        {
        auto it = theMap.get_data().cbegin();
        const std::wstring erasedKey = it->first;
        auto next = theMap.erase(it);
        CHECK(theMap.get_data().size() == 1);
        // 'next' is valid and should now be begin()
        CHECK((next == theMap.get_data().begin()));
        CHECK(theMap.get_data().find(erasedKey) == theMap.get_data().cend());
        }
        }

    SECTION("Multi value frequency double aggregate map (rvalue overload + clear)")
        {
        multi_value_frequency_double_aggregate_map<std::wstring, std::wstring> theMap;

        // rvalue insertions
        std::wstring k1 = L"Birds", v1 = L"Seeds";
        theMap.insert(std::move(k1), std::move(v1), 1, 2);
        CHECK(theMap.get_data().size() == 1);
        {
        const auto it = theMap.get_data().find(L"Birds");
        REQUIRE(it != theMap.get_data().cend());
        CHECK(it->second.second == 1);
        const auto& subs = it->second.first.get_data();
        REQUIRE(subs.size() == 1);
        CHECK(subs.begin()->first == L"Seeds");
        CHECK(subs.begin()->second.first == 1);
        CHECK(subs.begin()->second.second == 2);
        }

        theMap.clear();
        CHECK(theMap.get_data().empty());
        }

    SECTION("Multi value frequency double aggregate map (case-insensitive)")
        {
        using CI = string_util::less_basic_string_i_compare<std::wstring>;
        multi_value_frequency_double_aggregate_map<std::wstring, std::wstring, CI, CI> theMap;

        theMap.insert(L"Bees",  L"Honey", 2, 5);
        theMap.insert(L"BEES",  L"WAX",   1, 2);
        theMap.insert(L"bees",  L"honey", 3, 7);

        theMap.insert(L"WASPS", L"Paper",   1, 3);
        theMap.insert(L"wasps", L"papyrus", 4,10);
        theMap.insert(L"Wasps", L"PAPER",   2, 1);

        CHECK(theMap.get_data().size() == 2);

        // Bees case-insensitive merge
        {
        const auto it = theMap.get_data().find(L"bees");
        REQUIRE(it != theMap.get_data().cend());
        CHECK(it->second.second == 6);
        const auto& subs = it->second.first.get_data();
        REQUIRE(subs.size() == 2);

        auto sub = subs.find(L"honey");
        REQUIRE(sub != subs.cend());
        CHECK(sub->second.first  == 2);
        CHECK(sub->second.second == 12);

        sub = subs.find(L"wax");
        REQUIRE(sub != subs.cend());
        CHECK(sub->second.first  == 1);
        CHECK(sub->second.second == 2);
        }

        // Wasps case-insensitive merge
        {
        const auto it = theMap.get_data().find(L"wasps");
        REQUIRE(it != theMap.get_data().cend());
        CHECK(it->second.second == 7);
        const auto& subs = it->second.first.get_data();
        REQUIRE(subs.size() == 2);

        auto sub = subs.find(L"paper");
        REQUIRE(sub != subs.cend());
        CHECK(sub->second.first  == 2);
        CHECK(sub->second.second == 4);

        sub = subs.find(L"papyrus");
        REQUIRE(sub != subs.cend());
        CHECK(sub->second.first  == 1);
        CHECK(sub->second.second == 10);
        }
        }
    }

// NOLINTEND
// clang-format on
