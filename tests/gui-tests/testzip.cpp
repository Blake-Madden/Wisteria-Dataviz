#include "../../src/CRCpp/inc/CRC.h"
#include "../../src/util/zipcatalog.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <wx/bmpbndl.h>
#include <wx/image.h>
#include <wx/init.h>
#include <wx/mstream.h>
#include <wx/stdpaths.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

using namespace Wisteria;

#if wxUSE_SVG
    #include <wx/defs.h>
#endif

namespace
    {
    struct WxBoot
        {
        WxBoot() : m_ok(wxInitialize())
            {
            if (m_ok)
                {
                wxInitAllImageHandlers();
                }
            }

        ~WxBoot()
            {
            if (m_ok)
                {
                wxUninitialize();
                }
            }

        bool m_ok{ false };
        };

    static std::vector<unsigned char> ToBytes(wxMemoryOutputStream& out)
        {
        const auto* buf = out.GetOutputStreamBuffer();
        const auto* start = static_cast<const unsigned char*>(buf->GetBufferStart());
        const size_t len = out.GetLength();
        return std::vector<unsigned char>(start, start + len);
        }

    // --- UTF-8 helpers that accept both char and char8_t forms ---

    static std::vector<unsigned char> MakeUtf8(const char* s)
        {
        const auto* p = reinterpret_cast<const unsigned char*>(s);
        return std::vector<unsigned char>(p, p + std::strlen(s));
        }

    static std::vector<unsigned char> MakeUtf8(std::string_view sv)
        {
        const auto* p = reinterpret_cast<const unsigned char*>(sv.data());
        return std::vector<unsigned char>(p, p + sv.size());
        }

    static std::vector<unsigned char> MakeUtf8(const char8_t* s)
        {
        const auto* p = reinterpret_cast<const unsigned char*>(s);
        // std::char_traits<char8_t>::length is available in C++20
        const auto n = std::char_traits<char8_t>::length(s);
        return std::vector<unsigned char>(p, p + n);
        }

    static std::vector<unsigned char> MakeUtf8(std::u8string_view sv)
        {
        const auto* p = reinterpret_cast<const unsigned char*>(sv.data());
        return std::vector<unsigned char>(p, p + sv.size());
        }

    // Build an in-memory ZIP with (name, bytes) entries.
    static std::vector<unsigned char>
    MakeZip(const std::vector<std::pair<wxString, std::vector<unsigned char>>>& entries)
        {
        wxMemoryOutputStream out;
            {
            wxZipOutputStream zip(out);
            for (const auto& e : entries)
                {
                // Building the ZIP is a hard requirement for the tests using it.
                REQUIRE(zip.PutNextEntry(e.first));
                if (!e.second.empty())
                    {
                    REQUIRE(zip.WriteAll(e.second.data(), e.second.size()));
                    }
                zip.CloseEntry();
                }
            } // flush zip
        return ToBytes(out);
        }

    static std::vector<unsigned char> MakePngBytes()
        {
        wxImage img(2, 1);
        img.SetRGB(0, 0, 10, 20, 30);
        img.SetRGB(1, 0, 40, 50, 60);

        wxMemoryOutputStream pngOut;
        REQUIRE(img.SaveFile(pngOut, wxBITMAP_TYPE_PNG));
        return ToBytes(pngOut);
        }

#if wxUSE_SVG
    static std::vector<unsigned char> MakeSvgBytes()
        {
        constexpr static auto kSVG = u8R"(<?xml version="1.0" encoding="UTF-8"?>
                 <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16">
                   <rect x="0" y="0" width="16" height="16" fill="#000000"/>
                 </svg>)";
        return MakeUtf8(kSVG); // accepts char8_t*
        }
#endif
    } // namespace

TEST_CASE("ZipCatalog: basic catalog and path queries", "[ZipCatalog]")
    {
    WxBoot boot;
    REQUIRE(boot.ok);

    auto bytes = MakeZip({
        { "docs/readme.txt", MakeUtf8(u8"Hello \xF0\x9F\x8C\x8D") }, // "Hello 🌍" as UTF-8
        { "res/pixel.png", MakePngBytes() },
        { "empty.txt", {} },
        { "folder/sub/a.txt", MakeUtf8("A") },
        { "folder/b.txt", MakeUtf8("B") },
    });

    Wisteria::ZipCatalog zc(bytes.data(), bytes.size());

    SECTION("GetPaths returns all non-directory entries")
        {
        const wxArrayString paths = zc.GetPaths();
        CHECK(paths.size() == 5);
        CHECK(paths.Index("docs/readme.txt") != wxNOT_FOUND);
        CHECK(paths.Index("res/pixel.png") != wxNOT_FOUND);
        CHECK(paths.Index("empty.txt") != wxNOT_FOUND);
        CHECK(paths.Index("folder/sub/a.txt") != wxNOT_FOUND);
        CHECK(paths.Index("folder/b.txt") != wxNOT_FOUND);
        }

    SECTION("GetFilesInFolder is recursive and filters directories")
        {
        const wxArrayString docs = zc.GetFilesInFolder("docs");
        CHECK(docs.size() == 1);
        CHECK(docs[0] == "docs/readme.txt");

        const wxArrayString folderAll = zc.GetFilesInFolder("folder");
        CHECK(folderAll.Index("folder/b.txt") != wxNOT_FOUND);
        CHECK(folderAll.Index("folder/sub/a.txt") != wxNOT_FOUND);
        }
    }

