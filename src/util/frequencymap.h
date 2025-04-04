/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __FREQUENCY_MAPS_H__
#define __FREQUENCY_MAPS_H__

#include <cstddef>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

/// @brief Same as a @c std::set, but also keeps a frequency count of every unique value added.
template<typename T, typename Compare = std::less<T>>
class frequency_set
    {
  public:
    /// @private
    using map_type = std::map<T, size_t, Compare>;
    /// @private
    using const_iterator = typename map_type::const_iterator;

    /// @brief Constructor.
    /// @param value A value to initialize the set with.
    explicit frequency_set(const T& value) { insert(value); }

    /// @private
    frequency_set() = default;

    /** @brief Inserts an item into the set.
        @param value The value to insert.
        @returns An iterator to the inserted or updated item.
        @note If a value is already in the set, then that value's count is incremented.*/
    const_iterator insert(const T& value)
        {
        auto [index_iter, inserted] = m_table.try_emplace(value, 1);
        // if it was already there, then just update its counter
        if (!inserted)
            {
            ++(index_iter->second);
            }
        return index_iter;
        }

    /** @brief Inserts an item into the set.
        @param value The value to insert.
        @returns An iterator to the inserted or updated item.
        @note If a value is already in the set, then that value's count is incremented.*/
    const_iterator insert(T&& value)
        {
        auto [index_iter, inserted] = m_table.try_emplace(value, 1);
        // if it was already there, then just update its counter
        if (!inserted)
            {
            ++(index_iter->second);
            }
        return index_iter;
        }

    /// @brief Clears the contents of the set.
    void clear() noexcept { m_table.clear(); }

    /// @returns The (const) set of values and their respective frequency counts.
    [[nodiscard]]
    const map_type& get_data() const noexcept
        {
        return m_table;
        }

  private:
    map_type m_table;
    };

/// @brief Same as a frequency_set, expect it also enables the caller to
///     increment a second frequency count based on a criterion.
template<typename T, typename TCompare = std::less<T>>
class double_frequency_set
    {
  public:
    /// @private
    using map_type = std::map<T, std::pair<size_t, size_t>, TCompare>;
    /// @private
    using const_iterator = typename map_type::const_iterator;

    /** @brief Inserts an item into the set.
        @param value The value to insert.
        @param incrementSecondFrequency Whether or not to increment the custom counter.
        @returns An iterator to the inserted or updated item.
        @note If a value is already in the set, then that value's count is incremented.*/
    const_iterator insert(const T& value, const bool incrementSecondFrequency)
        {
        const size_t secondFrequency = incrementSecondFrequency ? 1 : 0;
        auto [index_iter, inserted] =
            m_table.try_emplace(value, std::make_pair(1 /*raw frequency count*/,
                                                      secondFrequency /*custom frequency count*/));
        // if it was already there, so just update it.
        if (!inserted)
            {
            ++(index_iter->second.first);
            index_iter->second.second += secondFrequency;
            }
        return index_iter;
        }

    /** @brief Inserts an item into the set.
        @param value The value to insert.
        @param incrementSecondFrequency Whether or not to increment the custom counter.
        @returns An iterator to the inserted or updated item.
        @note If a value is already in the set, then that value's count is incremented.*/
    const_iterator insert(T&& value, const bool incrementSecondFrequency)
        {
        const size_t secondFrequency = incrementSecondFrequency ? 1 : 0;
        auto [index_iter, inserted] =
            m_table.try_emplace(value, std::make_pair(1 /*raw frequency count*/,
                                                      secondFrequency /*custom frequency count*/));
        // if it was already there, so just update it.
        if (!inserted)
            {
            ++(index_iter->second.first);
            index_iter->second.second += secondFrequency;
            }
        return index_iter;
        }

    /** @brief Inserts another double_frequency_set into this one,
            copying over (or combining) the items, frequency counts, and custom counts.
        @param that The double_frequency_set to insert into this one.*/
    void operator+=(const double_frequency_set<T, TCompare>& that)
        {
        for (auto iter = that.get_data().cbegin(); iter != that.get_data().cend(); ++iter)
            {
            auto [index_iter, inserted] = m_table.try_emplace(
                iter->first, std::make_pair(iter->second.first /*raw frequency count*/,
                                            iter->second.second /*custom frequency count*/));
            // if it was already there, so just update it.
            if (!inserted)
                {
                index_iter->second.first += iter->second.first;
                index_iter->second.second += iter->second.second;
                }
            }
        }

    /** @brief Inserts another double_frequency_set into this one,
            copying over (or combining) the items and frequency counts,
            but enabling caller to use a different value for the custom counts.
            The value will be used for the custom count of items not already
            in this set, or will be added to items that are already in this set.
        @param that The double_frequency_set to insert into this one.
        @param frequencyIncrement The value to use to increment the custom count.*/
    void insert_with_custom_increment(const double_frequency_set<T, TCompare>& that,
                                      const size_t frequencyIncrement = 1)
        {
        for (auto iter = that.get_data().cbegin(); iter != that.get_data().cend(); ++iter)
            {
            auto [index_iter, inserted] = m_table.try_emplace(
                iter->first,
                std::make_pair(iter->second.first /*raw frequency count*/,
                               frequencyIncrement /*custom frequency count is overridden*/));
            // if it was already there, so just update it.
            if (!inserted)
                {
                index_iter->second.first += iter->second.first;
                // override other item's custom counter
                index_iter->second.second += frequencyIncrement;
                }
            }
        }

    /// @returns The set of values and their respective frequency counts.
    [[nodiscard]]
    const map_type& get_data() const noexcept
        {
        return m_table;
        }

  private:
    map_type m_table;
    };

