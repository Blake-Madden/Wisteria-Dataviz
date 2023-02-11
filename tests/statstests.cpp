#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/statistics.h"

using namespace Catch::Matchers;

TEST_CASE("Valid N", "[stats][validN]")
    {
    CHECK(statistics::valid_n({}) == 0);
    CHECK(statistics::valid_n(std::vector<double>{ std::numeric_limits<double>::quiet_NaN() }) == 0);
    CHECK(statistics::valid_n(std::vector<double>{
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN() }) == 0);
    CHECK(statistics::valid_n(std::vector<double>{
        std::numeric_limits<double>::quiet_NaN(),
        1.0,
        std::numeric_limits<double>::quiet_NaN() }) == 1);
    CHECK(statistics::valid_n(std::vector<double>{ -7.8, 1.0, 5.1 }) == 3);
    }

TEST_CASE("Normalize", "[stats][normalize]")
    {
    CHECK_THAT(statistics::normalize(1,50,1), WithinRel(0, 1e-4));
    CHECK_THAT(statistics::normalize(1,50,50), WithinRel(1, 1e-4));
    CHECK_THAT(statistics::normalize<double>(1,50,25.5), WithinRel(0.5, 1e-4));
    CHECK_THAT(statistics::normalize(-50,50,0), WithinRel(0.5, 1e-4));
    CHECK_THAT(statistics::normalize(-50,50,50), WithinRel(1, 1e-4));
    CHECK_THAT(statistics::normalize(-50,50,-50), WithinRel(0, 1e-4));
    }

TEST_CASE("Normalize excepctions", "[stats][normalize]")
    {
    double noWarn;
    CHECK_THROWS(noWarn = statistics::normalize(1,50,51)); // value out of range
    CHECK_THROWS(noWarn = statistics::normalize(51,50,50)); // bad range
    }

TEST_CASE("Mode", "[stats][mode]")
    {
    std::vector<double> values = { 5, 9, 6, 7, 6, 4, 3, -3, 17, 6 };
    std::set<double> modes = statistics::mode(values);
    CHECK(1 == modes.size());
    CHECK(6 == *modes.begin());
    }

TEST_CASE("Mode multiples", "[stats][mode]")
    {
    std::vector<double> values2Modes = { 7, 6, 5, 6, 7, 5, 5, 7, 3, -3, 17, 6 };
    std::set<double> modes = statistics::mode(values2Modes);
    auto pos = modes.begin();
    CHECK(3 == modes.size());
    CHECK(5 == *pos);
    CHECK(6 == *(++pos));
    CHECK(7 == *(++pos));
    }

TEST_CASE("Mode empty container", "[stats][mode]")
    {
    std::vector<double> values;
    std::set<double> modes = statistics::mode<double>(values);
    CHECK(0 == modes.size());
    }

TEST_CASE("Mode container", "[stats][mode]")
    {
    std::vector<double> values;
    values.push_back(5);
    values.push_back(9);
    values.push_back(6);
    values.push_back(7);
    values.push_back(6);
    values.push_back(4);
    values.push_back(3);
    values.push_back(-3);
    values.push_back(17);
    values.push_back(6);
    std::set<double> modes = statistics::mode<double>(values);
    CHECK(1 == modes.size());
    CHECK(6 == *modes.begin());
    }

TEST_CASE("Mode doubles", "[stats][mode]")
    {
    std::vector<double> values = { 5.2, 5.2, 6.52, 7.1, 6.0, 4.9, 3.1, -3.13, 17.958, 6.955 };
    std::set<double> modes = statistics::mode(values);
    CHECK(1 == modes.size());
    CHECK_THAT(*modes.begin(), WithinRel(5.2, 1e-4));
    }

