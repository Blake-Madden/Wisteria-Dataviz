# License Compatibility Check

Scan all source files for licenses incompatible with the Eclipse Foundation's licensing requirements.

## Scope

Scan ALL files including submodules. Check:
- C/C++ (.c, .cpp, .h, .hpp, .cc, .cxx, .hxx)
- C# (.cs)
- Java (.java)
- JavaScript/TypeScript (.js, .ts, .jsx, .tsx, .mjs)
- Python (.py)
- Rust (.rs)
- Go (.go)
- Fortran (.f, .f90, .f95, .for)
- Ruby (.rb)
- PHP (.php)
- Swift (.swift)
- Kotlin (.kt, .kts)
- Scala (.scala)
- Perl (.pl, .pm)
- Shell (.sh, .bash)
- PowerShell (.ps1)
- Lua (.lua)
- R (.r, .R)
- LaTeX (.tex, .sty)
- CSS/SCSS/LESS (.css, .scss, .less)
- HTML/XML (.html, .htm, .xml, .xsl)
- SQL (.sql)
- Assembly (.asm, .s)
- LICENSE, COPYING, and similar files
- README files that mention licensing

## Incompatible Licenses (Flag These)

- **GPL (any version)** - GNU General Public License
- **AGPL (any version)** - GNU Affero General Public License
- Any license with strong copyleft that requires derivative works to use the same license

## Compatible Licenses (OK)

Eclipse Foundation approved licenses (SPDX identifiers in parentheses):

- Adobe Glyph List License (Adobe-Glyph)
- Apache Software License 1.0 (Apache-1.0)
- Apache Software License 1.1 (Apache-1.1)
- Apache Software License 2.0 (Apache-2.0)
- Artistic License 2.0 (Artistic-2.0)
- Blue Oak Model License 1.0.0 (BlueOak-1.0.0)
- Boost Software License (BSL-1.0)
- BSD 2-Clause "Simplified" License (BSD-2-Clause)
- BSD 2-Clause FreeBSD License (BSD-2-Clause-FreeBSD)
- BSD 2-Clause with views sentence (BSD-2-Clause-Views)
- BSD 3-Clause "New" or "Revised" License (BSD-3-Clause)
- BSD 4-Clause "Original" or "Old" License (BSD-4-Clause)
- BSD Zero Clause License (0BSD)
- Common Development and Distribution License (CDDL) Version 1.0 (CDDL-1.0)
- Common Development and Distribution License (CDDL) Version 1.1 (CDDL-1.1)
- Common Public License Version 1.0 (CPL-1.0)
- Creative Commons Attribution 2.5 (CC-BY-2.5)
- Creative Commons Attribution 3.0 Unported (CC-BY-3.0)
- Creative Commons Attribution 4.0 International (CC-BY-4.0)
- Creative Commons Attribution Share Alike 3.0 Unported (CC-BY-SA-3.0)
- Creative Commons Attribution Share Alike 4.0 International (CC-BY-SA-4.0)
- Creative Commons Zero V1.0 Universal (CC0-1.0)
- Do What The Firetruck You Want To Public License (WTFPL)
- Eclipse Public License, v1.0 (EPL-1.0)
- Eclipse Public License, v2.0 (EPL-2.0)
- European Union Public License 1.1 (EUPL-1.1)
- European Union Public License 1.2 (EUPL-1.2)
- FreeType License (FTL)
- GNU Free Documentation License Version 1.3 (GFDL-1.3-only)
- GNU Lesser General Public License v2.0 only (LGPL-2.0-only)
- GNU Lesser General Public License v2.0 or later (LGPL-2.0-or-later)
- GNU Lesser General Public License v2.1 only (LGPL-2.1-only)
- GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
- GNU Lesser General Public License v3.0 only (LGPL-3.0-only)
- GNU Lesser General Public License v3.0 or later (LGPL-3.0-or-later)
- IBM Public License 1.0 (IPL-1.0)
- ISC License (ISC)
- MIT License (MIT)
- MIT No Attribution (MIT-0)
- Mozilla Public License Version 1.1 (MPL-1.1)
- Mozilla Public License Version 2.0 (MPL-2.0)
- NTP License (NTP)
- OpenSSL License (OpenSSL)
- PHP License v3.01 (PHP-3.01)
- PostgreSQL License (PostgreSQL)
- SIL Open Font License 1.1 (OFL-1.1)
- The Unlicense (UNLICENSE)
- Unicode License Agreement – Data Files and Software (2015) (Unicode-DFS-2015)
- Unicode License Agreement – Data Files and Software (2016) (Unicode-DFS-2016)
- Unicode Terms of Use (Unicode-TOU)
- Universal Permissive License v1.0 (UPL-1.0)
- W3C Software and Notice License (2002-12-31) (W3C)
- W3C Software Notice and Document License (2015-05-13) (W3C-20150513)
- W3C Software Notice and License (1998-07-20) (W3C-19980720)
- wxWindows Library License (wxWindows)
- X11 License (X11)
- Zlib License (Zlib)
- Zope Public License 2.1 (ZPL-2.1)

## What to Look For

- License headers at the top of source files
- SPDX-License-Identifier tags
- LICENSE, COPYING, LICENSE.txt, LICENSE.md files
- Copyright notices that reference GPL/AGPL
- References to gnu.org/licenses/gpl
- "General Public License" text (but verify - could be LGPL which is OK)

## Output

Report any files or directories with incompatible licenses:
- File/directory path
- License identified
- How it was identified (header, LICENSE file, etc.)
- Severity: INCOMPATIBLE for GPL/AGPL, WARNING for unclear licensing

If no issues found, confirm the codebase appears clean.
