// NOLINTBEGIN
// clang-format off

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/math/spcrules.h"

using namespace Catch::Matchers;
using namespace statistics;

constexpr double NaN{ std::numeric_limits<double>::quiet_NaN() };

TEST_CASE("SPC shifts", "[spc][shift]")
    {
    SECTION("At threshold")
        {
        const auto violations = spc_rules::find_shifts({ 1, 1, 1, 1, 1, 1 }, 0, 6);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].rule == spc_rule::shift);
        CHECK(violations[0].points == std::vector<size_t>{ 0, 1, 2, 3, 4, 5 });
        }
    SECTION("Below threshold")
        {
        CHECK(spc_rules::find_shifts({ 1, 1, 1, 1, 1, -1 }, 0, 6).empty());
        }
    SECTION("Long run is one violation")
        {
        const auto violations =
            spc_rules::find_shifts({ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 }, 0, 6);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].points.size() == 10);
        }
    SECTION("Both sides")
        {
        const auto violations =
            spc_rules::find_shifts({ 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1 }, 0, 6);
        REQUIRE(violations.size() == 2);
        CHECK(violations[1].points == std::vector<size_t>{ 6, 7, 8, 9, 10, 11 });
        }
    SECTION("Points on the center line are skipped")
        {
        const auto violations = spc_rules::find_shifts({ 1, 1, 0, 1, 1, 1, 1 }, 0, 6);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].points == std::vector<size_t>{ 0, 1, 3, 4, 5, 6 });
        }
    SECTION("Missing values are skipped")
        {
        const auto violations = spc_rules::find_shifts({ 1, 1, NaN, 1, 1, 1, 1 }, 0, 6);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].points == std::vector<size_t>{ 0, 1, 3, 4, 5, 6 });
        }
    SECTION("Custom threshold")
        {
        CHECK(spc_rules::find_shifts({ 1, 1, 1, -1 }, 0, 3).size() == 1);
        }
    SECTION("Empty and missing data")
        {
        CHECK(spc_rules::find_shifts({}, 0, 6).empty());
        CHECK(spc_rules::find_shifts({ NaN, NaN }, 0, 2).empty());
        CHECK(spc_rules::find_shifts({ 1, 1, 1 }, NaN, 2).empty());
        }
    SECTION("Invalid length")
        {
        CHECK_THROWS_AS(spc_rules::find_shifts({ 1, 1 }, 0, 1), std::invalid_argument);
        CHECK_NOTHROW(spc_rules::find_shifts({ 1, 1 }, 0, spc_rules::MIN_SHIFT_LENGTH));
        }
    }

TEST_CASE("SPC trends", "[spc][trend]")
    {
    SECTION("At threshold")
        {
        const auto violations = spc_rules::find_trends({ 1, 2, 3, 4, 5 }, 5);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].rule == spc_rule::trend);
        CHECK(violations[0].points == std::vector<size_t>{ 0, 1, 2, 3, 4 });
        }
    SECTION("Below threshold")
        {
        CHECK(spc_rules::find_trends({ 1, 2, 3, 4 }, 5).empty());
        }
    SECTION("Repeated values are skipped")
        {
        const auto violations = spc_rules::find_trends({ 1, 2, 2, 3, 4, 5 }, 5);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].points == std::vector<size_t>{ 0, 1, 3, 4, 5 });
        }
    SECTION("Missing value restarts a trend")
        {
        CHECK(spc_rules::find_trends({ 1, 2, NaN, 3, 4, 5 }, 5).empty());
        const auto violations = spc_rules::find_trends({ 1, 2, NaN, 3, 4, 5 }, 3);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].points == std::vector<size_t>{ 3, 4, 5 });
        }
    SECTION("Turning point is shared")
        {
        const auto violations = spc_rules::find_trends({ 1, 2, 3, 4, 5, 4, 3, 2, 1 }, 5);
        REQUIRE(violations.size() == 2);
        CHECK(violations[0].points == std::vector<size_t>{ 0, 1, 2, 3, 4 });
        CHECK(violations[1].points == std::vector<size_t>{ 4, 5, 6, 7, 8 });
        }
    SECTION("Empty data")
        {
        CHECK(spc_rules::find_trends({}, 5).empty());
        CHECK(spc_rules::find_trends({ NaN }, 3).empty());
        }
    SECTION("Invalid length")
        {
        CHECK_THROWS_AS(spc_rules::find_trends({ 1, 2, 3 }, 2), std::invalid_argument);
        CHECK_NOTHROW(spc_rules::find_trends({ 1, 2, 3 }, spc_rules::MIN_TREND_LENGTH));
        }
    }