TEST_CASE("Mode floor", "[stats][mode]")
    {
    std::vector<double> values = { 5.2, 5.2, 6.52, 7.1, 6.0, 4.9, 3.1, -3.13, 17.958, 6.955 };
    std::set<double> modes = statistics::mode<double>(values, floor_value<double>());
    CHECK(1 == modes.size());
    CHECK_THAT((6), WithinRel(*modes.begin(), 1e-4));

    std::vector<double> values2Modes = { 5.2, 5.2, 6.52, 5.3, 6.0, 4.9, 3.1, -3.13, 17.958, 6.955 };
    modes = statistics::mode<double>(values2Modes, floor_value<double>());
    CHECK(2 == modes.size());
    CHECK_THAT((5), WithinRel(*modes.begin(), 1e-4));
    CHECK_THAT((6), WithinRel(*(++modes.begin()), 1e-4));
    }

TEST_CASE("Mode one mode", "[stats][mode]")
    {
    std::vector<double> values = { 5, 5 };
    std::set<double> modes = statistics::mode<double>(values);
    CHECK(1 == modes.size());
    CHECK(5 == *modes.begin());
    }

TEST_CASE("Mode one value", "[stats][mode]")
    {
    std::vector<double> values = { 5 };
    std::set<double> modes = statistics::mode(values);
    CHECK(1 == modes.size());
    CHECK(5 == *modes.begin());
    }

TEST_CASE("Mode all mode", "[stats][mode]")
    {
    std::vector<double> values = { 5, 5, 9, 9 };
    std::set<double> modes = statistics::mode(values);
    CHECK(2 == modes.size());
    CHECK(5 == *modes.begin());
    CHECK(9 == *(++modes.begin()));
    }

TEST_CASE("Mode strings", "[stats][mode]")
    {
    std::vector<std::wstring> values = { L"sprite", L"coke", L"pepsi", L"coke", L"sasta" };
    std::set<std::wstring> modes = statistics::mode<std::wstring>(values);
    CHECK((1) == modes.size());
    CHECK(*modes.begin() == L"coke");
    }

TEST_CASE("Variance container", "[stats][variance]")
    {
    std::vector<double> values;
    values.push_back(5.2);
    values.push_back(67.0);
    values.push_back(0.0);
    values.push_back(-1.1);
    values.push_back(555.684);
    CHECK_THAT(58682.580331, WithinRel(statistics::variance(values, true), 1e-6));
    values.clear();
    values.push_back(0.0);
    values.push_back(0.0);
    values.push_back(0.0);
    values.push_back(0.0);
    CHECK_THAT(0.000000, WithinRel(statistics::variance(values, true), 1e-6));
    values.clear();
    values.push_back(-500142.2541);
    values.push_back(-5974.25681);
    values.push_back(-84689.26547);
    values.push_back(-579954.26578);
    CHECK_THAT(83675806361.102203, WithinRel(statistics::variance(values, true), 1e-4));
    values.clear();
    values.push_back(0.266448615);
    values.push_back(0.11703829);
    values.push_back(0.665102469);
    values.push_back(0.633862468);
    values.push_back(0.509262405);
    values.push_back(0.371353823);
    values.push_back(0.494912922);
    values.push_back(0.608961596);
    CHECK_THAT(0.037293, WithinRel(statistics::variance(values, true), 1e-5));

    values.clear();
    values.reserve(1'000'000);
    for (size_t i = 0; i < 1'000'000; ++i)
        { values.push_back(57855.56894 + i); }
    CHECK_THAT(83333416666.67, WithinRel(statistics::variance(values, true), 0.2));
    }

TEST_CASE("Variance array", "[stats][variance]")
    {
    std::vector<double> values = { 5.2, 67.0, 0.0, -1.1, 555.684 };
    CHECK_THAT(58682.580331, WithinRel(statistics::variance(values, true), 1e-6));
    values = { 0.0, 0.0, 0.0, 0.0 };
    CHECK_THAT(0.000000, WithinRel(statistics::variance(values, true), 1e-6));
    values = { -500142.2541, -5974.25681, -84689.26547, -579954.26578 };
    CHECK_THAT(83675806361.102203, WithinRel(statistics::variance(values, true), 1e-4));
    values = { 0.266448615, 0.11703829, 0.665102469, 0.633862468, 0.509262405, 0.371353823, 0.494912922, 0.608961596 };
    CHECK_THAT(0.037293, WithinRel(statistics::variance(values, true), 1e-5));

    values.clear();
    values.reserve(1'000'000);
    for (size_t i = 0; i < 1'000'000; ++i)
        { values.push_back(57855.56894 + i); }
    // there is some dispute between other stat packages about the hundredths place value, so don't compare
    // beyond that. Note that we are going with what SPSS reports (they get the same as us up to the hundreths place)
    CHECK_THAT(83333416666.67, WithinRel(statistics::variance(values, true), 0.2));
    }

