//********************************************************
// The following code example is taken from the book
//  C++17 - The Complete Guide
//  by Nicolai M. Josuttis (www.josuttis.com)
//  http://www.cppstd17.com
//
// The code is licensed under a
//  Creative Commons Attribution 4.0 International License
//  http://creativecommons.org/licenses/by/4.0/
//********************************************************

//********************************************************
// Modified by Blake Madden, where status info is also sent
// to the debug window (under Windows). Also, emits a
// compile error if being included in a release build.

#ifndef TRACKNEW_HPP
#define TRACKNEW_HPP

#ifdef NDEBUG
#error Do not include track_new.h in a release build.
#endif

#include <new>        // for std::align_val_t
#include <cstdio>     // for printf()
#include <cstdlib>    // for malloc() and aligned_alloc()
#ifdef _MSC_VER
#include <malloc.h>   // for _aligned_malloc() and _aligned_free()
#include <strsafe.h>  // for StringCbPrintfW()
#include <debugapi.h> // for OutputDebugStringW()
#endif

class TrackNew {
 private:
  inline static int numMalloc = 0;    // num malloc calls
  inline static size_t sumSize = 0;   // bytes allocated so far
  inline static bool doTrace = false; // tracing enabled
  inline static bool inNew = false;   // don't track output inside new overloads
  inline static wchar_t messageBuffer[2048]{ 0 }; // debug window message buffer
 public:
  static void reset() {               // reset new/memory counters
    numMalloc = 0;
    sumSize = 0;
  }

  static void trace(bool b) {         // enable/disable tracing
    doTrace = b;
  }

  // implementation of tracked allocation:
  static void* allocate(std::size_t size, std::size_t align,
                        const char* call) {
    // track and trace the allocation:
    ++numMalloc;
    sumSize += size;
    void* p;
    if (align == 0) {
      p = std::malloc(size);
    }
    else {
#ifdef _MSC_VER
      p = _aligned_malloc(size, align);     // Windows API
#else
      p = std::aligned_alloc(align, size);  // C++17 API
#endif
    }
    if (doTrace) {
      // DON'T use std::cout here because it might allocate memory
      // while we are allocating memory (core dump at best)
      printf("#%d %s ", numMalloc, call);
      printf("(%zu bytes, ", size);
      if (align > 0) {
        printf("%zu-bytes aligned) ", align);
      }
      else {
        printf("def-aligned) ");
      }
      printf("=> %p (total: %zu Bytes)\n", (void*)p, sumSize);
    }
    return p;
  }

  static void status() {              // print current state
    printf("%d allocations for %zu bytes\n", numMalloc, sumSize);
#ifdef _MSC_VER
    // Send status info to debugger window also (under Windows).
    // This is useful if being ran from a GUI program
    // (where a console window for printf isn't present).
    wmemset(messageBuffer, 0, std::size(messageBuffer));
    ::StringCbPrintfW(messageBuffer, std::size(messageBuffer),
                      L"%d allocations for %llu bytes\n",
                      numMalloc, (unsigned long long)sumSize);
    ::OutputDebugStringW(messageBuffer);
#endif
  }
};

[[nodiscard]]
void* operator new (std::size_t size) {
  return TrackNew::allocate(size, 0, "::new");
}

[[nodiscard]]
void* operator new (std::size_t size, std::align_val_t align) {
  return TrackNew::allocate(size, static_cast<size_t>(align),
                            "::new aligned");
}

[[nodiscard]]
void* operator new[] (std::size_t size) {
  return TrackNew::allocate(size, 0, "::new[]");
}

[[nodiscard]]
void* operator new[] (std::size_t size, std::align_val_t align) {
  return TrackNew::allocate(size, static_cast<size_t>(align),
                            "::new[] aligned");
}

// ensure deallocations match:
void operator delete (void* p) noexcept {
  std::free(p);
}
void operator delete (void* p, std::size_t) noexcept {
  ::operator delete(p);
}
void operator delete (void* p, std::align_val_t) noexcept {
#ifdef _MSC_VER
  _aligned_free(p);  // Windows API
#else
  std::free(p);      // C++17 API
#endif
}
void operator delete (void* p, std::size_t,
                               std::align_val_t align) noexcept {
  ::operator delete(p, align);
}

#endif // TRACKNEW_HPP
