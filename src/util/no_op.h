/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __NO_OP_H__
#define __NO_OP_H__

/** @brief A non-operational functor.*/
class no_op_functor
    {
public:
    /// @brief No-op function.
    inline void operator()() const noexcept
        {}
    /// @brief No-op function.
    template <typename T>
    inline void operator()(T&) const noexcept
        {}
    /// @brief No-op function.
    template <typename T>
    inline void operator()(const T&) const noexcept
        {}
    };

#endif //__NO_OP_H__