TEST_CASE("ZipCatalog: ReadTextFile", "[ZipCatalog]")
    {
    WxBoot boot;
    REQUIRE(boot.ok);

    auto bytes = MakeZip({
        { "docs/readme.txt", MakeUtf8(u8"Hello \xF0\x9F\x8C\x8D") }, // "Hello 🌍"
        { "empty.txt", {} },
    });

    Wisteria::ZipCatalog zc(bytes.data(), bytes.size());

    SECTION("Existing UTF-8 file decodes to wstring")
        {
        const std::wstring w = zc.ReadTextFile("docs/readme.txt");
        CHECK_FALSE(w.empty());
        CHECK(w == L"Hello 🌍");
        }

    SECTION("Missing or zero-length returns empty")
        {
        CHECK(zc.ReadTextFile("does/not/exist.txt").empty());
        CHECK(zc.ReadTextFile("empty.txt").empty());
        }
    }

TEST_CASE("ZipCatalog: ReadBitmap (PNG)", "[ZipCatalog][Image]")
    {
    WxBoot boot;
    REQUIRE(boot.ok);

    auto bytes = MakeZip({
        { "res/pixel.png", MakePngBytes() },
    });

    Wisteria::ZipCatalog zc(bytes.data(), bytes.size());

    wxBitmap bmp = zc.ReadBitmap("res/pixel.png", wxBITMAP_TYPE_PNG);
    CHECK(bmp.IsOk());
    if (bmp.IsOk())
        {
        CHECK(bmp.GetWidth() == 2);
        CHECK(bmp.GetHeight() == 1);
        }
    }

#if wxUSE_SVG
TEST_CASE("ZipCatalog: ReadSVG", "[ZipCatalog][SVG]")
    {
    WxBoot boot;
    REQUIRE(boot.ok);

    auto bytes = MakeZip({
        { "icons/box.svg", MakeSvgBytes() },
    });

    Wisteria::ZipCatalog zc(bytes.data(), bytes.size());

    wxBitmap bmp = zc.ReadSVG("icons/box.svg", wxSize(16, 16));
    CHECK(bmp.IsOk());
    if (bmp.IsOk())
        {
        CHECK(bmp.GetWidth() == 16);
        CHECK(bmp.GetHeight() == 16);
        }
    }
#endif

TEST_CASE("ZipCatalog: ReadFile behavior and errors", "[ZipCatalog]")
    {
    WxBoot boot;
    REQUIRE(boot.ok);

    auto bytes = MakeZip({
        { "empty.txt", {} },
        { "docs/readme.txt", MakeUtf8("data") },
    });

    Wisteria::ZipCatalog zc(bytes.data(), bytes.size());

    SECTION("ReadFile returns false for missing path")
        {
        wxMemoryOutputStream sink;
        CHECK_FALSE(zc.ReadFile("nope.txt", sink));
        }

    SECTION("ReadFile returns false for zero-length compressed entry")
        {
        wxMemoryOutputStream sink;
        CHECK_FALSE(zc.ReadFile("empty.txt", sink));
        }

    SECTION("ReadFile copies bytes into provided stream")
        {
        wxMemoryOutputStream sink;
        CHECK(zc.ReadFile("docs/readme.txt", sink));

        const auto* buf = sink.GetOutputStreamBuffer();
        const auto* start = static_cast<const char*>(buf->GetBufferStart());
        const size_t len = sink.GetLength();

        CHECK(len == 4);
        if (len == 4)
            {
            CHECK(std::string(start, start + len) == "data");
            }
        }
    }

