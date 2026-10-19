/** @addtogroup Utilities
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_SPC_RULES_H
#define WISTERIA_SPC_RULES_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace statistics
    {
    /// @brief Rules for detecting non-random patterns in a process.
    enum class spc_rule
        {
        /// @brief A run of points all above or all below the center line.
        shift,
        /// @brief A run of points all increasing or all decreasing.
        trend,
        /// @brief A run of points alternating up and down.
        alternating,
        /// @brief Too few or too many runs about the center line.
        runs
        };

    /// @brief A pattern found by a rule.
    struct spc_rule_violation
        {
        /// @brief The rule that found the pattern.
        spc_rule rule{ spc_rule::shift };
        /// @brief The indices of the points that make up the pattern.
        std::vector<size_t> points;
        };

    /// @brief The number of runs about the center line and the critical values for it.
    struct runs_summary
        {
        /// @brief The number of points that are not missing and not on the center line.
        size_t useful_observations{ 0 };
        /// @brief The number of runs (consecutive points on the same side of the center line).
        size_t runs{ 0 };
        /// @brief Having this many runs or fewer is a signal (zero if no count is).
        size_t lower_limit{ 0 };
        /// @brief Having this many runs or more is a signal.
        size_t upper_limit{ std::numeric_limits<size_t>::max() };
        /// @brief The number of runs expected in a random sequence.
        double expected_runs{ 0 };
        /// @brief The most points in a single run.
        size_t longest_run{ 0 };
        /// @brief The probability of this many runs or fewer in a random sequence.\n
        ///     A small value suggests clustering (e.g., a shift).
        double clustering_p_value{ 1 };
        /// @brief The probability of this many runs or more in a random sequence.\n
        ///     A small value suggests mixtures (e.g., oscillation or two alternating sources).
        double mixtures_p_value{ 1 };

        /// @returns @c true if there are too few runs, suggesting a shift or trend.
        [[nodiscard]]
        bool has_too_few_runs() const noexcept
            {
            return runs <= lower_limit;
            }

        /// @returns @c true if there are too many runs, suggesting oscillation.
        [[nodiscard]]
        bool has_too_many_runs() const noexcept
            {
            return runs >= upper_limit;
            }
        };

    /** @brief Run chart rules for detecting non-random patterns in a sequence of values.
        @details Missing values (NaN) are skipped by shifts and run counts,
            but restart a trend or an alternating run.
        @par Citation:
            Perla, R. J., Provost, L. P., and Murray, S. K. "The run chart: a simple analytical
            tool for learning from variation in healthcare processes."
            BMJ Quality & Safety, 20(1), 46-51, 2011.

            Nelson, L. S. "The Shewhart control chart: tests for special causes."
            Journal of Quality Technology, 16(4), 237-239, 1984.

            Swed, F. S., and Eisenhart, C. "Tables for testing randomness of grouping in a
            sequence of alternatives." The Annals of Mathematical Statistics, 14(1), 66-87, 1943.*/
    class spc_rules
        {
      public:
        /// @brief The fewest useful observations needed to test the number of runs.
        constexpr static size_t MIN_RUNS_OBSERVATIONS{ 10 };
        /// @brief The smallest allowed shift length.
        constexpr static size_t MIN_SHIFT_LENGTH{ 2 };
        /// @brief The smallest allowed trend length.
        constexpr static size_t MIN_TREND_LENGTH{ 3 };
        /// @brief The smallest allowed alternating length.
        constexpr static size_t MIN_ALTERNATING_LENGTH{ 3 };

        /** @brief Finds runs of points all above or all below the center line.
            @param values The values, in sequence.
            @param center The center line.
            @param min_length The fewest points that make a shift.
            @returns The shifts found.\n
                Points on the center line are skipped and neither break nor count toward a shift.
            @throws std::invalid_argument If @p min_length is less than @c MIN_SHIFT_LENGTH.*/
        [[nodiscard]]
        static std::vector<spc_rule_violation>
        find_shifts(const std::vector<double>& values, const double center, const size_t min_length)
            {
            check_min_length(min_length, MIN_SHIFT_LENGTH);
            std::vector<spc_rule_violation> violations;
            if (!std::isfinite(center))
                {
                return violations;
                }

            std::vector<size_t> run;
            int side{ 0 };
            for (size_t index = 0; index < values.size(); ++index)
                {
                const double value{ values[index] };
                if (!std::isfinite(value) || value == center)
                    {
                    continue;
                    }
                const int currentSide{ (value > center) ? 1 : -1 };
                if (currentSide != side)
                    {
                    add_if_long_enough(violations, spc_rule::shift, run, min_length);
                    side = currentSide;
                    }
                run.push_back(index);
                }
            add_if_long_enough(violations, spc_rule::shift, run, min_length);
            return violations;
            }

        /** @brief Finds runs of points all increasing or all decreasing.
            @param values The values, in sequence.
            @param min_length The fewest points that make a trend.
            @returns The trends found.\n
                A repeated value is skipped and neither breaks nor counts toward a trend.
            @throws std::invalid_argument If @p min_length is less than @c MIN_TREND_LENGTH.*/
        [[nodiscard]]
        static std::vector<spc_rule_violation> find_trends(const std::vector<double>& values,
                                                           const size_t min_length)
            {
            check_min_length(min_length, MIN_TREND_LENGTH);
            std::vector<spc_rule_violation> violations;
            std::vector<size_t> run;
            int direction{ 0 };
            for (size_t index = 0; index < values.size(); ++index)
                {
                const double value{ values[index] };
                if (!std::isfinite(value))
                    {
                    add_if_long_enough(violations, spc_rule::trend, run, min_length);
                    direction = 0;
                    continue;
                    }
                if (run.empty())
                    {
                    run.push_back(index);
                    continue;
                    }
                const double previous{ values[run.back()] };
                if (value == previous)
                    {
                    continue;
                    }
                const int step{ (value > previous) ? 1 : -1 };
                if (direction != 0 && step != direction)
                    {
                    // the turning point starts the next trend
                    const size_t turningPoint{ run.back() };
                    add_if_long_enough(violations, spc_rule::trend, run, min_length);
                    run.push_back(turningPoint);
                    }
                direction = step;
                run.push_back(index);
                }
            add_if_long_enough(violations, spc_rule::trend, run, min_length);
            return violations;
            }

        /** @brief Finds runs of points alternating up and down.
            @param values The values, in sequence.
            @param min_length The fewest points that make an alternating run.
            @returns The alternating runs found.\n
                A repeated value restarts the run.
            @throws std::invalid_argument If @p min_length is less than @c MIN_ALTERNATING_LENGTH.*/
        [[nodiscard]]
        static std::vector<spc_rule_violation> find_alternating(const std::vector<double>& values,
                                                                const size_t min_length)
            {
            check_min_length(min_length, MIN_ALTERNATING_LENGTH);
            std::vector<spc_rule_violation> violations;
            std::vector<size_t> run;
            int lastStep{ 0 };
            for (size_t index = 0; index < values.size(); ++index)
                {
                const double value{ values[index] };
                if (!std::isfinite(value))
                    {
                    add_if_long_enough(violations, spc_rule::alternating, run, min_length);
                    lastStep = 0;
                    continue;
                    }
                if (run.empty())
                    {
                    run.push_back(index);
                    continue;
                    }
                const double previous{ values[run.back()] };
                if (value == previous)
                    {
                    add_if_long_enough(violations, spc_rule::alternating, run, min_length);
                    run.push_back(index);
                    lastStep = 0;
                    continue;
                    }
                const int step{ (value > previous) ? 1 : -1 };
                if (lastStep != 0 && step == lastStep)
                    {
                    // two steps in the same direction, so the previous point starts a new run
                    const size_t lastPoint{ run.back() };
                    add_if_long_enough(violations, spc_rule::alternating, run, min_length);
                    run.push_back(lastPoint);
                    }
                lastStep = step;
                run.push_back(index);
                }
            add_if_long_enough(violations, spc_rule::alternating, run, min_length);
            return violations;
            }

        /** @brief Counts the runs about the center line and tests whether that count
                is unusual for a random sequence.
            @param values The values, in sequence.
            @param center The center line.
            @param alpha The significance level (split evenly between both tails).
            @returns The number of runs, its critical values, and its exact p-values,
                or @c std::nullopt if there are fewer than @c MIN_RUNS_OBSERVATIONS
                useful observations.
            @throws std::invalid_argument If @p alpha is not between 0 and 1.*/
        [[nodiscard]]
        static std::optional<runs_summary> count_runs(const std::vector<double>& values,
                                                      const double center,
                                                      const double alpha = 0.05)
            {
            check_alpha(alpha);
            if (!std::isfinite(center))
                {
                return std::nullopt;
                }

            size_t below{ 0 };
            size_t above{ 0 };
            size_t runs{ 0 };
            size_t currentLength{ 0 };
            size_t longestRun{ 0 };
            int side{ 0 };
            for (const auto value : values)
                {
                if (!std::isfinite(value) || value == center)
                    {
                    continue;
                    }
                const int currentSide{ (value > center) ? 1 : -1 };
                if (currentSide > 0)
                    {
                    ++above;
                    }
                else
                    {
                    ++below;
                    }
                if (currentSide != side)
                    {
                    ++runs;
                    side = currentSide;
                    currentLength = 0;
                    }
                ++currentLength;
                longestRun = std::max(longestRun, currentLength);
                }

            if (below + above < MIN_RUNS_OBSERVATIONS)
                {
                return std::nullopt;
                }

            const auto probabilities = runs_distribution(below, above);
            double clusteringPValue{ 0 };
            double mixturesPValue{ 0 };
            for (size_t runCount = 0; runCount < probabilities.size(); ++runCount)
                {
                if (runCount <= runs)
                    {
                    clusteringPValue += probabilities[runCount];
                    }
                if (runCount >= runs)
                    {
                    mixturesPValue += probabilities[runCount];
                    }
                }

            const auto [lowerLimit, upperLimit] = runs_limits(below, above, alpha);
            const auto total{ static_cast<double>(below + above) };
            return runs_summary{
                .useful_observations = below + above,
                .runs = runs,
                .lower_limit = lowerLimit,
                .upper_limit = upperLimit,
                .expected_runs =
                    1 + ((2 * static_cast<double>(below) * static_cast<double>(above)) / total),
                .longest_run = longestRun,
                .clustering_p_value = std::min(clusteringPValue, 1.0),
                .mixtures_p_value = std::min(mixturesPValue, 1.0)
            };
            }

        /** @brief Finds the critical values for the number of runs, using the exact
                distribution of runs in a random sequence.
            @param below The number of points below the center line.
            @param above The number of points above the center line.
            @param alpha The significance level (split evenly between both tails).
            @returns The lower and upper critical values. Having the lower value or fewer runs
                (or the upper value or more) is a signal.
                The lower value is zero if no count is too few.
            @throws std::invalid_argument If @p alpha is not between 0 and 1.*/
        [[nodiscard]]
        static std::pair<size_t, size_t> runs_limits(const size_t below, const size_t above,
                                                     const double alpha = 0.05)
            {
            check_alpha(alpha);
            const auto probabilities = runs_distribution(below, above);
            const size_t maxRuns{ probabilities.size() - 1 };

            const double tail{ alpha / 2 };
            size_t lowerLimit{ 0 };
            double cumulative{ 0 };
            for (size_t runCount = 2; runCount <= maxRuns; ++runCount)
                {
                cumulative += probabilities[runCount];
                if (cumulative > tail)
                    {
                    break;
                    }
                lowerLimit = runCount;
                }

            size_t upperLimit{ maxRuns + 1 };
            cumulative = 0;
            for (size_t runCount = maxRuns; runCount >= 2; --runCount)
                {
                cumulative += probabilities[runCount];
                if (cumulative > tail)
                    {
                    break;
                    }
                upperLimit = runCount;
                }

            return { lowerLimit, upperLimit };
            }

      private:
        /// @returns The probability of each number of runs in a random sequence
        ///     (indexed by the number of runs).
        [[nodiscard]]
        static std::vector<double> runs_distribution(const size_t below, const size_t above)
            {
            const size_t maxRuns{ (below == above) ? below + above :
                                                     (2 * std::min(below, above)) + 1 };
            std::vector<double> probabilities(maxRuns + 1, 0.0);
            // all points on one side is always a single run
            if (below == 0 || above == 0)
                {
                if (maxRuns > 0)
                    {
                    probabilities[1] = 1;
                    }
                return probabilities;
                }

            const double logArrangements{ log_binomial(below + above, below) };
            for (size_t runCount = 2; runCount <= maxRuns; ++runCount)
                {
                const size_t half{ runCount / 2 };
                probabilities[runCount] =
                    (runCount % 2 == 0) ?
                        2 * std::exp(log_binomial(below - 1, half - 1) +
                                     log_binomial(above - 1, half - 1) - logArrangements) :
                        std::exp(log_binomial(below - 1, half) + log_binomial(above - 1, half - 1) -
                                 logArrangements) +
                            std::exp(log_binomial(below - 1, half - 1) +
                                     log_binomial(above - 1, half) - logArrangements);
                }
            return probabilities;
            }

        static void check_alpha(const double alpha)
            {
            if (!std::isfinite(alpha) || alpha <= 0 || alpha >= 1)
                {
                throw std::invalid_argument("Significance level must be between 0 and 1.");
                }
            }

        static void check_min_length(const size_t min_length, const size_t smallest)
            {
            if (min_length < smallest)
                {
                throw std::invalid_argument("SPC rule length is too short.");
                }
            }

        /// @brief Adds the run as a violation if it is long enough, then clears it.
        static void add_if_long_enough(std::vector<spc_rule_violation>& violations,
                                       const spc_rule rule, std::vector<size_t>& run,
                                       const size_t min_length)
            {
            if (run.size() >= min_length)
                {
                violations.push_back(spc_rule_violation{ rule, std::move(run) });
                }
            run.clear();
            }

        /// @returns The log of the binomial coefficient, or negative infinity
        ///     if @p chosen is more than @p total.
        [[nodiscard]]
        static double log_binomial(const size_t total, const size_t chosen)
            {
            if (chosen > total)
                {
                return -std::numeric_limits<double>::infinity();
                }
            return std::lgamma(static_cast<double>(total) + 1) -
                   std::lgamma(static_cast<double>(chosen) + 1) -
                   std::lgamma(static_cast<double>(total - chosen) + 1);
            }
        };
    } // namespace statistics

/** @}*/

#endif // WISTERIA_SPC_RULES_H