/// @brief Same as a @c std::set, but also keeps a frequency count of every unique value added,
///     as well as an additional value to accumulate.
template<typename T, typename Compare = std::less<T>>
class aggregate_frequency_set
    {
  public:
    /// @private
    using map_type = std::map<T, std::pair<size_t, double>, Compare>;
    /// @private
    using const_iterator = typename map_type::const_iterator;
    /// @private
    aggregate_frequency_set() = default;

    /** @brief Constructor; inserts an item into the set.
        @param value The value to insert.
        @param aggregateValue The value to add to running total.
        @note If a value is already in the set, then that value's count is incremented.*/
    aggregate_frequency_set(const T& value, double aggregateValue)
        {
        insert(value, aggregateValue);
        }

    /** @brief Inserts an item into the set.
        @param value The value to insert.
        @param aggregateValue The value to add to running total.
        @returns An iterator to the inserted or updated item.
        @note If a value is already in the set, then that value's count is incremented.*/
    const_iterator insert(const T& value, double aggregateValue)
        {
        auto [index_iter, inserted] = m_table.try_emplace(value, std::make_pair(1, aggregateValue));
        // if it was already there, then just update its counter
        if (!inserted)
            {
            ++(index_iter->second.first);
            (index_iter->second.second) += aggregateValue;
            }
        return index_iter;
        }

    /** @brief Inserts an item into the set.
        @param value The value to insert.
        @param aggregateValue The value to add to running total.
        @returns An iterator to the inserted or updated item.
        @note If a value is already in the set, then that value's count is incremented.*/
    const_iterator insert(T&& value, double aggregateValue)
        {
        auto [index_iter, inserted] = m_table.try_emplace(value, std::make_pair(1, aggregateValue));
        // if it was already there, then just update its counter
        if (!inserted)
            {
            ++(index_iter->second.first);
            (index_iter->second.second) += aggregateValue;
            }
        return index_iter;
        }

    /// @brief Clears the contents of the set.
    void clear() noexcept { m_table.clear(); }

    /// @private
    [[nodiscard]]
    const map_type& get_data() const noexcept
        {
        return m_table;
        }

    /// @returns The set of values and their respective counts and totals.
    [[nodiscard]]
    map_type& get_data() noexcept
        {
        return m_table;
        }

  private:
    map_type m_table;
    };