TEST_CASE("Zip Catalog", "[zip]")
    {
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };
    ZipCatalog zc(appDir + L"/test_files/zip_data.zip");

        {
        const auto bmp =
            zc.ReadBitmap(L"thisisengineering-raeng-64YrPKiguAE-unsplash.jpg", wxBITMAP_TYPE_JPEG);
        CHECK(bmp.IsOk());
        }

        {
        wxMemoryOutputStream memstream;
        zc.ReadFile(L"thisisengineering-raeng-64YrPKiguAE-unsplash.jpg", memstream);
        wxStreamBuffer* theBuffer = memstream.GetOutputStreamBuffer();
        std::uint32_t crc =
            CRC::Calculate(theBuffer->GetBufferStart(), theBuffer->GetBufferSize(), CRC::CRC_32());

        wxFileInputStream theFile(appDir +
                                  L"/test_files/thisisengineering-raeng-64YrPKiguAE-unsplash.jpg");
        wxMemoryOutputStream memstream2;
        theFile.Read(memstream2);
        wxStreamBuffer* theBuffer2 = memstream.GetOutputStreamBuffer();
        std::uint32_t crc2 = CRC::Calculate(theBuffer2->GetBufferStart(),
                                            theBuffer2->GetBufferSize(), CRC::CRC_32());
        CHECK(crc == crc2);
        }

        {
        const auto bmp = zc.ReadSVG(L"piechart-subgrouped.svg", wxSize(64, 64));
        CHECK(bmp.IsOk());

        wxMemoryOutputStream memstream;
        zc.ReadFile(L"piechart-subgrouped.svg", memstream);
        wxStreamBuffer* theBuffer = memstream.GetOutputStreamBuffer();
        std::uint32_t crc =
            CRC::Calculate(theBuffer->GetBufferStart(), theBuffer->GetBufferSize(), CRC::CRC_32());

        wxFileInputStream theFile(appDir + L"/test_files/piechart-subgrouped.svg");
        wxMemoryOutputStream memstream2;
        theFile.Read(memstream2);
        wxStreamBuffer* theBuffer2 = memstream.GetOutputStreamBuffer();
        std::uint32_t crc2 = CRC::Calculate(theBuffer2->GetBufferStart(),
                                            theBuffer2->GetBufferSize(), CRC::CRC_32());
        CHECK(crc == crc2);
        }

        {
        // convert to wxString so that a char* is implicitly sent to RC::Calculate()
        const wxString str = zc.ReadTextFile(L"subsettests.cpp");
        CHECK(str.length());
        std::uint32_t crcStr = CRC::Calculate(str.c_str(), str.length(), CRC::CRC_32());

        wxMemoryOutputStream memstream;
        zc.ReadFile(L"subsettests.cpp", memstream);
        wxStreamBuffer* theBuffer = memstream.GetOutputStreamBuffer();
        std::uint32_t crc =
            CRC::Calculate(theBuffer->GetBufferStart(), theBuffer->GetBufferSize(), CRC::CRC_32());

        wxFileInputStream theFile(appDir + L"/test_files/subsettests.cpp");
        wxMemoryOutputStream memstream2;
        theFile.Read(memstream2);
        wxStreamBuffer* theBuffer2 = memstream.GetOutputStreamBuffer();
        std::uint32_t crc2 = CRC::Calculate(theBuffer2->GetBufferStart(),
                                            theBuffer2->GetBufferSize(), CRC::CRC_32());

        CHECK(str.length() == theBuffer->GetBufferSize());
        CHECK(theBuffer->GetBufferSize() == theBuffer2->GetBufferSize());
        CHECK(crc == crc2);
        CHECK(crc == crcStr);
        }

        {
        const wxString str = zc.ReadTextFile(L"listctrlextests.cpp");
        CHECK(str.length());
        std::uint32_t crcStr = CRC::Calculate(str.c_str(), str.length(), CRC::CRC_32());
        CHECK(3135223649 == crcStr); // double-byte, gets converted to UTF-8

        wxMemoryOutputStream memstream;
        zc.ReadFile(L"listctrlextests.cpp", memstream);
        wxStreamBuffer* theBuffer = memstream.GetOutputStreamBuffer();
        std::uint32_t crc =
            CRC::Calculate(theBuffer->GetBufferStart(), theBuffer->GetBufferSize(), CRC::CRC_32());

        wxFileInputStream theFile(appDir + L"/test_files/listctrlextests.cpp");
        wxMemoryOutputStream memstream2;
        theFile.Read(memstream2);
        wxStreamBuffer* theBuffer2 = memstream.GetOutputStreamBuffer();
        std::uint32_t crc2 = CRC::Calculate(theBuffer2->GetBufferStart(),
                                            theBuffer2->GetBufferSize(), CRC::CRC_32());
        CHECK(crc == crc2);
        }

        {
        const wxString str = zc.ReadTextFile(L"fileutiltests.cpp");
        CHECK(str.length());
        std::uint32_t crcStr = CRC::Calculate(str.c_str(), str.length(), CRC::CRC_32());

        wxMemoryOutputStream memstream;
        zc.ReadFile(L"fileutiltests.cpp", memstream);
        wxStreamBuffer* theBuffer = memstream.GetOutputStreamBuffer();
        std::uint32_t crc =
            CRC::Calculate(theBuffer->GetBufferStart(), theBuffer->GetBufferSize(), CRC::CRC_32());

        wxFileInputStream theFile(appDir + L"/test_files/fileutiltests.cpp");
        wxMemoryOutputStream memstream2;
        theFile.Read(memstream2);
        wxStreamBuffer* theBuffer2 = memstream.GetOutputStreamBuffer();
        std::uint32_t crc2 = CRC::Calculate(theBuffer2->GetBufferStart(),
                                            theBuffer2->GetBufferSize(), CRC::CRC_32());
        CHECK(crc == crc2);
        CHECK(crc == crcStr);
        }
    }
