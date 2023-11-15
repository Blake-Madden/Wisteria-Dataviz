#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../src/util/zipcatalog.h"
#include "../../src/CRCpp/inc/CRC.h"

using namespace Wisteria;

TEST_CASE("Zip Catalog", "[zip]")
    {
    const wxString appDir{ wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath() };
    ZipCatalog zc(appDir + L"/test_files/zip_data.zip");

        {
        const auto bmp = zc.ReadBitmap(L"thisisengineering-raeng-64YrPKiguAE-unsplash.jpg", wxBITMAP_TYPE_JPEG);
        CHECK(bmp.IsOk());
        }

        {
        wxMemoryOutputStream memstream;
        zc.ReadFile(L"thisisengineering-raeng-64YrPKiguAE-unsplash.jpg", memstream);
        wxStreamBuffer* theBuffer = memstream.GetOutputStreamBuffer();
        std::uint32_t crc = CRC::Calculate(theBuffer->GetBufferStart(), theBuffer->GetBufferSize(), CRC::CRC_32());

        wxFileInputStream theFile(appDir + L"/test_files/thisisengineering-raeng-64YrPKiguAE-unsplash.jpg");
        wxMemoryOutputStream memstream2;
        theFile.Read(memstream2);
        wxStreamBuffer* theBuffer2 = memstream.GetOutputStreamBuffer();
        std::uint32_t crc2 = CRC::Calculate(theBuffer2->GetBufferStart(), theBuffer2->GetBufferSize(), CRC::CRC_32());
        CHECK(crc == crc2);
        }

        {
        const auto bmp = zc.ReadSVG(L"piechart-subgrouped.svg", wxSize(64, 64));
        CHECK(bmp.IsOk());

        wxMemoryOutputStream memstream;
        zc.ReadFile(L"piechart-subgrouped.svg", memstream);
        wxStreamBuffer* theBuffer = memstream.GetOutputStreamBuffer();
        std::uint32_t crc = CRC::Calculate(theBuffer->GetBufferStart(), theBuffer->GetBufferSize(), CRC::CRC_32());
    
        wxFileInputStream theFile(appDir + L"/test_files/piechart-subgrouped.svg");
        wxMemoryOutputStream memstream2;
        theFile.Read(memstream2);
        wxStreamBuffer* theBuffer2 = memstream.GetOutputStreamBuffer();
        std::uint32_t crc2 = CRC::Calculate(theBuffer2->GetBufferStart(), theBuffer2->GetBufferSize(), CRC::CRC_32());
        CHECK(crc == crc2);
        }

        {
        const auto str = zc.ReadTextFile(L"subsettests.cpp");
        CHECK(str.length());
        std::uint32_t crcStr = CRC::Calculate(str.GetData(), str.size(), CRC::CRC_32());

        wxMemoryOutputStream memstream;
        zc.ReadFile(L"subsettests.cpp", memstream);
        wxStreamBuffer* theBuffer = memstream.GetOutputStreamBuffer();
        std::uint32_t crc = CRC::Calculate(theBuffer->GetBufferStart(), theBuffer->GetBufferSize(), CRC::CRC_32());
    
        wxFileInputStream theFile(appDir + L"/test_files/subsettests.cpp");
        wxMemoryOutputStream memstream2;
        theFile.Read(memstream2);
        wxStreamBuffer* theBuffer2 = memstream.GetOutputStreamBuffer();
        std::uint32_t crc2 = CRC::Calculate(theBuffer2->GetBufferStart(), theBuffer2->GetBufferSize(), CRC::CRC_32());
        CHECK(crc == crc2);
        CHECK(crc == crcStr);
        }

        {
        const auto str = zc.ReadTextFile(L"listctrlextests.cpp");
        CHECK(str.length());
        std::uint32_t crcStr = CRC::Calculate(str.GetData(), str.size(), CRC::CRC_32());
        CHECK(3135223649 == crcStr); // double-byte, gets converted to UTF-8

        wxMemoryOutputStream memstream;
        zc.ReadFile(L"listctrlextests.cpp", memstream);
        wxStreamBuffer* theBuffer = memstream.GetOutputStreamBuffer();
        std::uint32_t crc = CRC::Calculate(theBuffer->GetBufferStart(), theBuffer->GetBufferSize(), CRC::CRC_32());
    
        wxFileInputStream theFile(appDir + L"/test_files/listctrlextests.cpp");
        wxMemoryOutputStream memstream2;
        theFile.Read(memstream2);
        wxStreamBuffer* theBuffer2 = memstream.GetOutputStreamBuffer();
        std::uint32_t crc2 = CRC::Calculate(theBuffer2->GetBufferStart(), theBuffer2->GetBufferSize(), CRC::CRC_32());
        CHECK(crc == crc2);
        }

        {
        const auto str = zc.ReadTextFile(L"fileutiltests.cpp");
        CHECK(str.length());
        std::uint32_t crcStr = CRC::Calculate(str.GetData(), str.size(), CRC::CRC_32());

        wxMemoryOutputStream memstream;
        zc.ReadFile(L"fileutiltests.cpp", memstream);
        wxStreamBuffer* theBuffer = memstream.GetOutputStreamBuffer();
        std::uint32_t crc = CRC::Calculate(theBuffer->GetBufferStart(), theBuffer->GetBufferSize(), CRC::CRC_32());
    
        wxFileInputStream theFile(appDir + L"/test_files/fileutiltests.cpp");
        wxMemoryOutputStream memstream2;
        theFile.Read(memstream2);
        wxStreamBuffer* theBuffer2 = memstream.GetOutputStreamBuffer();
        std::uint32_t crc2 = CRC::Calculate(theBuffer2->GetBufferStart(), theBuffer2->GetBufferSize(), CRC::CRC_32());
        CHECK(crc == crc2);
        CHECK(crc == crcStr);
        }
    }