/// @brief Same as a @c std::map, but also keeps a frequency count of every unique value added.
template<typename T1, typename T2, typename Compare = std::less<T1>>
class frequency_map
    {
  public:
    /// Key/(value & count)
    /// @private
    using map_type = std::map<T1, std::pair<T2, size_t>, Compare>;
    /// @private
    using const_iterator = typename map_type::const_iterator;

    /** @brief Inserts a pair of items into the map.
        @param value1 The key.
        @param value2 The value associated with the key.
        @returns An iterator to the inserted or updated item.
        @note If the key is already in the map, then that key's count is incremented;
            however, @c value2 will be ignored.*/
    const_iterator insert(const T1& value1, const T2& value2)
        {
        auto [index_iter, inserted] = m_table.try_emplace(value1, std::make_pair(value2, 1));
        // if it was already there, so just update it.
        if (!inserted)
            {
            ++(index_iter->second.second);
            }
        return index_iter;
        }

    /** @brief Inserts a pair of items into the map.
        @param value1 The key.
        @param value2 The value associated with the key.
        @returns An iterator to the inserted or updated item.
        @note If the key is already in the map, then that key's count is incremented;
            however, @c value2 will be ignored.*/
    const_iterator insert(T1&& value1, T2&& value2)
        {
        auto [index_iter, inserted] = m_table.try_emplace(value1, std::make_pair(value2, 1));
        // if it was already there, so just update it.
        if (!inserted)
            {
            ++(index_iter->second.second);
            }
        return index_iter;
        }

    /// @returns The map of pairs and their respective frequency counts.
    [[nodiscard]]
    const map_type& get_data() const noexcept
        {
        return m_table;
        }

  private:
    map_type m_table;
    };

/** @brief Same as a @c std::map (where the key is a single value), but also
        supports multiple (unique) values connected to each key and includes
        an aggregator for each key.*/
template<typename T1, typename T2, typename Compare = std::less<T1>,
         typename CompareSecondaryValues = std::less<T2>>
