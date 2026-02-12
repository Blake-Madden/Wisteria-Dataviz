// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/statistics.h"
#include <array>

using namespace Catch::Matchers;
using namespace statistics;

TEST_CASE("Valid N", "[stats][validN]")
    {
    using std::numeric_limits;

    CHECK(statistics::valid_n({}) == 0);

    CHECK(statistics::valid_n(std::vector<double>{
        numeric_limits<double>::quiet_NaN() }) == 0);

    CHECK(statistics::valid_n(std::vector<double>{
        numeric_limits<double>::quiet_NaN(),
        numeric_limits<double>::quiet_NaN() }) == 0);

    CHECK(statistics::valid_n(std::vector<double>{
        numeric_limits<double>::quiet_NaN(), 1.0,
        numeric_limits<double>::quiet_NaN() }) == 1);

    CHECK(statistics::valid_n(std::vector<double>{ -7.8, 1.0, 5.1 }) == 3);
    }

TEST_CASE("Valid N excludes infinities", "[stats][validN][inf]")
    {
    using std::numeric_limits;

    CHECK(statistics::valid_n(std::vector<double>{
        numeric_limits<double>::infinity(),
        -numeric_limits<double>::infinity() }) == 0);

    CHECK(statistics::valid_n(std::vector<double>{
        numeric_limits<double>::infinity(), 2.0,
        -numeric_limits<double>::infinity(), -3.5 }) == 2);
    }

TEST_CASE("Valid N includes signed zero, subnormals, and extremes", "[stats][validN][edge]")
    {
    using std::numeric_limits;

    // signed zeros are finite
    CHECK(statistics::valid_n(std::vector<double>{ +0.0, -0.0 }) == 2);

    // subnormal (denormal) magnitudes are finite
    constexpr double dmin = numeric_limits<double>::denorm_min();
    CHECK(statistics::valid_n(std::vector<double>{ dmin, -dmin }) == 2);

    // extreme but finite values are included
    CHECK(statistics::valid_n(std::vector<double>{
        numeric_limits<double>::max(),
        numeric_limits<double>::lowest(),
        42.0 }) == 3);
    }

