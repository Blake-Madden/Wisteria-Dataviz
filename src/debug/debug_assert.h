/** @addtogroup Debugging
    @brief Functions used for debugging.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef DEBUG_ASSERT_H
#define DEBUG_ASSERT_H

/**
\def NON_UNIT_TEST_ASSERT(expression)
    If unit test symbol (`__UNITTEST`) is defined, then does nothing; otherwise asserts.
    This is useful for suppressing asserts when unit testing,
    but enabling them in regular debug builds.
* @} */

//----------------------------------------------------------------------
#ifndef NON_UNIT_TEST_ASSERT
    #ifdef __UNITTEST
        #define NON_UNIT_TEST_ASSERT(x) ((void)0)
    #else
        #define NON_UNIT_TEST_ASSERT(x) assert(x)
    #endif
#endif

#endif // DEBUG_ASSERT_H
