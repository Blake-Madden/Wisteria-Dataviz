#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../src/util/fileutil.h"

TEST_CASE("Common folder", "[file]")
    {
    CHECK(GetCommonFolder(L"z:/user/bob/stuff", L"c:/user/bob/stuffing").empty());
    CHECK(GetCommonFolder(L"/stuff", L"c:/user/bob/stuffing").empty());
    CHECK(GetCommonFolder(L"", L"c:/user/bob/stuffing").empty());
    CHECK(GetCommonFolder(L"//", L"//").empty());
    CHECK(GetCommonFolder(L"stuff//", L"stuff//").empty()); // top common folder is missing
    CHECK(GetCommonFolder(L"/stuff", L"").empty());
    CHECK(GetCommonFolder(L"", L"").empty());
    CHECK(GetCommonFolder(L"/", L"/").empty());
    CHECK(GetCommonFolder(L"/", L"\\").empty());
    CHECK(GetCommonFolder(L"data\\stuff\\", L"data/stuff/").empty()); // inconsistent separators

    CHECK(GetCommonFolder(L"c:/user/bob/stuff", L"c:/user/bob/things") == wxString(L"bob"));
    CHECK(GetCommonFolder(L"c:/USER/bob/stuff", L"c:/user/bob/stuffing") == wxString(L"bob"));
    CHECK(GetCommonFolder(L"data/stuff", L"data/user/bob/stuffing") == wxString(L"data"));
    CHECK(GetCommonFolder(L"data/stuff", L"DATA/stuffing") == wxString(L"data"));
    CHECK(GetCommonFolder(L"data/stuff", L"data/stuffing") == wxString(L"data"));
    CHECK(GetCommonFolder(L"data/stuff.txt", L"data/stuff.txt") == wxString(L"data"));
    CHECK(GetCommonFolder(L"data/stuff", L"data/stuff") == wxString(L"data"));
    CHECK(GetCommonFolder(L"data/stuff/", L"data/stuff/") == wxString(L"stuff"));
    CHECK(GetCommonFolder(L"data\\stuff\\", L"data\\stuff\\") == wxString(L"stuff"));
    }

TEST_CASE("Web page extensions", "[file]")
    {
    CHECK(GetExtensionOrDomain(L"").empty());
    CHECK(GetExtensionOrDomain(L"/").empty());
    CHECK(GetExtensionOrDomain(L"//").empty());
    CHECK(GetExtensionOrDomain(L"business").empty());
    CHECK(GetExtensionOrDomain(L"/business").empty());
    CHECK(GetExtensionOrDomain(L"org").empty());

    CHECK(GetExtensionOrDomain(L"business.org") == wxString{ "org" });
    CHECK(GetExtensionOrDomain(L"/business.org") == wxString{ "org" });
    CHECK(GetExtensionOrDomain(L"www.mycompany.com/business.doc") ==
          wxString{ "doc" });
    CHECK(GetExtensionOrDomain(L"www.mycompany.com/business").empty());

    CHECK(GetExtensionOrDomain(L"www.mycompany.com/business.doc?5145") == wxString{ "doc" });
    CHECK(GetExtensionOrDomain(L"www.mycompany.com/business.js?5145") == wxString{ "js" });
    CHECK(GetExtensionOrDomain(L"www.mycompany.com/business.js?id=501") == wxString{ "js" });
    CHECK(GetExtensionOrDomain(L"www.mycompany.com/business.js?id=501;l=en") == wxString{ "js" });
    CHECK(GetExtensionOrDomain(L"www.mycompany.com/business.js.php?id=501;l=en") == wxString{ "php" });
    CHECK(GetExtensionOrDomain(L"www.mycompany.com/js?5") == wxString{ "js" });
    CHECK(GetExtensionOrDomain(L"www.mycompany.com/css?en") == wxString{ "css" });
    }

TEST_CASE("Parse title from file name", "[parse-title]")
    {
    SECTION("Local File")
        {
        CHECK(ParseTitleFromFileName(L"c:\\files\\file.txt") == wxString("file"));
        CHECK(ParseTitleFromFileName(L"/users/files/file.txt") == wxString("file"));
        }
    SECTION("Local File Illegal Chars")
        {
        CHECK(ParseTitleFromFileName(L"c:\\files\\fi?l*e.txt") == wxString("file"));
        CHECK(ParseTitleFromFileName(L"/users/files/fi?l*e.txt") == wxString("file"));
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