TEST_CASE("SPC alternating", "[spc][alternating]")
    {
    SECTION("At threshold")
        {
        const auto violations = spc_rules::find_alternating({ 1, 3, 1, 3, 1, 3 }, 6);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].rule == spc_rule::alternating);
        CHECK(violations[0].points == std::vector<size_t>{ 0, 1, 2, 3, 4, 5 });
        }
    SECTION("Below threshold")
        {
        CHECK(spc_rules::find_alternating({ 1, 3, 1, 3, 1, 3 }, 7).empty());
        }
    SECTION("Repeated value restarts the run")
        {
        const auto violations = spc_rules::find_alternating({ 1, 3, 1, 1, 3, 1, 3 }, 4);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].points == std::vector<size_t>{ 3, 4, 5, 6 });
        }
    SECTION("Two steps the same way restarts the run")
        {
        const auto violations = spc_rules::find_alternating({ 1, 3, 5, 1, 5, 1 }, 4);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].points == std::vector<size_t>{ 1, 2, 3, 4, 5 });
        }
    SECTION("Missing value restarts the run")
        {
        const std::vector<double> values{ 1, 3, 1, NaN, 3, 1, 3, 1 };
        const auto violations = spc_rules::find_alternating(values, 4);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].points == std::vector<size_t>{ 4, 5, 6, 7 });

        // with a shorter length, the points on both sides of the gap are separate runs
        const auto shortViolations = spc_rules::find_alternating(values, 3);
        REQUIRE(shortViolations.size() == 2);
        CHECK(shortViolations[0].points == std::vector<size_t>{ 0, 1, 2 });
        CHECK(shortViolations[1].points == std::vector<size_t>{ 4, 5, 6, 7 });
        }
    SECTION("Missing values at the ends")
        {
        const auto violations =
            spc_rules::find_alternating({ NaN, 1, 3, 1, 3, NaN }, 4);
        REQUIRE(violations.size() == 1);
        CHECK(violations[0].points == std::vector<size_t>{ 1, 2, 3, 4 });
        CHECK(spc_rules::find_alternating({ NaN, NaN, NaN }, 3).empty());
        }
    SECTION("Invalid length")
        {
        CHECK_THROWS_AS(spc_rules::find_alternating({ 1, 3, 1 }, 2), std::invalid_argument);
        CHECK_NOTHROW(spc_rules::find_alternating({ 1, 3, 1 }, spc_rules::MIN_ALTERNATING_LENGTH));
        }
    }

