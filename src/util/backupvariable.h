/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef BACKUP_VARIABLE_H
#define BACKUP_VARIABLE_H

/// @brief Class that remembers its original value from construction.
template<typename T>
class BackupVariable
    {
  public:
    /// @brief Constructor.
    /// @param value The starting value.
    explicit BackupVariable(const T& value) : m_originalValue(value), m_value(value) {}

    /// @private
    BackupVariable() = delete;

    /// @private
    BackupVariable& operator=(const T& value) noexcept
        {
        m_value = value;
        return *this;
        }

    /// @private
    [[nodiscard]]
    bool operator==(const T& value) const noexcept
        {
        return m_value == value;
        }

    /// @private
    [[nodiscard]]
    bool operator<(const T& value) const noexcept
        {
        return m_value < value;
        }

    /// @private
    [[nodiscard]]
    bool operator<=(const T& value) const noexcept
        {
        return m_value <= value;
        }

    /// @private
    [[nodiscard]]
    bool operator>(const T& value) const noexcept
        {
        return m_value > value;
        }

    /// @private
    [[nodiscard]]
    bool operator>=(const T& value) const noexcept
        {
        return m_value >= value;
        }

    /// @private
    [[nodiscard]]
    explicit operator T() const noexcept
        {
        return m_value;
        }

    /// @returns The current value.
    [[nodiscard]]
    const T& get_value() const noexcept
        {
        return m_value;
        }

    /// @private
    [[nodiscard]]
    T& get_value() noexcept
        {
        return m_value;
        }

    /// @returns @c true if the value has changed.
    [[nodiscard]]
    bool has_changed() const noexcept
        {
        return m_value != m_originalValue;
        }

  private:
    T m_originalValue;
    T m_value;
    };

    /** @}*/

#endif // BACKUP_VARIABLE_H
