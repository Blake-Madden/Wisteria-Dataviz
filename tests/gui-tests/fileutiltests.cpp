#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../src/util/fileutil.h"

TEST_CASE("Parse title from file name", "[parse-title]")
    {
    SECTION("Local File")
        {
        CHECK(ParseTitleFromFileName(L"c:\\files\\file.txt") == wxT("file"));
        CHECK(ParseTitleFromFileName(L"/users/files/file.txt") == wxT("file"));
        }
    SECTION("Local File Illegal Chars")
        {
        CHECK(ParseTitleFromFileName(L"c:\\files\\fi?l*e.txt") == wxT("file"));
        CHECK(ParseTitleFromFileName(L"/users/files/fi?l*e.txt") == wxT("file"));
        }
    SECTION("Url Ending With Slash")
        {
        CHECK(ParseTitleFromFileName(L"http://money.cnn.com/2011/08/18/news/economy/bachmann_gas_prices/") == L"bachmann_gas_prices");
        }
    SECTION("Url Query")
        {
        CHECK(ParseTitleFromFileName(L"http://money.cnn.com/2011/08/18/news/economy/bachmann_gas_prices/?iref=NS1") == L"bachmann_gas_prices");
        }
    }

TEST_CASE("Path resolver", "[pathresolve]")
    {
    SECTION("Null")
        {
        FilePathResolverBase pathResolve(L"");
        CHECK(pathResolve.IsInvalidFile());
        CHECK(pathResolve.GetResolvedPath().empty());
        }

    SECTION("Excell Cell")
        {
        FilePathResolverBase pathResolve;

        pathResolve.ResolvePath(L"C:\\Testing\\Text\\data.xlsx#Sheet1#A4");
        CHECK(pathResolve.IsExcelCell());

        pathResolve.ResolvePath(L"C:\\TESTING\\TEXT\\DATA.XLSX#SHEET1#A4");
        CHECK(pathResolve.IsExcelCell());

        pathResolve.ResolvePath(L"C:\\Testing\\Text\\data.xlsx#");
        CHECK(pathResolve.IsExcelCell());

        // with extra dot
        pathResolve.ResolvePath(L"C:\\Testing\\Text\\data.Xlsx#Sheet.1#A4");
        CHECK(pathResolve.IsExcelCell());

        // not much of a path, but OK. Tests boundary issues
        pathResolve.ResolvePath(L"/data.xlsx#");
        CHECK(pathResolve.IsExcelCell());

        // a local file that we can open normally, and an Excel file
        pathResolve.ResolvePath(L"C:\\Testing\\Text\\data.xlsx");
        CHECK(pathResolve.IsLocalOrNetworkFile());
        CHECK(pathResolve.IsSpreadsheet(wxFileName(L"C:\\Testing\\Text\\data.xlsx")));
        }

    SECTION("Archive File")
        {
        FilePathResolverBase pathResolve;

        pathResolve.ResolvePath(L"C:\\Testing\\Text\\data.zip#file.txt");
        CHECK(pathResolve.IsArchivedFile());

        pathResolve.ResolvePath(L"C:\\TESTING\\TEXT\\DATA.ZIP#SUBFOLDER/ANOTHER_FOLDER/FILE.TXT");
        CHECK(pathResolve.IsArchivedFile());

        pathResolve.ResolvePath(L"C:\\Testing\\Text\\data.zip#");
        CHECK(pathResolve.IsArchivedFile());

        // not much of a path, but OK. Tests boundary issues
        pathResolve.ResolvePath(L"/data.zip#");
        CHECK(pathResolve.IsArchivedFile());

        // a local file that we can open normally, and an archive file
        pathResolve.ResolvePath(L"C:\\Testing\\Text\\data.zip");
        CHECK(pathResolve.IsLocalOrNetworkFile());
        CHECK(pathResolve.IsArchive(wxFileName(L"C:\\Testing\\Text\\data.zip")));
        }

    SECTION("Local With File Protocol")
        {
        FilePathResolverBase pathResolve;

        pathResolve.ResolvePath(L"file://localhost/C:\\Testing\\Text\\file.txt");
        CHECK(pathResolve.IsLocalOrNetworkFile());
        CHECK(pathResolve.GetResolvedPath() == L"C:\\Testing\\Text\\file.txt");

        pathResolve.ResolvePath(L"file:///C:\\Testing\\Text\\file.txt");
        CHECK(pathResolve.IsLocalOrNetworkFile());
        CHECK(pathResolve.GetResolvedPath() == L"C:\\Testing\\Text\\file.txt");
        }

    SECTION("Resetting")
        {
        FilePathResolverBase pathResolve;

        pathResolve.ResolvePath(L"file://localhost/C:\\Testing\\Text\\file.txt");
        CHECK(pathResolve.IsLocalOrNetworkFile());
        CHECK(pathResolve.GetResolvedPath() == L"C:\\Testing\\Text\\file.txt");

        pathResolve.ResolvePath(L"");
        CHECK(pathResolve.IsInvalidFile());
        CHECK(pathResolve.GetResolvedPath().empty());

        pathResolve.ResolvePath(L"C:\\Testing\\Text\\file.txt");
        CHECK(pathResolve.IsLocalOrNetworkFile());
        CHECK(pathResolve.GetResolvedPath() == L"C:\\Testing\\Text\\file.txt");
        }

    SECTION("Web Paths")
        {
        FilePathResolverBase pathResolve;
        // fixes bad slashes and encodes spaces
        pathResolve.ResolvePath(L" https:\\\\www.acme.com\\about us info.html  ");
        CHECK(pathResolve.IsWebFile());
        CHECK(pathResolve.GetFileType() == FilePathType::HTTPS);
        CHECK(pathResolve.GetResolvedPath() == L"https://www.acme.com/about%20us%20info.html");

        pathResolve.ResolvePath(L"http:\\\\www.acme.com\\about us info.html");
        CHECK(pathResolve.IsWebFile());
        CHECK(pathResolve.GetFileType() == FilePathType::HTTP);
        CHECK(pathResolve.GetResolvedPath() == L"http://www.acme.com/about%20us%20info.html");

        pathResolve.ResolvePath(L"www.acme.com\\about us info.html");
        CHECK(pathResolve.IsWebFile());
        CHECK(pathResolve.GetFileType() == FilePathType::HTTP);// safe assumption to fallback to
        CHECK(pathResolve.GetResolvedPath() == L"http://www.acme.com/about%20us%20info.html");

        pathResolve.ResolvePath(L"ftp:\\\\acme.com\\dataset.zip");
        CHECK(pathResolve.IsWebFile());
        CHECK(pathResolve.GetFileType() == FilePathType::FTP);
        CHECK(pathResolve.GetResolvedPath() == L"ftp://acme.com/dataset.zip");

        pathResolve.ResolvePath(L"gopher:\\\\acme.com\\dataset.zip");
        CHECK(pathResolve.IsWebFile());
        CHECK(pathResolve.GetFileType() == FilePathType::Gopher);
        CHECK(pathResolve.GetResolvedPath() == L"gopher://acme.com/dataset.zip");
        }

    SECTION("Not Paths")
        {
        FilePathResolverBase pathResolve;

        pathResolve.ResolvePath(L"Hello, world.");
        CHECK(pathResolve.IsInvalidFile());

        pathResolve.ResolvePath(L"Hello, world.A");
        CHECK(pathResolve.IsInvalidFile());

        pathResolve.ResolvePath(L"Some really long text that I am typing that clearly is not a filepath.");
        CHECK(pathResolve.IsInvalidFile());
        }

    SECTION("Local Or Network Path")
        {
        FilePathResolverBase pathResolve;

        pathResolve.ResolvePath(L"C:\\Testing\\Text\\file.txt");
        CHECK(pathResolve.IsLocalOrNetworkFile());
        CHECK(pathResolve.GetResolvedPath() == L"C:\\Testing\\Text\\file.txt");

        // different drive
        pathResolve.ResolvePath(L"Z:\\Testing\\Text\\file.txt");
        CHECK(pathResolve.IsLocalOrNetworkFile());
        CHECK(pathResolve.GetResolvedPath() == L"Z:\\Testing\\Text\\file.txt");
    
        // bad slash, should still work
        pathResolve.ResolvePath(L"Z:/Testing\\Text/file.txt");
        CHECK(pathResolve.IsLocalOrNetworkFile());
        CHECK(pathResolve.GetResolvedPath() == L"Z:/Testing\\Text/file.txt");

        // missing slash
        pathResolve.ResolvePath(L"Z:Testing\\Text\\file.txt");
        CHECK(pathResolve.IsInvalidFile());

        // just a drive letter, not a file path
        pathResolve.ResolvePath(L"c:\\");
        CHECK(pathResolve.IsLocalOrNetworkFile());

        // UNIX path should be OK
        pathResolve.ResolvePath(L"/test.txt");
        CHECK(pathResolve.IsLocalOrNetworkFile());

        // UNC
        pathResolve.ResolvePath(L"\\\\nt-server\\test.txt");
        CHECK(pathResolve.IsLocalOrNetworkFile());

        // Bad UNC
        pathResolve.ResolvePath(L"\\nt-server\\test.txt");
        CHECK(pathResolve.IsInvalidFile());
        }
    }