/*  R code to cross-check a RunChart of the piston ring data (qcc::pistonrings,
    from Montgomery's "Introduction to Statistical Quality Control"), plotted as sample means
    with "baseline": 25 and the shift, trend, and runs rules enabled:

    library(dplyr)
    library(qicharts2)
    library(runcharter)
    library(DescTools)

    data(pistonrings, package = "qcc")
    rings <- pistonrings

    # number of samples used for the median (0 to use all samples)
    baseline <- 25

    # one point per sample (use median() here to match a "median" sample statistic)
    means <- rings |>
      group_by(sample) |>
      summarise(diameter = mean(diameter), .groups = "drop")

    baseline_rows <- if (baseline > 0) seq_len(baseline) else seq_len(nrow(means))

    # ---- qicharts2 ----
    # averages each sample itself (agg.fun), so pass the raw rows;
    # freeze is qicharts2's name for the baseline
    qic_freeze <- if (baseline > 0) baseline else NULL
    p1 <- qic(sample, diameter, data = rings, chart = "run", agg.fun = "mean",
              freeze = qic_freeze,
              title = "Piston rings: sample means (qicharts2)")
    print(p1)
    print(summary(p1))
    # compare: the median (cl) and longest.run with the legend's median and longest run

    # individual observations instead of samples
    p2 <- qic(diameter, data = rings, chart = "run", title = "Piston rings: observations")
    print(p2)

    # ---- runcharter ----
    # wants a date column and a grouping column, so make both up;
    # it only looks for runs after the baseline, and starts a new median after each one it finds
    rc_data <- means |>
      mutate(grp = "rings", date = as.Date("2026-01-01") + sample - 1)
    rc <- runcharter(rc_data,
                     med_rows  = baseline,  # median from the baseline samples
                     runlength = 6,         # same as the default shift length
                     direction = "both",
                     datecol = date, grpvar = grp, yval = diameter,
                     chart_title = "Piston rings: sample means (runcharter)")
    print(rc)

    # ---- the run chart rules, by hand (neither package has the trend rule) ----
    med <- median(means$diameter[baseline_rows])
    side <- sign(means$diameter - med)
    side <- side[side != 0]                       # points on the median are skipped
    side_runs <- rle(side)

    n1   <- sum(side < 0)                         # points below the median
    n2   <- sum(side > 0)                         # points above
    runs <- length(side_runs$lengths)
    N    <- n1 + n2

    cat("median:", med, "\n")
    cat("runs:", runs,
        " expected:", 1 + 2 * n1 * n2 / N,
        " longest:", max(side_runs$lengths), "\n")
    cat("shifts (6+):", sum(side_runs$lengths >= 6), "\n")

    steps <- sign(diff(means$diameter))
    steps <- steps[steps != 0]                    # repeated values are skipped
    cat("trends (5+ points):", sum(rle(steps)$lengths >= 4), "\n")

    # ---- exact runs test p-values, as shown in the legend ----
    # probability of seeing r runs (Swed and Eisenhart)
    runs_pmf <- function(r) {
      if (r %% 2 == 0) {
        k <- r / 2
        2 * choose(n1 - 1, k - 1) * choose(n2 - 1, k - 1) / choose(N, n1)
      } else {
        k <- (r - 1) / 2
        (choose(n1 - 1, k) * choose(n2 - 1, k - 1) +
            choose(n1 - 1, k - 1) * choose(n2 - 1, k)) / choose(N, n1)
      }
    }
    all_r <- 2:N
    pmf   <- sapply(all_r, runs_pmf)
    cat("clustering p:", round(sum(pmf[all_r <= runs]), 3), "\n")   # this few runs or fewer
    cat("mixtures p:  ", round(sum(pmf[all_r >= runs]), 3), "\n")   # this many runs or more

    # the same test from DescTools; pass above/below (not the raw means),
    # or it splits at its own median of all the data and ignores the baseline
    print(RunsTest(side > 0, exact = TRUE, alternative = "less"))     # clustering
    print(RunsTest(side > 0, exact = TRUE, alternative = "greater"))  # mixtures
*/

TEST_CASE("SPC runs limits", "[spc][runs]")
    {
    // critical values from Swed, F. S., and Eisenhart, C. "Tables for testing randomness of
    // grouping in a sequence of alternatives." The Annals of Mathematical Statistics,
    // 14(1), 66-87, 1943.
    SECTION("Swed-Eisenhart 10 and 10")
        {
        const auto [lower, upper] = spc_rules::runs_limits(10, 10);
        CHECK(lower == 6);
        CHECK(upper == 16);
        }
    SECTION("Swed-Eisenhart 5 and 5")
        {
        const auto [lower, upper] = spc_rules::runs_limits(5, 5);
        CHECK(lower == 2);
        CHECK(upper == 10);
        }
    SECTION("Swed-Eisenhart 20 and 20")
        {
        const auto [lower, upper] = spc_rules::runs_limits(20, 20);
        CHECK(lower == 14);
        CHECK(upper == 28);
        }
    SECTION("Swed-Eisenhart 5 and 10 has no upper value")
        {
        const auto [lower, upper] = spc_rules::runs_limits(5, 10);
        CHECK(lower == 3);
        // the most runs possible is 11, so no count is too many
        CHECK(upper == 12);
        }
    SECTION("Too few points for any signal")
        {
        const auto [lower, upper] = spc_rules::runs_limits(3, 3);
        CHECK(lower == 0);
        CHECK(upper == 7);
        }
    SECTION("All on one side")
        {
        const auto [lower, upper] = spc_rules::runs_limits(0, 12);
        CHECK(lower == 0);
        CHECK(upper == 2);
        }
    // the cumulative probabilities for 10 and 10 are 0.0185 for 6 or fewer runs,
    // 0.0513 for 7 or fewer, 0.2422 for 9 or fewer, and 0.4141 for 10 or fewer
    // (the distribution is symmetric about 11)
    SECTION("Tiny significance level has no signal")
        {
        const auto [lower, upper] = spc_rules::runs_limits(10, 10, 1e-12);
        CHECK(lower == 0);
        // the most runs possible is 20
        CHECK(upper == 21);
        }
    SECTION("Larger significance level")
        {
        // 7 or fewer runs (0.0513) is still above 0.05, so the limits match alpha = 0.05
        const auto [lower, upper] = spc_rules::runs_limits(10, 10, 0.10);
        CHECK(lower == 6);
        CHECK(upper == 16);
        }
    SECTION("Significance level of 0.5")
        {
        const auto [lower, upper] = spc_rules::runs_limits(10, 10, 0.5);
        CHECK(lower == 9);
        CHECK(upper == 13);
        }
    SECTION("Significance level near 1 leaves only the middle count")
        {
        const auto [lower, upper] = spc_rules::runs_limits(10, 10, 0.999);
        CHECK(lower == 10);
        CHECK(upper == 12);
        }
    SECTION("Significance levels just inside the range are accepted")
        {
        CHECK_NOTHROW(spc_rules::runs_limits(10, 10, std::nextafter(0.0, 1.0)));
        CHECK_NOTHROW(spc_rules::runs_limits(10, 10, std::nextafter(1.0, 0.0)));
        }
    SECTION("Invalid significance level")
        {
        CHECK_THROWS_AS(spc_rules::runs_limits(10, 10, NaN), std::invalid_argument);
        CHECK_THROWS_AS(spc_rules::runs_limits(10, 10, 0), std::invalid_argument);
        CHECK_THROWS_AS(spc_rules::runs_limits(10, 10, 1), std::invalid_argument);
        CHECK_THROWS_AS(spc_rules::runs_limits(10, 10, 2), std::invalid_argument);
        CHECK_THROWS_AS(spc_rules::runs_limits(10, 10, -0.05), std::invalid_argument);
        CHECK_THROWS_AS(
            spc_rules::runs_limits(10, 10, std::numeric_limits<double>::infinity()),
            std::invalid_argument);
        CHECK_THROWS_AS(spc_rules::count_runs({ 1, -1 }, 0, NaN), std::invalid_argument);
        }
    }