TEST_CASE("Variance one obs.", "[stats][variance]")
    {
    std::vector<double> values = { 5.2 };
    CHECK_THROWS(statistics::variance(values, true));
    }

TEST_CASE("Variance no obs.", "[stats][variance]")
    {
    std::vector<double> values;
    CHECK_THROWS(statistics::variance(values, true));
    }

TEST_CASE("Mean array", "[stats][mean]")
    {
    std::vector<double> values = { 5.2, 67.0, 0.0, -1.1, 555.684 };
    CHECK_THAT(125.3568, WithinRel(statistics::mean(values), 1e-6));
    values = { 5.2 };
    CHECK_THAT(5.200000, WithinRel(statistics::mean(values), 1e-6));
    values = { 0.0, 0.0, 0.0, 0.0 };
    CHECK_THAT(0.000000, WithinRel(statistics::mean(values), 1e-6));
    values = { -500142.2541, -5974.25681, -84689.26547, -579954.26578 };
    CHECK_THAT(-292690.01054, WithinRel(statistics::mean(values), 1e-6));
    values = { 0.266448615, 0.11703829, 0.665102469, 0.633862468, 0.509262405, 0.371353823, 0.494912922, 0.608961596 };
    CHECK_THAT(0.458367824, WithinRel(statistics::mean(values), 1e-6));

    values.clear();
    for (size_t i = 0; i < 1000000; ++i)
        { values.push_back(57855.56894 + i); }
    CHECK_THAT(557855.06894, WithinRel(statistics::mean(values), 1e-4));
    }

TEST_CASE("Mean container", "[stats][mean]")
    {
    std::vector<double> values;
    values.push_back(5.2);
    values.push_back(67.0);
    values.push_back(0.0);
    values.push_back(-1.1);
    values.push_back(555.684);
    CHECK_THAT(125.3568, WithinRel(statistics::mean(values), 1e-6));
    values.clear();
    values.push_back(5.2);
    CHECK_THAT(5.200000, WithinRel(statistics::mean(values), 1e-6));
    values.clear();
    values.push_back(0.0);
    values.push_back(0.0);
    values.push_back(0.0);
    values.push_back(0.0);
    CHECK_THAT(0.000000, WithinRel(statistics::mean(values), 1e-6));
    values.clear();
    values.push_back(-500142.2541);
    values.push_back(-5974.25681);
    values.push_back(-84689.26547);
    values.push_back(-579954.26578);
    CHECK_THAT(-292690.01054, WithinRel(statistics::mean(values), 1e-6));
    values.clear();
    values.push_back(0.266448615);
    values.push_back(0.11703829);
    values.push_back(0.665102469);
    values.push_back(0.633862468);
    values.push_back(0.509262405);
    values.push_back(0.371353823);
    values.push_back(0.494912922);
    values.push_back(0.608961596);
    CHECK_THAT(0.458367824, WithinRel(statistics::mean(values), 1e-6));

    values.clear();
    for (size_t i = 0; i < 1000000; ++i)
        { values.push_back(57855.56894 + i); }
    CHECK_THAT(557855.06894, WithinRel(statistics::mean(values), 1e-4));
    }

TEST_CASE("Mean no obs.", "[stats][mean]")
    {
    std::vector<double> values;
    CHECK_THROWS(statistics::mean(values));
    }