class multi_value_aggregate_map
    {
  public:
    /// @private
    using values_set = std::set<T2, CompareSecondaryValues>;
    /// @private
    using values_and_aggregate_pair_type = std::pair<values_set, double>;
    /// @private
    using map_type = std::map<T1, values_and_aggregate_pair_type, Compare>;
    /// @private
    using const_iterator = typename map_type::const_iterator;
    /// @private
    using iterator = typename map_type::iterator;
    /// @private
    using value_type = std::pair<T1, values_and_aggregate_pair_type>;
    /// @private
    multi_value_aggregate_map() = default;

    /** @brief Inserts a pair of items into the map.
        @param value1 The first value of the pair.
        @param value2 The second value of the pair.
        @param aggregateValue the amount to increase the aggregated value for the item.
            Would normally be @c 1.
        @returns An iterator to the inserted or updated item.
        @note The first value is what makes the item unique.
            If a key is already in the map, then that key's count is incremented.
            If the second value isn't in the key's current values,
            then that value is added to the list of values connected to that key.*/
    const_iterator insert(const T1& value1, const T2& value2, const double aggregateValue = 1)
        {
        auto [index_iter, inserted] = m_table.try_emplace(
            value1, values_and_aggregate_pair_type(values_set({ value2 }), aggregateValue));
        // if it was already there, then just update it
        if (!inserted)
            {
            if (m_secondaryValuesMax == static_cast<size_t>(-1) ||
                index_iter->second.first.size() < m_secondaryValuesMax)
                {
                index_iter->second.first.insert(value2);
                }
            index_iter->second.second += aggregateValue;
            }
        return index_iter;
        }

    /** @brief Inserts a pair of items into the map.
        @param value1 The first value of the pair.
        @param value2 The second value of the pair.
        @param aggregateValue the amount to increase the aggregated value for the item.
            Would normally be @c 1.
        @returns An iterator to the inserted or updated item.
        @note The first value is what makes the item unique.
            If a key is already in the map, then that key's count is incremented.
            If the second value isn't in the key's current values,
            then that value is added to the list of values connected to that key.*/
    const_iterator insert(T1&& value1, T2&& value2, const double aggregateValue = 1)
        {
        auto [index_iter, inserted] = m_table.try_emplace(
            value1, values_and_aggregate_pair_type(values_set({ value2 }), aggregateValue));
        // if it was already there, then just update it
        if (!inserted)
            {
            if (m_secondaryValuesMax == static_cast<size_t>(-1) ||
                index_iter->second.first.size() < m_secondaryValuesMax)
                {
                index_iter->second.first.insert(value2);
                }
            index_iter->second.second += aggregateValue;
            }
        return index_iter;
        }

    /** @brief Insert an already constructed item with its values and counts loaded.
        @details This would normally be used if needing to update an item,
            where you would have to copy, edit, delete, and then insert the copy back in.
        @param value The value to insert.
        @returns A (const) iterator and flag indicating whether the value was inserted
            (will be @c false if value was already in the map).*/
    [[nodiscard]]
    std::pair<const_iterator, bool> insert(const value_type& value)
        {
        return m_table.try_emplace(value);
        }

    /** @brief Insert an already constructed item with its values and counts loaded.
        @details This would normally be used if needing to update an item,
            where you would have to copy, edit, delete, and then insert the copy back in.
        @param value The value to insert.
        @returns A (const) iterator and flag indicating whether the value was inserted
            (will be @c false if value was already in the map).*/
    [[nodiscard]]
    std::pair<const_iterator, bool> insert(value_type&& value)
        {
        return m_table.try_emplace(value);
        }

    /// @returns The map of pairs and their respective frequency counts.
    [[nodiscard]]
    const map_type& get_data() const noexcept
        {
        return m_table;
        }

    /// Clears the contents from the map.
    void clear() noexcept { m_table.clear(); }

    /** @brief Erases the specified iterator.
        @returns The next iterator after this iterator, or @c end() if that was the last item.
        @param position The iterator to erase.*/
    iterator erase(const_iterator position) { return m_table.erase(position); }

    /** @brief Sets the maximum number of values that each key can have.
        @details By default, there is no size limitation.
        @param size The maximum number of values that each key can have.
            The value @c -1 will allow keys to contain any number of values (the default).
        @note It is more optimal to call this prior to any calls to insert().
            Otherwise, any existing items in the map will need to have their respective
            value lists resized.*/
    void set_values_list_max_size(const size_t size)
        {
        if (size != static_cast<size_t>(-1) && get_data().size())
            {
            for (auto mapIter = m_table.begin(); mapIter != m_table.end(); ++mapIter)
                {
                if (mapIter->second.first.size() > size)
                    {
                    // trim all values beyond the maximum length
                    auto endPos = mapIter->second.first.begin();
                    std::advance(endPos, size);
                    mapIter->second.first.erase(endPos);
                    }
                }
            }
        m_secondaryValuesMax = size;
        }

  private:
    map_type m_table;
    size_t m_secondaryValuesMax{ static_cast<size_t>(-1) };
    };

/** @brief Same as a @c std::map (where the key is a single value), but also
         supports multiple (unique) values (with their own frequency counts)
         connected to each key. Includes an aggregate for each key.*/
template<typename T1, typename T2, typename Compare = std::less<T1>,
         typename CompareSecondaryValues = std::less<T2>>