TEST_CASE("SPC count runs", "[spc][runs]")
    {
    SECTION("Too few useful observations")
        {
        CHECK_FALSE(spc_rules::count_runs({ 1, -1, 1, -1, 1, -1, 1, -1, 1, 0 }, 0).has_value());
        }
    SECTION("Too few runs")
        {
        std::vector<double> values(10, 1.0);
        values.insert(values.end(), 10, -1.0);
        const auto summary = spc_rules::count_runs(values, 0);
        REQUIRE(summary.has_value());
        CHECK(summary->useful_observations == 20);
        CHECK(summary->runs == 2);
        CHECK(summary->has_too_few_runs());
        CHECK_FALSE(summary->has_too_many_runs());
        CHECK(summary->longest_run == 10);
        // 1 + (2 * 10 * 10) / 20
        CHECK(summary->expected_runs == 11.0);
        // only 2 of the 184,756 arrangements have 2 runs
        CHECK_THAT(summary->clustering_p_value, WithinAbs(2.0 / 184'756, 1e-12));
        CHECK_THAT(summary->mixtures_p_value, WithinAbs(1.0, 1e-12));
        }
    SECTION("P-values agree with the critical values")
        {
        // ten above and ten below, in six runs, which is at the lower critical value
        std::vector<double> values;
        for (const size_t length : { 3, 4, 3, 3, 4, 3 })
            {
            const double side = (values.empty() || values.back() < 0) ? 1.0 : -1.0;
            values.insert(values.end(), length, side);
            }
        const auto summary = spc_rules::count_runs(values, 0);
        REQUIRE(summary.has_value());
        CHECK(summary->runs == 6);
        CHECK(summary->longest_run == 4);
        CHECK(summary->has_too_few_runs());
        // 3,422 of the 184,756 arrangements have 6 runs or fewer
        CHECK_THAT(summary->clustering_p_value, WithinAbs(3'422.0 / 184'756, 1e-12));
        CHECK(summary->clustering_p_value <= 0.025);
        }
    SECTION("Too many runs")
        {
        std::vector<double> values;
        for (size_t i = 0; i < 10; ++i)
            {
            values.push_back(1);
            values.push_back(-1);
            }
        const auto summary = spc_rules::count_runs(values, 0);
        REQUIRE(summary.has_value());
        CHECK(summary->runs == 20);
        CHECK(summary->has_too_many_runs());
        CHECK_FALSE(summary->has_too_few_runs());
        }
    SECTION("Points on the center line and missing values are not useful")
        {
        std::vector<double> values(10, 1.0);
        values.insert(values.end(), 10, -1.0);
        values.push_back(0);
        values.push_back(NaN);
        const auto summary = spc_rules::count_runs(values, 0);
        REQUIRE(summary.has_value());
        CHECK(summary->useful_observations == 20);
        }
    }

// NOLINTEND
// clang-format on