TEST_CASE("Median array", "[stats][median]")
    {
    std::vector<double> values = { 5.2, 67.0, 0.0, -1.1, 555.684 };
    CHECK_THAT(5.2, WithinRel(statistics::median(values), 1e-6));
    values = { 5.2, 67.0, -1.1, 555.684 };
    CHECK_THAT(36.1, WithinRel(statistics::median(values), 1e-6));
    values  = { 5.2 };
    CHECK_THAT(5.200000, WithinRel(statistics::median(values), 1e-6));
    values = { 0.0, 0.0, 0.0, 0.0 };
    CHECK_THAT(0.000000, WithinRel(statistics::median(values), 1e-6));
    values = { -500142.2541, -5974.25681, -84689.26547, -579954.26578 };
    CHECK_THAT(-292415.759785, WithinRel(statistics::median(values), 1e-6));
    values = { 0.266448615, 0.11703829, 0.665102469, 0.633862468, 0.509262405, 0.371353823, 0.494912922, 0.608961596 };
    CHECK_THAT(0.502087664, WithinRel(statistics::median(values), 1e-6));

    values.clear();
    values.reserve(1'000'000);
    for (size_t i = 0; i < 1'000'000; ++i)
        { values.push_back(57855.56894 + i); }
    CHECK_THAT(557855.068940, WithinRel(statistics::median(values), 1e-6));
    }

TEST_CASE("Median container", "[stats][median]")
    {
    std::vector<double> values;
    values.push_back(5.2);
    values.push_back(67.0);
    values.push_back(0.0);
    values.push_back(-1.1);
    values.push_back(555.684);
    CHECK_THAT(5.2, WithinRel(statistics::median(values), 1e-6));
    values.clear();
    values.push_back(5.2);
    values.push_back(67.0);
    values.push_back(-1.1);
    values.push_back(555.684);
    CHECK_THAT(36.1, WithinRel(statistics::median(values), 1e-6));
    values.clear();
    values.push_back(5.2);
    CHECK_THAT(5.200000, WithinRel(statistics::median(values), 1e-6));
    values.clear();
    values.push_back(0.0);
    values.push_back(0.0);
    values.push_back(0.0);
    values.push_back(0.0);
    CHECK_THAT(0.000000, WithinRel(statistics::median(values), 1e-6));
    values.clear();
    values.push_back(-500142.2541);
    values.push_back(-5974.25681);
    values.push_back(-84689.26547);
    values.push_back(-579954.26578);
    CHECK_THAT(-292415.759785, WithinRel(statistics::median(values), 1e-6));
    values.clear();
    values.push_back(0.266448615);
    values.push_back(0.11703829);
    values.push_back(0.665102469);
    values.push_back(0.633862468);
    values.push_back(0.509262405);
    values.push_back(0.371353823);
    values.push_back(0.494912922);
    values.push_back(0.608961596);
    CHECK_THAT(0.502087664, WithinRel(statistics::median(values), 1e-6));

    values.clear();
    for (size_t i = 0; i < 1000000; ++i)
        { values.push_back(57855.56894 + i); }
    CHECK_THAT(557855.068940, WithinRel(statistics::median(values), 1e-6));
    }

TEST_CASE("Median no obs.", "[stats][median]")
    {
    std::vector<double> values;
    CHECK_THROWS(statistics::median(values));
    }

TEST_CASE("Percentiles Array", "[stats][percentiles]")
    {
    double lower_percentile_value, upper_percentile_value;
    std::vector<double> values = { -1.1, 0.0, 5.2, 67.0, 555.684 };
    statistics::quartiles_presorted(values, lower_percentile_value, upper_percentile_value);
    CHECK_THAT(0.0, WithinRel(lower_percentile_value, 1e-6));
    CHECK_THAT(67., WithinRel(upper_percentile_value, 1e-6));

    std::vector<double> zeroValues = { 0.0, 0.0, 0.0, 0.0 };
    statistics::quartiles_presorted(zeroValues, lower_percentile_value, upper_percentile_value);
    CHECK_THAT(0., WithinRel(lower_percentile_value, 1e-6));
    CHECK_THAT(0., WithinRel(upper_percentile_value, 1e-6));

    std::vector<double> negativeValues = { -579954.26578, -500142.2541, -84689.26547, -5974.25681 };
    statistics::quartiles_presorted(negativeValues, lower_percentile_value, upper_percentile_value);
    CHECK_THAT(-540048.25994, WithinRel(lower_percentile_value, 1e-6));
    CHECK_THAT(-45331.76114, WithinRel(upper_percentile_value, 1e-6));

    std::vector<double> floatValues = { 0.11703829, 0.266448615, 0.371353823, 0.494912922, 0.509262405, 0.608961596, 0.633862468, 0.665102469 };
    statistics::quartiles_presorted(floatValues, lower_percentile_value, upper_percentile_value);
    CHECK_THAT(0.318901, WithinRel(lower_percentile_value, 1e-6));
    CHECK_THAT(0.621412, WithinRel(upper_percentile_value, 1e-6));
    }