class multi_value_frequency_aggregate_map
    {
  public:
    /// @private
    using values_set = frequency_set<T2, CompareSecondaryValues>;
    /// @private
    using values_and_aggregate_pair_type = std::pair<values_set, double>;
    /// @private
    using map_type = std::map<T1, values_and_aggregate_pair_type, Compare>;
    /// @private
    using const_iterator = typename map_type::const_iterator;
    /// @private
    using iterator = typename map_type::iterator;
    /// @private
    using value_type = std::pair<T1, values_and_aggregate_pair_type>;
    /// @private
    multi_value_frequency_aggregate_map() = default;

    /** @brief Inserts a pair of items into the map.
        @param value1 The first value of the pair.
        @param value2 The second value of the pair.
        @param aggregateValue the amount to increase the aggregated value for the item.
            Would normally be @c 1.
        @returns An iterator to the inserted or updated item.
        @note The first value is what makes the item unique.
            If a key is already in the map, then that key's count is incremented.
            If the second value isn't in the key's current values,
            then that value is added to the list of values connected to that key.*/
    const_iterator insert(const T1& value1, const T2& value2, const double aggregateValue = 1)
        {
        auto [index_iter, inserted] = m_table.try_emplace(
            value1, values_and_aggregate_pair_type(values_set({ value2 }), aggregateValue));
        // if it was already there, then just update it
        if (!inserted)
            {
            index_iter->second.first.insert(value2);
            index_iter->second.second += aggregateValue;
            }
        return index_iter;
        }

    /** @brief Inserts a pair of items into the map.
        @param value1 The first value of the pair.
        @param value2 The second value of the pair.
        @param aggregateValue the amount to increase the aggregated value for the item.
            Would normally be @c 1.
        @returns An iterator to the inserted or updated item.
        @note The first value is what makes the item unique.
            If a key is already in the map, then that key's count is incremented.
            If the second value isn't in the key's current values,
            then that value is added to the list of values connected to that key.*/
    const_iterator insert(T1&& value1, T2&& value2, const double aggregateValue = 1)
        {
        auto [index_iter, inserted] = m_table.try_emplace(
            value1, values_and_aggregate_pair_type(values_set({ value2 }), aggregateValue));
        // if it was already there, then just update it
        if (!inserted)
            {
            index_iter->second.first.insert(value2);
            index_iter->second.second += aggregateValue;
            }
        return index_iter;
        }

    /** @brief Insert an already constructed item with its values and counts loaded.
        @details This would normally be used if needing to update an item,
            where you would have to copy, edit, delete, and then insert the copy back in.
        @param value The value to insert.
        @returns A (const) iterator and flag indicating whether the value was inserted
            (will be @c false if value was already in the map).*/
    [[nodiscard]]
    std::pair<const_iterator, bool> insert(const value_type& value)
        {
        return m_table.try_emplace(value);
        }

    /** @brief Insert an already constructed item with its values and counts loaded.
        @details This would normally be used if needing to update an item,
            where you would have to copy, edit, delete, and then insert the copy back in.
        @param value The value to insert.
        @returns A (const) iterator and flag indicating whether the value was inserted
            (will be @c false if value was already in the map).*/
    [[nodiscard]]
    std::pair<const_iterator, bool> insert(value_type&& value)
        {
        return m_table.try_emplace(value);
        }

    /// @returns The map of pairs and their respective frequency counts.
    [[nodiscard]]
    const map_type& get_data() const noexcept
        {
        return m_table;
        }

    /// Clears the contents from the map.
    void clear() noexcept { m_table.clear(); }

    /** @brief Erases the specified iterator.
        @returns The next iterator after this iterator, or @c end() if that was the last item.
        @param position The iterator to erase.*/
    iterator erase(const_iterator position) { return m_table.erase(position); }

  private:
    map_type m_table;
    };

/** @brief Same as a @c std::map (where the key is a single value), but also
        supports multiple (unique) values (with their own frequency counts and aggregates)
        connected to each key. Includes an aggregate for each key and each subitem under each key.
    @todo Add unit test.*/
template<typename T1, typename T2, typename Compare = std::less<T1>,
         typename CompareSecondaryValues = std::less<T2>>
