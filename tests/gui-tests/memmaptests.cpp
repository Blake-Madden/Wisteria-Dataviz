#include "../../src/util/memorymappedfile.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/init.h>
#include <wx/stdpaths.h>
#include <wx/string.h>

#ifdef _WIN32
    #include <process.h>

static int getpid_compat() { return _getpid(); }
#else
    #include <unistd.h>

static int getpid_compat() { return getpid(); }
#endif

namespace
    {
    static wxString MakeTempFile()
        {
        // Creates the file and returns its path (unique per call)
        return wxFileName::CreateTempFileName(L"mmf_");
        }

    static wxString MakeTempPathOnly()
        {
        wxString dir = wxFileName::GetTempDir();
        wxString candidate;

        do
            {
            candidate =
                wxFileName(
                    dir, wxString::Format(L"mmf_missing_%d_%lld.tmp", getpid_compat(),
                                          static_cast<long long>(std::time(nullptr)) + std::rand()))
                    .GetFullPath();
            } while (wxFileName::FileExists(candidate) || wxFileName::DirExists(candidate));

        return candidate;
        }

    // Helper: unique non-ASCII path in temp dir (no file creation/deletion here)
    static wxString MakeTempNonAsciiPath()
        {
        const wxString dir = wxFileName::GetTempDir();
        wxString candidate;

        do
            {
            candidate =
                wxFileName(dir, wxString::Format(
                                    L"mmf_非ASCII_тест_résumé_雪_%d_%lld.txt", getpid_compat(),
                                    static_cast<long long>(std::time(nullptr)) + std::rand()))
                    .GetFullPath();
            } while (wxFileName::FileExists(candidate) || wxFileName::DirExists(candidate));

        return candidate;
        }

    static void WriteAllUtf8(const wxString& path, const wxString& text)
        {
        const wxString tempDir = wxFileName::GetTempDir();
        REQUIRE(wxFileName(path).GetPath().StartsWith(tempDir));

        wxFile f;
        REQUIRE(f.Create(path, true /*overwrite*/));
        const wxCharBuffer utf8 = text.utf8_str();
        REQUIRE(f.Write(utf8.data(), utf8.length()));
        }

    static wxString ReadAllUtf8(const wxString& path)
        {
        wxFile f;
        REQUIRE(f.Open(path));
        const wxFileOffset len = f.Length();
        std::string buf;
        buf.resize(static_cast<size_t>(len));
        if (len > 0)
            {
            REQUIRE(f.Read(buf.data(), static_cast<size_t>(len)) == len);
            }
        return wxString::FromUTF8(buf.data(), buf.size());
        }
    } // namespace

// ----- Tests -----

TEST_CASE("Map read-only and verify content", "[memorymap][ro]")
    {
    const wxString path = MakeTempFile();
    const wxString body = L"Hello, world!\n";
    WriteAllUtf8(path, body);

    MemoryMappedFile mmf;
    REQUIRE_NOTHROW(mmf.MapFile(path, true /*readOnly*/));

    REQUIRE(mmf.IsOk());
    REQUIRE(mmf.IsReadOnly());
    REQUIRE(mmf.GetStream() != nullptr);
    REQUIRE(mmf.GetMapSize() == body.utf8_str().length());

    const char* p = static_cast<const char*>(mmf.GetStream());
    const wxString mapped = wxString::FromUTF8(p, mmf.GetMapSize());
    CHECK(mapped == body);

    mmf.UnmapFile();
    }

TEST_CASE("Map read-write and modify persists to file", "[memorymap][rw]")
    {
    const wxString path = MakeTempFile();
    WriteAllUtf8(path, L"abcde");

    MemoryMappedFile mmf;
    REQUIRE_NOTHROW(mmf.MapFile(path, false /*readOnly*/));

    REQUIRE(mmf.IsOk());
    REQUIRE_FALSE(mmf.IsReadOnly());
    REQUIRE(mmf.GetStream() != nullptr);
    REQUIRE(mmf.GetMapSize() >= 5);

    char* p = static_cast<char*>(mmf.GetStream());
    p[0] = 'X';
    p[1] = 'Y';
    p[2] = 'Z';

    mmf.UnmapFile();

    CHECK(ReadAllUtf8(path) == L"XYZde");
    }

TEST_CASE("Unmap is safe when never mapped (idempotent)", "[memorymap][unmap]")
    {
    MemoryMappedFile mmf;
    // should do nothing harmful
    mmf.UnmapFile();
    REQUIRE_FALSE(mmf.IsOk());
    }

TEST_CASE("Nonexistent file throws mapping exception", "[memorymap][errors]")
    {
    MemoryMappedFile mmf;
    const wxString path = MakeTempPathOnly(); // ensure it doesn't exist
    REQUIRE_THROWS_AS(mmf.MapFile(path, true /*readOnly*/), MemoryMappedFileException);
    }

TEST_CASE("Zero-length file throws empty exception", "[memorymap][errors]")
    {
    const wxString path = MakeTempFile(); // already created, empty
    MemoryMappedFile mmf;
    REQUIRE_THROWS_AS(mmf.MapFile(path, true /*readOnly*/), MemoryMappedFileEmptyException);
    }

TEST_CASE("Map read-only with UTF-8 (non-ASCII) filename", "[memorymap][ro][unicode]")
    {
    const wxString path = MakeTempNonAsciiPath();
    const wxString body = L"ping 🐉 — café — Σx² = π\n";

    // Write the file with UTF-8 content
    WriteAllUtf8(path, body);

    // Map read-only
    MemoryMappedFile mmf;
    REQUIRE_NOTHROW(mmf.MapFile(path, true /*readOnly*/));

    REQUIRE(mmf.IsOk());
    REQUIRE(mmf.IsReadOnly());
    REQUIRE(mmf.GetStream() != nullptr);
    REQUIRE(mmf.GetMapSize() == body.utf8_str().length());

    // Validate mapped bytes == file contents
    const char* p = static_cast<const char*>(mmf.GetStream());
    const wxString mapped = wxString::FromUTF8(p, mmf.GetMapSize());
    CHECK(mapped == body);

    mmf.UnmapFile();
    }
