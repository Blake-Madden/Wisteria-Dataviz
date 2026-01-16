# Code Review

Review code changes for correctness, style, and project-specific issues.

## Standard Review Items

- Logic errors and bugs
- Error handling gaps
- Performance concerns
- Code clarity and maintainability
- API misuse

## wxWidgets-Specific Checks

This project uses wxWidgets trunk (latest development version). Prefer modern APIs over legacy patterns.

- Prefer `Bind()` over event tables for event handling
- Prefer `wxString::FromUTF8()` over manual conversions
- `wxT()` macro is unnecessary in modern wx - just use string literals
- Use `wxUILocale` instead of deprecated `wxLocale` where applicable
- Memory management: raw `new` for wxWidgets objects managed by parent windows, smart pointers elsewhere
- Correct use of `wxDECLARE_*` and `wxIMPLEMENT_*` macros
- Sizer usage and layout correctness
- Platform-specific code properly guarded (`__WXMSW__`, `__WXGTK__`, `__WXOSX__`)
- Flag any deprecated wxWidgets APIs - suggest modern replacements

## Internationalization (i18n) Checks

- User-visible strings wrapped with `_()` for translation
- Non-translatable strings should use `DONTTRANSLATE()` or `_DT()` (see `src/util/donttranslate.h`)
- No hardcoded user-facing text without translation macro
- Format strings using positional parameters for translator reordering
- No string concatenation that breaks translation context
- Proper use of `wxPLURAL()` for pluralized strings
- Date/number formatting using locale-aware functions

## C++ Checks

- C++20 features used appropriately
- RAII and smart pointer usage (except wxWidgets parent-managed objects)
- Const correctness
- No unnecessary copies (use references, move semantics)
- Range-based for loops where applicable
- `[[nodiscard]]` on functions where return value matters
- Avoiding raw `new`/`delete` outside wxWidgets context
- No undefined behavior (null derefs, out-of-bounds, etc.)
- Exception safety
- Proper use of `constexpr` and `noexcept` where appropriate
- Avoiding C-style casts (use `static_cast`, `dynamic_cast`, etc.)

## Project-Specific

- Doxygen comments on public APIs (`@brief`, `@param`, `@returns`)
- Consistent code style with existing codebase
- No modifications to submodules (src/easyexif, src/wxSimpleJSON, src/CRCpp, src/utfcpp, src/wxStartPage)

## Output

Present findings grouped by severity (errors, warnings, suggestions). For each issue:
- File and line number
- Description of the problem
- Suggested fix if applicable