class multi_value_frequency_double_aggregate_map
    {
  public:
    /// @private
    using values_set = aggregate_frequency_set<T2, CompareSecondaryValues>;
    /// @private
    using values_and_aggregate_pair_type = std::pair<values_set, double>;
    /// @private
    using map_type = std::map<T1, values_and_aggregate_pair_type, Compare>;
    /// @private
    using const_iterator = typename map_type::const_iterator;
    /// @private
    using iterator = typename map_type::iterator;
    /// @private
    using value_type = std::pair<T1, values_and_aggregate_pair_type>;
    /// @private
    multi_value_frequency_double_aggregate_map() = default;

    /** @brief Inserts a pair of items into the map.
        @param value1 The first value of the pair.
        @param value2 The second value of the pair.
        @param aggregateValue1 The amount to increase the aggregated value for the key.\n
            Would normally be @c 1.
        @param aggregateValue2 The amount to increase the aggregated value for the subitem.\n
            Would normally be @c 1.
        @returns An iterator to the inserted or updated item.
        @note The first value is what makes the item unique.\n
            If a key is already in the map, then that key's count is incremented.\n
            If the second value isn't in the key's current values,
            then that value is added to the list of values connected to that key.*/
    const_iterator insert(const T1& value1, const T2& value2, const double aggregateValue1 = 1,
                          const double aggregateValue2 = 1)
        {
        auto [index_iter, inserted] = m_table.try_emplace(
            value1, values_and_aggregate_pair_type(values_set({ value2, aggregateValue2 }),
                                                   aggregateValue1));
        // if it was already there, then just update it
        if (!inserted)
            {
            index_iter->second.first.insert(value2, aggregateValue2);
            index_iter->second.second += aggregateValue1;
            }
        return index_iter;
        }

    /** @brief Inserts a pair of items into the map.
        @param value1 The first value of the pair.
        @param value2 The second value of the pair.
        @param aggregateValue1 The amount to increase the aggregated value for the key.\n
            Would normally be @c 1.
        @param aggregateValue2 The amount to increase the aggregated value for the subitem.\n
            Would normally be @c 1.
        @returns An iterator to the inserted or updated item.
        @note The first value is what makes the item unique.\n
            If a key is already in the map, then that key's count is incremented.\n
            If the second value isn't in the key's current values,
            then that value is added to the list of values connected to that key.*/
    const_iterator insert(T1&& value1, T2&& value2, const double aggregateValue1 = 1,
                          const double aggregateValue2 = 1)
        {
        auto [index_iter, inserted] = m_table.try_emplace(
            value1, values_and_aggregate_pair_type(values_set({ value2, aggregateValue2 }),
                                                   aggregateValue1));
        // if it was already there, then just update it
        if (!inserted)
            {
            index_iter->second.first.insert(value2, aggregateValue2);
            index_iter->second.second += aggregateValue1;
            }
        return index_iter;
        }

    /** @brief Insert an already constructed item with its values and counts loaded.
        @details This would normally be used if needing to update an item,
            where you would have to copy, edit, delete, and then insert the copy back in.
        @param value The value to insert.
        @returns A (const) iterator and flag indicating whether the value was inserted
            (will be @c false if value was already in the map).*/
    [[nodiscard]]
    std::pair<const_iterator, bool> insert(const value_type& value)
        {
        return m_table.try_emplace(value);
        }

    /** @brief Insert an already constructed item with its values and counts loaded.
        @details This would normally be used if needing to update an item,
            where you would have to copy, edit, delete, and then insert the copy back in.
        @param value The value to insert.
        @returns A (const) iterator and flag indicating whether the value was inserted
            (will be @c false if value was already in the map).*/
    [[nodiscard]]
    std::pair<const_iterator, bool> insert(value_type&& value)
        {
        return m_table.try_emplace(value);
        }

    /// @returns The map of pairs and their respective frequency counts.
    [[nodiscard]]
    const map_type& get_data() const noexcept
        {
        return m_table;
        }

    /// Clears the contents from the map.
    void clear() noexcept { m_table.clear(); }

    /** @brief Erases the specified iterator.
        @returns The next iterator after this iterator, or @c end() if that was the last item.
        @param position The iterator to erase.*/
    iterator erase(const_iterator position) { return m_table.erase(position); }

  private:
    map_type m_table;
    };

    /** @}*/

#endif // __FREQUENCY_MAPS_H__