TEST_CASE("Percentiles Container", "[stats][percentiles]")
    {
    double lower_percentile_value, upper_percentile_value;
    std::vector<double> values;
    values.push_back(-1.1);
    values.push_back(0.0);
    values.push_back(5.2);
    values.push_back(67.0);
    values.push_back(555.684);
    statistics::quartiles_presorted(values, lower_percentile_value, upper_percentile_value);
    CHECK_THAT(0., WithinRel(lower_percentile_value, 1e-6));
    CHECK_THAT(67., WithinRel(upper_percentile_value, 1e-6));

    values.clear();
    values.push_back(0.0);
    values.push_back(0.0);
    values.push_back(0.0);
    values.push_back(0.0);
    statistics::quartiles_presorted(values, lower_percentile_value, upper_percentile_value);
    CHECK_THAT(0., WithinRel(lower_percentile_value, 1e-6));
    CHECK_THAT(0., WithinRel(upper_percentile_value, 1e-6));

    values.clear();
    values.push_back(-579954.26578);
    values.push_back(-500142.2541);
    values.push_back(-84689.26547);
    values.push_back(-5974.25681);
    statistics::quartiles_presorted(values, lower_percentile_value, upper_percentile_value);
    CHECK_THAT(-540048.25994, WithinRel(lower_percentile_value, 1e-6));
    CHECK_THAT(-45331.76114, WithinRel(upper_percentile_value, 1e-6));

    values.clear();
    values.push_back(0.11703829);
    values.push_back(0.266448615);
    values.push_back(0.371353823);
    values.push_back(0.494912922);
    values.push_back(0.509262405);
    values.push_back(0.608961596);
    values.push_back(0.633862468);
    values.push_back(0.665102469);
    statistics::quartiles_presorted(values, lower_percentile_value, upper_percentile_value);
    CHECK_THAT(0.318901, WithinRel(lower_percentile_value, 1e-6));
    CHECK_THAT(0.621412, WithinRel(upper_percentile_value, 1e-6));

    values.clear();
    values.reserve(1'000'000);
    for (size_t i = 0; i < 1'000'000; ++i)
        { values.push_back(57'855.56894 + i); }
    statistics::quartiles_presorted(values, lower_percentile_value, upper_percentile_value);
    CHECK_THAT(307855.06894, WithinRel(lower_percentile_value, 1e-6));
    CHECK_THAT(807855.06894, WithinRel(upper_percentile_value, 1e-6));
    }

TEST_CASE("Percentiles One Obs", "[stats][percentiles]")
    {
    double lower_percentile_value, upper_percentile_value;
    std::vector<double> singleValues = { 5.2 };
    statistics::quartiles_presorted(singleValues, lower_percentile_value, upper_percentile_value);
    CHECK_THAT(5.2, WithinRel(lower_percentile_value, 1e-6));
    CHECK_THAT(5.2, WithinRel(upper_percentile_value, 1e-6));
    }

TEST_CASE("Percentiles No Obs", "[stats][percentiles]")
    {
    double lower_percentile_value, upper_percentile_value;
    std::vector<double> noValues;
    CHECK_THROWS(statistics::quartiles_presorted(noValues, lower_percentile_value, upper_percentile_value));
    }

TEST_CASE("Std Dev Sampling", "[stats][stddev]")
    {
    std::vector<double> values = { 2, 4, 4, 4, 5, 5, 7, 9 };
    CHECK_THAT(2.138089, WithinRel(statistics::standard_deviation(values, true), 1e-6));
    CHECK_THAT(2, WithinRel(statistics::standard_deviation(values, false), 1e-6));
    }

TEST_CASE("Std Dev Sampling2", "[stats][stddev]")
    {
    std::vector<double> values = { 0.390625, 19.580625, 4.730625, 2.030625, 2.480625, 2.030625, 6.630625, 2.480625 };
    CHECK_THAT(6.174666995, WithinRel(statistics::standard_deviation(values, true), 1e-6));
    CHECK_THAT(5.775872093, WithinRel(statistics::standard_deviation(values, false), 1e-6));
    }

TEST_CASE("StdDevArray", "[stats][stddev]")
    {
    std::vector<double> values = { 5.2, 67.0, 0.0, -1.1, 555.684 };
    CHECK_THAT(242.244877, WithinRel(statistics::standard_deviation(values, true), 1e-6));
    values = { 0.0, 0.0, 0.0, 0.0 };
    CHECK_THAT(0.000000, WithinRel(statistics::standard_deviation(values, true), 1e-6));
    values = { -500142.2541, -5974.25681, -84689.26547, -579954.26578 };
    CHECK_THAT(289267.707083, WithinRel(statistics::standard_deviation(values, true), 1e-6));
    values = { 0.266448615, 0.11703829, 0.665102469, 0.633862468, 0.509262405, 0.371353823, 0.494912922, 0.608961596 };
    CHECK_THAT(0.193114, WithinRel(statistics::standard_deviation(values, true), 1e-5));

    values.clear();
    values.reserve(1000000);
    for (size_t i = 0; i < 1000000; ++i)
        { values.push_back(57855.56894 + i); }
    CHECK_THAT(288675.278932, WithinRel(statistics::standard_deviation(values, true), 1e-6));
    }

TEST_CASE("StdDevContainer", "[stats][stddev]")
    {
    std::vector<double> values;
    values.push_back(5.2);
    values.push_back(67.0);
    values.push_back(0.0);
    values.push_back(-1.1);
    values.push_back(555.684);
    CHECK_THAT(242.244877, WithinRel(statistics::standard_deviation(values, true), 1e-6));
    values.clear();
    values.push_back(0.0);
    values.push_back(0.0);
    values.push_back(0.0);
    values.push_back(0.0);
    CHECK_THAT(0.000000, WithinRel(statistics::standard_deviation(values, true), 1e-6));
    values.clear();
    values.push_back(-500142.2541);
    values.push_back(-5974.25681);
    values.push_back(-84689.26547);
    values.push_back(-579954.26578);
    CHECK_THAT(289267.707083, WithinRel(statistics::standard_deviation(values, true), 1e-6));
    values.clear();
    values.push_back(0.266448615);
    values.push_back(0.11703829);
    values.push_back(0.665102469);
    values.push_back(0.633862468);
    values.push_back(0.509262405);
    values.push_back(0.371353823);
    values.push_back(0.494912922);
    values.push_back(0.608961596);
    CHECK_THAT(0.193114, WithinRel(statistics::standard_deviation(values, true), 1e-5));

    values.clear();
    values.reserve(1'000'000);
    for (size_t i = 0; i < 1'000'000; ++i)
        { values.push_back(57855.56894 + i); }
    CHECK_THAT(288675.278932, WithinRel(statistics::standard_deviation(values, true), 1e-6));
    }

TEST_CASE("StdDevOneObs", "[stats][stddev]")
    {
    std::vector<double> singleValues = { 5.2 };
    CHECK_THROWS(WithinRel(statistics::standard_deviation(singleValues, true), 1e-6));
    }

TEST_CASE("Std Dev No Obs", "[stats][stddev]")
    {
    std::vector<double> values = { 5.2 };
    CHECK_THROWS(WithinRel(statistics::standard_deviation(values, true), 1e-6));
    }

TEST_CASE("Skewness", "[stats][skewness]")
    {
    std::vector<double> values = { 5, 9, 6, 7, 6, 4, 3, -3, 17, 6 };
    CHECK_THAT(0.655624318326135, WithinRel(statistics::skewness(values, true), 1e-6));
    values = { 5.8, 8.6, 6.1, 7.2, 12.9, 9.1, 8.1, 6.3, 5, 3.5, 3.7, 9.6, 3.8, 7.8, 9.4 };
    CHECK_THAT(0.426722223352162, WithinRel(statistics::skewness(values, true), 1e-6));
    values = { -579954.26578, -500142.2541, -84689.26547, -5974.25681 };
    CHECK_THAT(-0.00177720554350465, WithinRel(statistics::skewness(values, true), 1e-6));
    }

TEST_CASE("Kurtosis", "[stats][kurtosis]")
    {
    std::vector<double> values = { 5, 9, 6, 7, 6, 4, 3, -3, 17, 6 };
    CHECK_THAT(3.06151337502657, WithinRel(statistics::kurtosis(values, true), 1e-6));
    values = { 5.8, 8.6, 6.1, 7.2, 12.9, 9.1, 8.1, 6.3, 5, 3.5, 3.7, 9.6, 3.8, 7.8, 9.4 };
    CHECK_THAT(0.0920228776443861, WithinRel(statistics::kurtosis(values, true), 1e-6));
    values = { -579954.26578, -500142.2541, -84689.26547, -5974.25681 };
    CHECK_THAT(-5.2679181267415, WithinRel(statistics::kurtosis(values, true), 1e-6));
    }

TEST_CASE("SEM", "[stats][sem]")
    {
    std::vector<double> values = { 5, 9, 6, 7, 6, 4, 3, -3, 17, 6 };
    CHECK_THAT(1.58464857653396, WithinRel(statistics::standard_error_of_mean(values, true), 1e-6));
    values = { 5.8, 8.6, 6.1, 7.2, 12.9, 9.1, 8.1, 6.3, 5, 3.5, 3.7, 9.6, 3.8, 7.8, 9.4 };
    CHECK_THAT(0.67636751538865, WithinRel(statistics::standard_error_of_mean(values, true), 1e-6));
    values = { -579954.26578, -500142.2541, -84689.26547, -5974.25681 };
    CHECK_THAT(144633.85354154, WithinRel(statistics::standard_error_of_mean(values, true), 1e-6));
    }

TEST_CASE("Outliers Array", "[stats][outliers]")
    {
    std::vector<double> values = { 5, 9, 6, 7, 6, 4, 3, -3, 6, 17 };
    statistics::find_outliers fo(values);
    CHECK_THAT(-0.5, WithinRel(fo.get_lower_outlier_boundary(), 1e-6));
    CHECK_THAT(11.5, WithinRel(fo.get_upper_outlier_boundary(), 1e-6));
    CHECK(static_cast<double>(-5) == fo.get_lower_extreme_boundary());
    CHECK(static_cast<double>(16) == fo.get_upper_extreme_boundary());
    CHECK(values.begin() + 7 == fo());
    CHECK(values.begin() + 9 == fo());
    CHECK(values.begin() + 10 == fo());//end of the trail
    CHECK(values.begin() + 10 == fo());//end of the trail still
    }

TEST_CASE("Outliers container", "[stats][outliers]")
    {
    std::vector<double> values;
    values.push_back(5);
    values.push_back(9);
    values.push_back(-3);
    values.push_back(6);
    values.push_back(7);
    values.push_back(6);
    values.push_back(6);
    values.push_back(4);
    values.push_back(3);
    values.push_back(17);
    statistics::find_outliers fo(values);
    CHECK_THAT(-0.5, WithinRel(fo.get_lower_outlier_boundary(), 1e-6));
    CHECK_THAT(11.5, WithinRel(fo.get_upper_outlier_boundary(), 1e-6));
    CHECK(static_cast<double>(-5) == fo.get_lower_extreme_boundary());
    CHECK(static_cast<double>(16) == fo.get_upper_extreme_boundary());
    CHECK(values.begin()+2 == fo());
    CHECK(values.begin()+9  ==  fo());
    CHECK(values.end() == fo()); // end of the trail
    CHECK(values.end() == fo()); // end of the trail still
    }