TEST_CASE("Valid N mixed bag", "[stats][validN][mixed]")
    {
    using std::numeric_limits;

    CHECK(statistics::valid_n(std::vector<double>{
        numeric_limits<double>::quiet_NaN(),
        numeric_limits<double>::infinity(),
        -1.25,
        -numeric_limits<double>::infinity(),
        0.0,
        numeric_limits<double>::quiet_NaN() }) == 2);
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

TEST_CASE("Normalize exceptions", "[stats][normalize]")
    {
    double noWarn;
    CHECK_THROWS(noWarn = statistics::normalize(1,50,51)); // value out of range
    CHECK_THROWS(noWarn = statistics::normalize(51,50,50)); // bad range
    }

TEST_CASE("normalize returns input value when any argument is non-finite",
          "[stats][normalize][nan][inf]")
    {
    using std::numeric_limits;

    const double inf  = numeric_limits<double>::infinity();
    const double ninf = -numeric_limits<double>::infinity();
    const double nan  = numeric_limits<double>::quiet_NaN();
    const double v    = 42.0;

    // value non-finite → return that same non-finite value
    CHECK(std::isinf(statistics::normalize(0.0, 100.0, inf)));
    CHECK(std::isinf(statistics::normalize(0.0, 100.0, ninf)));
    CHECK(std::isnan(statistics::normalize(0.0, 100.0, nan)));

    // range_min non-finite → return finite value unchanged
    CHECK(statistics::normalize(inf, 100.0, v)  == v);
    CHECK(statistics::normalize(ninf, 100.0, v) == v);
    CHECK(statistics::normalize(nan, 100.0, v)  == v);

    // range_max non-finite → return finite value unchanged
    CHECK(statistics::normalize(0.0, inf, v)  == v);
    CHECK(statistics::normalize(0.0, ninf, v) == v);
    CHECK(statistics::normalize(0.0, nan, v)  == v);

    // both bounds non-finite → still return finite value unchanged
    CHECK(statistics::normalize(inf,  inf,  v) == v);
    CHECK(statistics::normalize(ninf, inf,  v) == v);
    CHECK(statistics::normalize(ninf, ninf, v) == v);
    }

TEST_CASE("normalize behaves normally for finite inputs", "[stats][normalize]")
{
    // Exactly representable finite cases
    CHECK(statistics::normalize(0.0, 100.0, 50.0) == 0.5);
    CHECK(statistics::normalize(0.0, 100.0, 0.0)  == 0.0);
    CHECK(statistics::normalize(0.0, 100.0, 100.0) == 1.0);
    }

TEST_CASE("mean handles finite and non-finite inputs correctly", "[stats][mean]")
    {
    using namespace statistics;
    using std::numeric_limits;

    constexpr double inf  = numeric_limits<double>::infinity();
    constexpr double ninf = -numeric_limits<double>::infinity();
    constexpr double nan  = numeric_limits<double>::quiet_NaN();

    SECTION("all finite → valid mean")
        {
        CHECK(mean({1.0, 2.0, 3.0}) == 2.0);
        }

    SECTION("mixture of finite and NaN → NaNs ignored")
        {
        CHECK(mean({1.0, 2.0, nan, 3.0}) == 2.0);
        }

    SECTION("mixture of finite and infinities → infinities ignored")
        {
        CHECK(mean({1.0, inf, 2.0, ninf, 3.0}) == 2.0);
        }

    SECTION("only non-finite values → throws invalid_argument")
        {
        CHECK_THROWS_AS(mean({nan, nan, inf, ninf}), std::invalid_argument);
        }

    SECTION("empty vector → throws invalid_argument")
        {
        CHECK_THROWS_AS(mean({}), std::invalid_argument);
        }

    SECTION("sum zero but finite values → returns 0")
        {
        // mean = (−1 + 1 + 0) / 3 = 0
        CHECK(mean({-1.0, 1.0, 0.0}) == 0.0);
        }

    SECTION("finite plus one infinity → ignores infinity, averages finite ones")
        {
        // ignoring +inf leaves {10, 20}
        CHECK(mean({10.0, 20.0, inf}) == 15.0);
        }

    SECTION("single finite among non-finites → returns that value")
        {
        CHECK(mean({inf, nan, 7.5, ninf}) == 7.5);
        }
    }

TEST_CASE("median incorrectly includes infinities", "[stats][median][inf]")
    {
    using namespace statistics;
    using std::numeric_limits;
    using Catch::Matchers::WithinAbs;

    constexpr double inf  = numeric_limits<double>::infinity();
    constexpr double ninf = -numeric_limits<double>::infinity();

    const std::vector<double> data{ 1.0, 3.0, 2.0, inf, ninf };

    const double m = median(data);

    CHECK_THAT(m, WithinAbs(2.0, 1e-6));
    }

TEST_CASE("sum_of_powers ignores infinities and NaNs", "[stats][sum_of_powers][inf][nan]")
    {
    using namespace statistics;
    using std::numeric_limits;
    using std::isfinite;

    SECTION("finite values only")
        {
        const std::vector<double> data{ 1.0, 2.0, 3.0 };
        // mean = 2.0 → (1−2)² + (2−2)² + (3−2)² = 2
        CHECK_THAT(sum_of_powers(data, 2.0), Catch::Matchers::WithinAbs(2.0, 1e-12));
        }

    SECTION("infinities are skipped")
        {
        const std::vector<double> data{ 1.0, 2.0, 3.0,
                                        numeric_limits<double>::infinity(),
                                        -numeric_limits<double>::infinity() };
        // same finite subset {1,2,3} → result still 2
        CHECK_THAT(sum_of_powers(data, 2.0), Catch::Matchers::WithinAbs(2.0, 1e-12));
        }

    SECTION("NaNs are skipped")
        {
        const std::vector<double> data{ 1.0, numeric_limits<double>::quiet_NaN(),
                                        2.0, 3.0 };
        // mean of finite {1,2,3} = 2 → sum of squares = 2
        CHECK_THAT(sum_of_powers(data, 2.0), Catch::Matchers::WithinAbs(2.0, 1e-12));
        }

    SECTION("mix of NaN and infinities ignored")
        {
        const std::vector<double> data{
            numeric_limits<double>::quiet_NaN(),
            1.0, 2.0, 3.0,
            numeric_limits<double>::infinity(),
            -numeric_limits<double>::infinity()
        };
        CHECK_THAT(sum_of_powers(data, 2.0), Catch::Matchers::WithinAbs(2.0, 1e-12));
        }

    SECTION("only non-finite values → mean() throws")
        {
        const std::vector<double> data{
            numeric_limits<double>::quiet_NaN(),
            numeric_limits<double>::infinity(),
            -numeric_limits<double>::infinity()
        };
        CHECK_THROWS_AS(sum_of_powers(data, 2.0), std::invalid_argument);
        }
    }

TEST_CASE("sum_of_powers: only non-finite values → throws via mean()", "[stats][sum_of_powers][guard]")
    {
    using namespace statistics;
    using std::numeric_limits;

    // mean(data) throws (no finite observations), so sum_of_powers should propagate that
    const std::vector<double> data{
        numeric_limits<double>::infinity(),
        -numeric_limits<double>::infinity(),
        numeric_limits<double>::quiet_NaN()
    };

    CHECK_THROWS_AS(sum_of_powers(data, /*power=*/2.0), std::invalid_argument);
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
    // beyond that. Note that we are going with what SPSS reports (they get the same as us up to the hundredths place)
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

TEST_CASE("Standard deviation throws with only one valid value", "[stats][stddev]")
    {
    using namespace statistics;
    using std::numeric_limits;

    // one finite, one NaN → valid_n() == 1
    std::vector<double> data{
        numeric_limits<double>::quiet_NaN(),
        42.0
    };

    CHECK_THROWS_AS(standard_deviation(data, /*is_sample=*/true), std::invalid_argument);
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

// This set assumes: 
// - statistics::find_outliers is constructed with a const reference to the data vector.
// - operator() returns an iterator into that same vector.
// - It returns end() when there are no more outliers.

TEST_CASE("find_outliers: NaN present but no numeric outliers", "[stats][outliers][nan]")
    {
    using std::numeric_limits;

    const std::vector<double> data{
        10.0, 10.2, 9.8, 10.1, 9.9, 10.05,
        numeric_limits<double>::quiet_NaN()
    };

    statistics::find_outliers fo{ data };

    auto it1 = fo();
    CHECK(it1 == data.cend());

    // persistent: second call should also yield end()
    auto it2 = fo();
    CHECK(it2 == data.cend());
    }

TEST_CASE("find_outliers: NaN before a real outlier is ignored", "[stats][outliers][nan][order]")
    {
    using std::numeric_limits;

    const std::vector<double> data{
        1.0, 1.02, 0.98, 1.01,
        numeric_limits<double>::quiet_NaN(),
        100.0
    };

    statistics::find_outliers fo{ data };

    auto it1 = fo();
    REQUIRE(it1 != data.cend());
    CHECK(std::isfinite(*it1));
    CHECK(*it1 == 100.0);

    auto it2 = fo();
    CHECK(it2 == data.cend());
    }

TEST_CASE("find_outliers: only NaNs and infinities", "[stats][outliers][nan][inf]")
    {
    using std::numeric_limits;

    const std::vector<double> data{
        numeric_limits<double>::quiet_NaN(),
        numeric_limits<double>::infinity(),
        -numeric_limits<double>::infinity()
    };

    statistics::find_outliers fo{ data };

    auto it = fo();
    CHECK(it == data.cend());
    }

TEST_CASE("find_outliers: multiple finite outliers, NaNs interspersed", "[stats][outliers][nan][multi]")
    {
    using std::numeric_limits;

    const std::vector<double> data{
        2.0, 2.1, 1.9, 2.05,
        numeric_limits<double>::quiet_NaN(),
        -50.0,
        2.02,
        numeric_limits<double>::quiet_NaN(),
        75.0
    };

    statistics::find_outliers fo{ data };

    auto it1 = fo();
    REQUIRE(it1 != data.cend());
    CHECK(std::isfinite(*it1));
    CHECK(*it1 == -50.0);

    auto it2 = fo();
    REQUIRE(it2 != data.cend());
    CHECK(std::isfinite(*it2));
    CHECK(*it2 == 75.0);

    auto it3 = fo();
    CHECK(it3 == data.cend());
    }

// ---------------- z_score tests ----------------

TEST_CASE("z_score basic positive values", "[z_score]")
    {
    CHECK_THAT(z_score(10.0, 5.0, 2.0), WithinRel(2.5, 1e-6));
    CHECK_THAT(z_score(5.0, 5.0, 2.0), WithinAbs(0.0, 1e-6));
    CHECK_THAT(z_score(0.0, 5.0, 2.0), WithinRel(-2.5, 1e-6));
    }

TEST_CASE("z_score with negative mean and stdDev", "[z_score]")
    {
    CHECK_THAT(z_score(-5.0, -10.0, 2.0), WithinRel(2.5, 1e-6));
    CHECK_THAT(z_score(-10.0, -10.0, 2.0), WithinAbs(0.0, 1e-6));
    CHECK_THAT(z_score(-15.0, -10.0, 2.0), WithinRel(-2.5, 1e-6));
    }

TEST_CASE("z_score with zero standard deviation", "[z_score]")
    {
    // safe_divide returns 0 for division by zero
    CHECK_THAT(z_score(10.0, 5.0, 0.0), WithinAbs(0.0, 1e-6));
    CHECK_THAT(z_score(5.0, 5.0, 0.0), WithinAbs(0.0, 1e-6));
    }

// Deterministic fuzz-style test for z_score
TEST_CASE("z_score deterministic range", "[z_score][fuzz]")
    {
    for (double value = -10.0; value <= 10.0; value += 1.0)
        {
        for (double mean = -5.0; mean <= 5.0; mean += 1.0)
            {
            for (double stdDev : {0.0, 1.0, 2.0, 5.0})
                {
                double result = z_score(value, mean, stdDev);

                if (stdDev == 0.0)
                    {
                    // safe_divide returns 0 for zero stdDev
                    CHECK_THAT(result, WithinAbs(0.0, 1e-6));
                    }
                else
                    {
                    CHECK(result == (value - mean) / stdDev);
                    }
                }
            }
        }
    }

// ---------------- phi_coefficient tests ----------------

TEST_CASE("phi_coefficient basic correlation", "[phi_coefficient]")
    {
    int a1[] = {1, 0, 1, 0};
    int a2[] = {1, 0, 1, 0};

    CHECK_THAT(phi_coefficient(a1, a1 + 4, a2, a2 + 4), WithinRel(1.0, 1e-6));
    }

TEST_CASE("phi_coefficient negative correlation", "[phi_coefficient]")
    {
    int a1[] = {1, 0, 1, 0};
    int a2[] = {0, 1, 0, 1};

    CHECK_THAT(phi_coefficient(a1, a1 + 4, a2, a2 + 4), WithinRel(-1.0, 1e-6));
    }

TEST_CASE("phi_coefficient no correlation", "[phi_coefficient]")
    {
    int a1[] = {1, 1, 0, 0};
    int a2[] = {0, 1, 0, 1};

    CHECK_THAT(phi_coefficient(a1, a1 + 4, a2, a2 + 4), WithinAbs(0.0, 1e-6));
    }

TEST_CASE("phi_coefficient all zeros or ones", "[phi_coefficient]")
    {
    int allOnes[] = {1, 1, 1, 1};
    int allZeros[] = {0, 0, 0, 0};

    // perfectly undefined, safe_divide returns 0
    CHECK_THAT(phi_coefficient(allOnes, allOnes + 4, allZeros, allZeros + 4), WithinAbs(0.0, 1e-6));
    }

TEST_CASE("phi_coefficient mismatched array sizes throws", "[phi_coefficient]")
    {
    int a1[] = {1, 0, 1};
    int a2[] = {1, 0, 1, 0};

    CHECK_THROWS_AS(phi_coefficient(a1, a1 + 3, a2, a2 + 4), std::range_error);
    }

// ---------------- phi_coefficient deterministic fuzz-style test ----------------

TEST_CASE("phi_coefficient always in [-1, 1] for all 0/1 arrays", "[phi_coefficient][fuzz]")
    {
    constexpr int size = 8;
    std::array<int, size> a1{};
    std::array<int, size> a2{};

    // Iterate over all combinations of 0/1 arrays (2^8 = 256 combinations)
    for (int mask1 = 0; mask1 < (1 << size); ++mask1)
        {
        for (int mask2 = 0; mask2 < (1 << size); ++mask2)
            {
            for (int i = 0; i < size; ++i)
                {
                a1[i] = (mask1 & (1 << i)) ? 1 : 0;
                a2[i] = (mask2 & (1 << i)) ? 1 : 0;
                }

            double result = phi_coefficient(a1.data(), a1.data() + size, a2.data(), a2.data() + size);

            // Always within [-1, 1]
            CHECK(result >= -1.0);
            CHECK(result <= 1.0);

            // If result is numerically zero, check with WithinAbs
            if (result == 0.0)
                {
                CHECK_THAT(result, WithinAbs(0.0, 1e-6));
                }
            }
        }
    }

// ---------------- regularized_incomplete_beta tests ----------------
// Reference values from Wolfram Alpha and NIST Digital Library of Mathematical Functions
TEST_CASE("regularized_incomplete_beta boundary values", "[stats][beta]")
    {
    // I_0(a,b) = 0 for all a,b > 0
    CHECK_THAT(regularized_incomplete_beta(0.0, 1.0, 1.0), WithinAbs(0.0, 1e-10));
    CHECK_THAT(regularized_incomplete_beta(0.0, 2.0, 3.0), WithinAbs(0.0, 1e-10));

    // I_1(a,b) = 1 for all a,b > 0
    CHECK_THAT(regularized_incomplete_beta(1.0, 1.0, 1.0), WithinAbs(1.0, 1e-10));
    CHECK_THAT(regularized_incomplete_beta(1.0, 2.0, 3.0), WithinAbs(1.0, 1e-10));
    }

TEST_CASE("regularized_incomplete_beta uniform distribution", "[stats][beta]")
    {
    // When a=b=1 (uniform distribution), I_x(1,1) = x
    CHECK_THAT(regularized_incomplete_beta(0.0, 1.0, 1.0), WithinAbs(0.0, 1e-10));
    CHECK_THAT(regularized_incomplete_beta(0.25, 1.0, 1.0), WithinAbs(0.25, 1e-10));
    CHECK_THAT(regularized_incomplete_beta(0.5, 1.0, 1.0), WithinAbs(0.5, 1e-10));
    CHECK_THAT(regularized_incomplete_beta(0.75, 1.0, 1.0), WithinAbs(0.75, 1e-10));
    CHECK_THAT(regularized_incomplete_beta(1.0, 1.0, 1.0), WithinAbs(1.0, 1e-10));
    }

TEST_CASE("regularized_incomplete_beta known values", "[stats][beta]")
    {
    // Reference: Wolfram Alpha - "RegularizedIncompleteBeta[0.5, 2, 3]"
    // I_0.5(2,3) = 0.6875
    CHECK_THAT(regularized_incomplete_beta(0.5, 2.0, 3.0), WithinRel(0.6875, 1e-6));

    // I_0.3(2,5) = 0.57982... (from Wolfram Alpha: BetaRegularized[0.3, 2, 5])
    CHECK_THAT(regularized_incomplete_beta(0.3, 2.0, 5.0), WithinRel(0.579825, 1e-4));

    // I_0.7(3,2) = 0.6517 (from Wolfram Alpha: BetaRegularized[0.7, 3, 2])
    CHECK_THAT(regularized_incomplete_beta(0.7, 3.0, 2.0), WithinRel(0.6517, 1e-3));
    }

TEST_CASE("regularized_incomplete_beta symmetry relation", "[stats][beta]")
    {
    // I_x(a,b) = 1 - I_{1-x}(b,a)
    const double x = 0.3;
    const double a = 2.5;
    const double b = 4.0;

    const double left = regularized_incomplete_beta(x, a, b);
    const double right = 1.0 - regularized_incomplete_beta(1.0 - x, b, a);

    CHECK_THAT(left, WithinRel(right, 1e-8));
    }

TEST_CASE("regularized_incomplete_beta edge cases", "[stats][beta]")
    {
    // Limits of x
    CHECK(regularized_incomplete_beta(0.0, 2.0, 3.0) == 0.0);
    CHECK(regularized_incomplete_beta(1.0, 2.0, 3.0) == 1.0);

    // Identity: I_x(a, 1) = x^a
    CHECK_THAT(regularized_incomplete_beta(0.5, 3.0, 1.0), WithinRel(0.125, 1e-12));
    }

TEST_CASE("regularized_incomplete_beta precision at tails", "[stats][beta]")
    {
    // Testing small x with large a/b (common in high-significance p-values)
    // R: pbeta(0.1, 10, 10) = 0.000003929882449...
    const double val = regularized_incomplete_beta(0.1, 10.0, 10.0);
    CHECK_THAT(val, WithinRel(0.000003929882449, 1e-6));
    }

TEST_CASE("regularized_incomplete_beta precision symmetry point for large parameters", "[stats][beta]")
    {
    // Published case: Symmetry point for large parameters
    // R: pbeta(0.5, 500, 500)
    const double large_sym = regularized_incomplete_beta(0.5, 500.0, 500.0);
    CHECK_THAT(large_sym, WithinRel(0.5, 1e-6));
    }

TEST_CASE("regularized_incomplete_beta precision high symmetrys", "[stats][beta]")
    {
    // Published case: High asymmetry
    // R: pbeta(0.01, 0.5, 20.0) = 0.4713699
    const double j_shape = regularized_incomplete_beta(0.01, 0.5, 20.0);
    CHECK_THAT(j_shape, WithinRel(0.4713699, 1e-6));
    }

TEST_CASE("regularized_incomplete_beta precision Abramowitz & Stegun case", "[stats][beta]")
    {
    // A&S Table 26.4 Case
    // x=0.2, a=2, b=2
    // I_0.2(2, 2) = 3(0.2)^2 - 2(0.2)^3 = 0.12 - 0.016 = 0.104
    CHECK_THAT(regularized_incomplete_beta(0.2, 2.0, 2.0), WithinRel(0.104, 1e-6));
    }

TEST_CASE("regularized_incomplete_beta t-distribution bridge", "[stats][beta]")
    {
    // Verify the t-stat to p-value conversion
    // Case: t = 2.0, df = 10
    // x = df / (df + t^2) = 10 / (10 + 4) = 0.7142857...
    // I_x(df/2, 0.5) should match the two-tailed p-value
    const double t = 2.0;
    const double df = 10.0;
    const double x = df / (df + t * t);
    const double a = df / 2.0;
    const double b = 0.5;

    const double p_val = regularized_incomplete_beta(x, a, b);
    
    // R: 2 * pt(-2.0, df=10) = 0.07338803...
    CHECK_THAT(p_val, WithinRel(0.07338803, 1e-6));
    }

TEST_CASE("regularized_incomplete_beta large parameters", "[stats][beta]")
    {
    // Tests Lanczos/Log-Gamma stability to prevent overflow
    // R: pbeta(0.5, 100, 100) should be exactly 0.5 due to symmetry
    CHECK_THAT(regularized_incomplete_beta(0.5, 100.0, 100.0), WithinRel(0.5, 1e-12));
    }

TEST_CASE("regularized_incomplete_beta bad shapes", "[stats][beta]")
    {
    CHECK(std::isnan(regularized_incomplete_beta(0.5, -1, 100.0)));
    CHECK(std::isnan(regularized_incomplete_beta(0.5, 1, -0.5)));
    }

// ---------------- t_distribution_p_value tests ----------------
// Reference: Standard t-distribution tables from statistics textbooks
// Montgomery, D.C. & Runger, G.C. (2014). Applied Statistics and Probability
// for Engineers (6th ed.). Wiley. Appendix Table III.
TEST_CASE("t_distribution_p_value at t=0", "[stats][ttest]")
    {
    // t=0 should give p=1 for any df (symmetric around 0)
    CHECK_THAT(t_distribution_p_value(0.0, 1.0), WithinAbs(1.0, 1e-6));
    CHECK_THAT(t_distribution_p_value(0.0, 10.0), WithinAbs(1.0, 1e-6));
    CHECK_THAT(t_distribution_p_value(0.0, 100.0), WithinAbs(1.0, 1e-6));
    }

TEST_CASE("t_distribution_p_value symmetry", "[stats][ttest]")
    {
    // p-value should be the same for +t and -t (two-tailed)
    CHECK_THAT(t_distribution_p_value(2.0, 10.0),
               WithinRel(t_distribution_p_value(-2.0, 10.0), 1e-10));
    CHECK_THAT(t_distribution_p_value(1.5, 5.0),
               WithinRel(t_distribution_p_value(-1.5, 5.0), 1e-10));
    }

TEST_CASE("t_distribution_p_value known critical values", "[stats][ttest]")
    {
    // Reference: Standard t-tables
    // For df=10, t=2.228 gives two-tailed p=0.05 (approximately)
    CHECK_THAT(t_distribution_p_value(2.228, 10.0), WithinAbs(0.05, 0.002));

    // For df=10, t=3.169 gives two-tailed p=0.01 (approximately)
    CHECK_THAT(t_distribution_p_value(3.169, 10.0), WithinAbs(0.01, 0.002));

    // For df=20, t=2.086 gives two-tailed p=0.05 (approximately)
    CHECK_THAT(t_distribution_p_value(2.086, 20.0), WithinAbs(0.05, 0.002));

    // For df=120, t=1.980 gives two-tailed p=0.05 (approximately, close to z)
    CHECK_THAT(t_distribution_p_value(1.980, 120.0), WithinAbs(0.05, 0.002));
    }

TEST_CASE("t_distribution_p_value converges to normal", "[stats][ttest]")
    {
    // As df -> infinity, t-distribution approaches standard normal
    // For z=1.96, two-tailed p ≈ 0.05
    CHECK_THAT(t_distribution_p_value(1.96, 1000.0), WithinAbs(0.05, 0.002));

    // For z=2.576, two-tailed p ≈ 0.01
    CHECK_THAT(t_distribution_p_value(2.576, 1000.0), WithinAbs(0.01, 0.002));
    }

TEST_CASE("t_distribution_p_value invalid inputs", "[stats][ttest]")
    {
    // df <= 0 should return NaN
    CHECK(std::isnan(t_distribution_p_value(1.0, 0.0)));
    CHECK(std::isnan(t_distribution_p_value(1.0, -1.0)));

    // NaN inputs should return NaN
    CHECK(std::isnan(t_distribution_p_value(std::numeric_limits<double>::quiet_NaN(), 10.0)));
    CHECK(std::isnan(t_distribution_p_value(1.0, std::numeric_limits<double>::quiet_NaN())));
    }

/*  R code to validate simple linear regression:

    # Define the data
    x <- c(10, 8, 13, 9, 11, 14, 6, 4, 12, 7, 5)
    y <- c(8.04, 6.95, 7.58, 8.81, 8.33, 9.96, 7.24, 4.26, 10.84, 4.82, 5.68)

    # Perform linear regression
    model <- lm(y ~ x, na.action = na.omit)

    # View the full summary
    summary_info <- summary(model)
    print(summary_info)

    # 1. Residual Standard Error (matches results.standard_error)
    # This is the 'sigma' field in the summary object
    res_std_error <- summary_info$sigma

    # 2. Standard Error of the Slope (matches results.slope_standard_error)
    # This is found in the coefficients table [row "x", column "Std. Error"]
    slope_std_error <- summary_info$coefficients["x", "Std. Error"]

    # Extract the p-value for the slope 'x'
    # The coefficients table is a matrix: [Row "x", Column "Pr(>|t|)"]
    p_val <- summary_info$coefficients["x", "Pr(>|t|)"]

    # Extract the t-statistic while we're at it
    t_stat <- summary_info$coefficients["x", "t value"]

    # Extract specific values for comparison
    cat("\n--- Comparison Values ---\n")
    cat("Slope:      ", coef(model)["x"], "\n")
    cat("Intercept:  ", coef(model)["(Intercept)"], "\n")
    # Derived from R-squared with the correct sign
    correlation_derived <- sqrt(summary_info$r.squared) * sign(coef(model)["x"])
    cat("Correlation (from R2):", correlation_derived, "\n")
    cat("R-squared:  ", summary_info$r.squared, "\n")
    cat("Residual Std Error (S): ", res_std_error, "\n")
    cat("Slope Std Error (SE):   ", slope_std_error, "\n")
    cat("\n--- Significance Testing ---\n")
    cat("T-statistic: ", t_stat, "\n")
    cat("P-value:     ", p_val, "\n")
*/

// ---------------- linear_regression tests ----------------
// Reference: Kutner, M.H., Nachtsheim, C.J., Neter, J., & Li, W. (2005).
// Applied Linear Statistical Models (5th ed.). McGraw-Hill/Irwin.
// Example data from Chapter 1 (Toluca Company dataset, simplified).
TEST_CASE("linear_regression textbook example", "[stats][regression]")
    {
    // textbook example: Hours studying vs. exam score
    // hand-calculated values for verification:
    // mean_x = 3, mean_y = 63
    // SS_xx = 10, SS_yy = 430, SS_xy = 65
    // slope = 65/10 = 6.5, intercept = 63 - 6.5*3 = 43.5
    // r = 65/sqrt(10*430) = 65/65.574 = 0.9912
    const std::vector<double> hours = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    const std::vector<double> scores = { 50.0, 55.0, 65.0, 70.0, 75.0 };

    const auto result = linear_regression(hours, scores);

    CHECK(result.is_valid());
    CHECK(result.n == 5);
    CHECK_THAT(result.slope, WithinRel(6.5, 1e-10));
    CHECK_THAT(result.intercept, WithinRel(43.5, 1e-10));
    CHECK_THAT(result.correlation, WithinRel(0.9912, 1e-3));
    CHECK_THAT(result.r_squared, WithinRel(0.9825, 1e-3));
    }

TEST_CASE("linear_regression perfect positive correlation", "[stats][regression]")
    {
    // perfect positive linear relationship: y = 2x + 3
    const std::vector<double> x = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    const std::vector<double> y = { 5.0, 7.0, 9.0, 11.0, 13.0 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK_THAT(result.slope, WithinAbs(2.0, 1e-10));
    CHECK_THAT(result.intercept, WithinAbs(3.0, 1e-10));
    CHECK_THAT(result.correlation, WithinAbs(1.0, 1e-10));
    CHECK_THAT(result.r_squared, WithinAbs(1.0, 1e-10));
    // perfect fit means residuals = 0, so SE = 0
    CHECK_THAT(result.standard_error, WithinAbs(0.0, 1e-10));
    CHECK_THAT(result.slope_standard_error, WithinAbs(0, 1e-10));
    }

TEST_CASE("linear_regression perfect negative correlation", "[stats][regression]")
    {
    // perfect negative linear relationship: y = -3x + 20
    const std::vector<double> x = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    const std::vector<double> y = { 17.0, 14.0, 11.0, 8.0, 5.0 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK_THAT(result.slope, WithinAbs(-3.0, 1e-10));
    CHECK_THAT(result.intercept, WithinAbs(20.0, 1e-10));
    CHECK_THAT(result.correlation, WithinAbs(-1.0, 1e-10));
    CHECK_THAT(result.r_squared, WithinAbs(1.0, 1e-10));
    CHECK_THAT(result.standard_error, WithinAbs(0.0, 1e-10));
    CHECK_THAT(result.slope_standard_error, WithinAbs(0, 1e-10));
    }

TEST_CASE("linear_regression no correlation", "[stats][regression]")
    {
    // horizontal line: y is constant, no relationship with x
    const std::vector<double> x = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    const std::vector<double> y = { 10.0, 10.0, 10.0, 10.0, 10.0 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK_THAT(result.slope, WithinAbs(0.0, 1e-10));
    CHECK_THAT(result.intercept, WithinAbs(10.0, 1e-10));
    // when y is constant, r and r^2 are perfect prediction of constant,
    // which is "unreliable" and set to NaN
    CHECK(std::isnan(result.r_squared));
    CHECK(std::isnan(result.correlation));
    CHECK_THAT(result.standard_error, WithinAbs(0.0, 1e-10));
    CHECK_THAT(result.slope_standard_error, WithinAbs(0, 1e-10));
    }

TEST_CASE("linear_regression with noise", "[stats][regression]")
    {
    // hand-calculated example with some scatter
    // x = {1, 2, 3, 4, 5}, y = {2.1, 3.9, 6.2, 7.8, 10.1}
    // Best fit line: y ≈ 1.99x + 0.06
    const std::vector<double> x = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    const std::vector<double> y = { 2.1, 3.9, 6.2, 7.8, 10.1 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK(result.n == 5);
    CHECK_THAT(result.slope, WithinRel(1.99, 0.01));
    CHECK_THAT(result.intercept, WithinAbs(0.05, 0.01));
    CHECK(result.correlation > 0.99); // very high correlation
    CHECK(result.r_squared > 0.98);
    CHECK_THAT(result.t_statistic, WithinAbs(33.32129 , 1e-06));
    CHECK(result.p_value < 0.01); // highly significant
    }

TEST_CASE("linear_regression NIST Norris dataset subset", "[stats][regression]")
    {
    // NIST Statistical Reference Datasets (subset of 16 points)
    // https://www.itl.nist.gov/div898/strd/lls/data/Norris.shtml
    // The Norris dataset has y ≈ x relationship (slope ≈ 1, intercept ≈ 0)
    // Hand-verified values for this 16-point subset:
    // slope ≈ 1.00321, intercept ≈ -0.2140
    const std::vector<double> x = { 0.2, 337.4, 118.2, 884.6, 10.1, 226.5, 666.3, 996.3,
                                    448.6, 777.0, 558.2, 0.4, 0.6, 775.5, 666.9, 338.0 };
    const std::vector<double> y = { 0.1, 338.8, 118.1, 888.0, 9.2, 228.1, 668.5, 998.5,
                                    449.1, 779.0, 559.2, 0.3, 0.1, 778.1, 668.8, 339.3 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK(result.n == 16);
    // verified values for this subset
    CHECK_THAT(result.slope, WithinRel(1.00321, 1e-4));
    CHECK_THAT(result.intercept, WithinAbs(-0.214, 0.01));
    // very high correlation expected (y ≈ x)
    CHECK_THAT(result.r_squared ,WithinAbs(0.9999, 1e-4));
    }

TEST_CASE("linear_regression minimum valid points (n=2)", "[stats][regression]")
    {
    // with exactly 2 points, we can fit a perfect line
    const std::vector<double> x = { 0.0, 10.0 };
    const std::vector<double> y = { 5.0, 25.0 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK(result.n == 2);
    CHECK_THAT(result.slope, WithinAbs(2.0, 1e-10));
    CHECK_THAT(result.intercept, WithinAbs(5.0, 1e-10));
    CHECK_THAT(result.correlation, WithinAbs(1.0, 1e-10));
    CHECK_THAT(result.r_squared, WithinAbs(1.0, 1e-10));
    // with n=2, df=0, so SE and p-value will be undefined/NaN
    CHECK(std::isnan(result.standard_error));
    CHECK(std::isnan(result.p_value));
    }

TEST_CASE("linear_regression with three points", "[stats][regression]")
    {
    // three points with some scatter: (1,2), (2,4), (3,5)
    // hand-calculated: slope = 1.5, intercept = 0.667, r ≈ 0.9820
    const std::vector<double> x = { 1.0, 2.0, 3.0 };
    const std::vector<double> y = { 2.0, 4.0, 5.0 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK(result.n == 3);
    CHECK_THAT(result.slope, WithinRel(1.5, 1e-10));
    CHECK_THAT(result.intercept, WithinRel(0.6666667, 1e-6));
    CHECK_THAT(result.correlation, WithinRel(0.9819, 1e-3));
    // with n=3, df=1
    CHECK_THAT(result.t_statistic, WithinAbs(5.196152, 1e-6));
    CHECK_THAT(result.p_value, WithinAbs(0.1210377, 1e-6));
    }

// ---------------- linear_regression edge cases ----------------
TEST_CASE("linear_regression empty input", "[stats][regression][edge]")
    {
    const std::vector<double> empty;

    const auto result = linear_regression(empty, empty);

    CHECK_FALSE(result.is_valid());
    CHECK(result.n == 0);
    CHECK(std::isnan(result.slope));
    CHECK(std::isnan(result.intercept));
    CHECK(std::isnan(result.correlation));
    CHECK(std::isnan(result.t_statistic));
    CHECK(std::isnan(result.p_value));
    }

TEST_CASE("linear_regression single point", "[stats][regression][edge]")
    {
    const std::vector<double> x = { 5.0 };
    const std::vector<double> y = { 10.0 };

    const auto result = linear_regression(x, y);

    // with only 1 point, regression is undefined
    CHECK_FALSE(result.is_valid());
    // n is 0 because the function returns early before counting valid pairs
    CHECK(result.n == 0);
    CHECK(std::isnan(result.slope));
    CHECK(std::isnan(result.intercept));
    CHECK(std::isnan(result.correlation));
    CHECK(std::isnan(result.t_statistic));
    CHECK(std::isnan(result.p_value));
    }

TEST_CASE("linear_regression mismatched sizes", "[stats][regression][edge]")
    {
    const std::vector<double> x = { 1.0, 2.0, 3.0 };
    const std::vector<double> y = { 1.0, 2.0 };

    const auto result = linear_regression(x, y);

    CHECK_FALSE(result.is_valid());
    CHECK(result.n == 0);
    CHECK(std::isnan(result.slope));
    CHECK(std::isnan(result.intercept));
    CHECK(std::isnan(result.correlation));
    CHECK(std::isnan(result.t_statistic));
    CHECK(std::isnan(result.p_value));
    }

TEST_CASE("linear_regression vertical line (zero variance in x)", "[stats][regression][edge]")
    {
    // all x values are the same -> undefined slope
    const std::vector<double> x = { 5.0, 5.0, 5.0, 5.0 };
    const std::vector<double> y = { 1.0, 2.0, 3.0, 4.0 };

    const auto result = linear_regression(x, y);

    CHECK_FALSE(result.is_valid());
    CHECK(std::isnan(result.slope));
    CHECK(std::isnan(result.intercept));
    CHECK(std::isnan(result.correlation));
    CHECK(std::isnan(result.t_statistic));
    CHECK(std::isnan(result.p_value));
    CHECK(std::isnan(result.standard_error));
    }

TEST_CASE("linear_regression with x NaN value", "[stats][regression][nan]")
    {
    using std::numeric_limits;

    // NaN values should be excluded from the calculation
    const std::vector<double> x = { 1.0, 2.0, numeric_limits<double>::quiet_NaN(), 4.0, 5.0 };
    const std::vector<double> y = { 2.0, 4.0, 6.0, 8.0, 10.0 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK(result.n == 4); // only 4 valid pairs
    CHECK_THAT(result.slope, WithinAbs(2.0, 1e-10));
    CHECK_THAT(result.intercept, WithinAbs(0.0, 1e-10));
    CHECK_THAT(result.correlation, WithinRel(1.0, 1e-6));
    CHECK_THAT(result.r_squared, WithinRel(1.0, 1e-6));
    CHECK(std::isinf(result.t_statistic));
    CHECK_THAT(result.p_value, WithinAbs(0.0, 1e-6));
    }

TEST_CASE("linear_regression with y NaN value", "[stats][regression][nan]")
    {
    using std::numeric_limits;

    // NaN values should be excluded from the calculation
    const std::vector<double> x = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    const std::vector<double> y = { 2.0, 4.0, numeric_limits<double>::quiet_NaN(), 8.0, 10.0 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK(result.n == 4); // only 4 valid pairs
    CHECK_THAT(result.slope, WithinAbs(2.0, 1e-10));
    CHECK_THAT(result.intercept, WithinAbs(0.0, 1e-10));
    CHECK_THAT(result.correlation, WithinRel(1.0, 1e-6));
    CHECK_THAT(result.r_squared, WithinRel(1.0, 1e-6));
    CHECK(std::isinf(result.t_statistic));
    CHECK_THAT(result.p_value, WithinAbs(0.0, 1e-6));
    }

TEST_CASE("linear_regression with infinity values", "[stats][regression][inf]")
    {
    using std::numeric_limits;

    // infinity values should be excluded
    const std::vector<double> x = { 1.0, 2.0, 3.0, numeric_limits<double>::infinity() };
    const std::vector<double> y = { 3.0, 5.0, 7.0, 100.0 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK(result.n == 3);
    CHECK_THAT(result.slope, WithinAbs(2.0, 1e-10));
    CHECK_THAT(result.intercept, WithinAbs(1.0, 1e-10));
    CHECK_THAT(result.correlation, WithinRel(1.0, 1e-6));
    CHECK_THAT(result.r_squared, WithinRel(1.0, 1e-6));
    CHECK(std::isinf(result.t_statistic));
    CHECK_THAT(result.p_value, WithinAbs(0.0, 1e-6));
    }

TEST_CASE("linear_regression all NaN", "[stats][regression][nan]")
    {
    using std::numeric_limits;

    const std::vector<double> x = { numeric_limits<double>::quiet_NaN(),
                                    numeric_limits<double>::quiet_NaN() };
    const std::vector<double> y = { 1.0, 2.0 };

    const auto result = linear_regression(x, y);

    CHECK_FALSE(result.is_valid());
    CHECK(result.n == 0);
    CHECK(std::isnan(result.slope));
    CHECK(std::isnan(result.intercept));
    CHECK(std::isnan(result.correlation));
    CHECK(std::isnan(result.t_statistic));
    CHECK(std::isnan(result.p_value));
    CHECK(std::isnan(result.standard_error));
    }

TEST_CASE("linear_regression negative values", "[stats][regression]")
    {
    // regression with all negative values
    const std::vector<double> x = { -5.0, -4.0, -3.0, -2.0, -1.0 };
    const std::vector<double> y = { -15.0, -12.0, -9.0, -6.0, -3.0 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK_THAT(result.slope, WithinAbs(3.0, 1e-10));
    CHECK_THAT(result.intercept, WithinAbs(0.0, 1e-10));
    CHECK_THAT(result.correlation, WithinAbs(1.0, 1e-10));
    }

TEST_CASE("linear_regression large values", "[stats][regression]")
    {
    // test numerical stability with large values
    const std::vector<double> x = { 1e8, 2e8, 3e8, 4e8, 5e8 };
    const std::vector<double> y = { 2e8, 4e8, 6e8, 8e8, 1e9 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK_THAT(result.slope, WithinAbs(2.0, 1e-6));
    CHECK_THAT(result.intercept, WithinAbs(0.0, 1e6)); // some numerical error expected
    CHECK_THAT(result.correlation, WithinAbs(1.0, 1e-10));
    }

TEST_CASE("linear_regression small values", "[stats][regression]")
    {
    // test numerical stability with small values
    const std::vector<double> x = { 1e-8, 2e-8, 3e-8, 4e-8, 5e-8 };
    const std::vector<double> y = { 3e-8, 6e-8, 9e-8, 1.2e-7, 1.5e-7 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK_THAT(result.slope, WithinAbs(3.0, 1e-6));
    CHECK_THAT(result.correlation, WithinAbs(1.0, 1e-10));
    }

TEST_CASE("linear_regression p-value significance levels", "[stats][regression]")
    {
    // test that significant relationships give small p-values
    // and non-significant relationships give large p-values
    SECTION("highly significant relationship")
        {
        const std::vector<double> x = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        const std::vector<double> y = { 2.1, 3.9, 6.1, 8.0, 9.9, 12.1, 13.9, 16.0, 18.1, 20.0 };

        const auto result = linear_regression(x, y);

        CHECK(result.is_valid());
        CHECK_THAT(result.slope, WithinAbs(1.999394, 1e-6));
        CHECK_THAT(result.intercept, WithinAbs(0.01333333, 1e-6));
        CHECK_THAT(result.correlation, WithinRel(0.9998955, 1e-6));
        CHECK_THAT(result.r_squared, WithinRel(0.9997909, 1e-6));
        CHECK_THAT(result.t_statistic, WithinRel(195.5875 , 1e-6));
        CHECK_THAT(result.p_value, WithinAbs(0.0, 1e-6));
        }

    SECTION("no significant relationship")
        {
        // random-ish data with no clear pattern
        const std::vector<double> x = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        const std::vector<double> y = { 5.2, 4.8, 5.1, 4.9, 5.0, 5.1, 4.8, 5.2, 4.9, 5.0 };

        const auto result = linear_regression(x, y);

        CHECK(result.is_valid());
        CHECK_THAT(result.slope, WithinAbs(-0.004848485, 1e-6));
        CHECK_THAT(result.intercept, WithinAbs(5.026667, 1e-6));
        CHECK_THAT(result.correlation, WithinRel(-0.09847319 , 1e-6));
        CHECK_THAT(result.r_squared, WithinRel(0.00969697, 1e-6));
        CHECK_THAT(result.t_statistic, WithinRel(-0.2798846, 1e-6));
        CHECK_THAT(result.p_value, WithinAbs(0.7866666, 1e-6));
        }
    }

TEST_CASE("linear_regression filtering results in n=1", "[stats][regression][nan]")
    {
    const std::vector<double> x = { 1.0, std::numeric_limits<double>::quiet_NaN() };
    const std::vector<double> y = { 2.0, 4.0 };

    const auto result = linear_regression(x, y);

    CHECK_FALSE(result.is_valid());
    CHECK(result.n == 1);
    CHECK(std::isnan(result.slope));
    }

TEST_CASE("linear_regression near-vertical line", "[stats][regression][edge]")
    {
    // Points are almost vertical: (1.0, 1), (1.00000000001, 2).
    // Slope should be massive, but not NaN.
    const std::vector<double> x = { 1.0, 1.0000000001 };
    const std::vector<double> y = { 1.0, 2.0 };

    const auto result = linear_regression(x, y);

    CHECK(result.is_valid());
    CHECK(result.slope > 1e9); 
    CHECK(std::isfinite(result.slope));
    }

TEST_CASE("linear_regression Anscombe's Quartet Case I", "[stats][regression]")
    {
    // dataset I from Anscombe's Quartet
    const std::vector<double> x = { 10, 8, 13, 9, 11, 14, 6, 4, 12, 7, 5 };
    const std::vector<double> y = { 8.04, 6.95, 7.58, 8.81, 8.33, 9.96, 7.24, 4.26, 10.84, 4.82, 5.68 };

    const auto result = linear_regression(x, y);

    // standard properties for Anscombe's I
    CHECK_THAT(result.slope, WithinRel(0.5, 0.01));
    CHECK_THAT(result.intercept, WithinRel(3.0, 0.01));
    CHECK_THAT(result.r_squared, WithinRel(0.667, 0.01));
    }

TEST_CASE("linear_regression results struct is_valid", "[stats][regression]")
    {
    SECTION("default constructed is invalid")
        {
        linear_regression_results results;
        CHECK_FALSE(results.is_valid());
        }

    SECTION("valid results")
        {
        linear_regression_results results;
        results.n = 5;
        results.slope = 2.0;
        results.intercept = 1.0;
        CHECK(results.is_valid());
        }

    SECTION("n=1 is invalid")
        {
        linear_regression_results results;
        results.n = 1;
        results.slope = 2.0;
        results.intercept = 1.0;
        CHECK_FALSE(results.is_valid());
        }

    SECTION("NaN slope is invalid")
        {
        linear_regression_results results;
        results.n = 5;
        results.slope = std::numeric_limits<double>::quiet_NaN();
        results.intercept = 1.0;
        CHECK_FALSE(results.is_valid());
        }
    }

// NOLINTEND
// clang-format on
