/********************************************************************************
 * Copyright (c) 2005-2026 Blake Madden
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Blake Madden - initial implementation
 ********************************************************************************/

// Include Lua while temporarily suppressing its compiler warnings

// PUSH
#if defined(__clang__)
    #pragma clang diagnostic push
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
#elif defined(_MSC_VER)
    #pragma warning(push)
#endif

// Suppress warnings
#if defined(__clang__)
    #pragma clang diagnostic ignored "-Wpedantic"
    #pragma clang diagnostic ignored "-Wgnu-label-as-value"
#elif defined(__GNUC__)
    // You need to not use -Wpedantic with GCC
    #pragma GCC diagnostic ignored "-Wpedantic"
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    #pragma GCC diagnostic ignored "-Wuninitialized"
#elif defined(_MSC_VER)
    #pragma warning(disable : 4701 4702 4310 4244 4334 4267)
#endif

// don't build the interpreter, just the library
#define MAKE_LIB
#include "../lua/onelua.c"

// POP
#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#elif defined(_MSC_VER)
    #pragma warning(pop)
#endif
