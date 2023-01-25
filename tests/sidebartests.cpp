#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/ui/controls/sidebar.h"

using namespace Wisteria::UI;

TEST_CASE("Sidebar control", "[sidebar]")
    {
    std::vector<wxBitmapBundle> m_imageList;
    SideBar m_sideBar = new SideBar(wxTheApp->GetTopWindow());
    m_sideBar->SetImageList(m_imageList);
    m_sideBar->SetSize(200, 400);
    m_sideBar->DeleteAllFolders();

    SECTION("Insert Item")
        {
        m_sideBar->InsertItem(0, L"first", 1, 5);
        m_sideBar->InsertItem(4, L"last", 5, 5);
        m_sideBar->InsertItem(1, L"second", 2, 5);
        CHECK(m_sideBar->GetFolderCount() == 6);
        CHECK(m_sideBar->FindFolder(1).value() == 0);
        CHECK(m_sideBar->FindFolder(2).value() == 1);
        CHECK(m_sideBar->FindFolder(5).value() == 5);
        m_sideBar->DeleteAllFolders();
        CHECK(m_sideBar->GetFolderCount() == 0);
        }
    SECTION("Insert Subitem")
        {
        m_sideBar->InsertItem(0, L"first", 1, -1);
        m_sideBar->InsertItem(1, L"second", 2, -1);
        m_sideBar->InsertItem(2, L"third", 3, -1);
        CHECK(m_sideBar->InsertSubItemById(1, L"subitem", 4, -1));
        CHECK(m_sideBar->InsertSubItemById(1, L"subitem2", 4, -1));
        CHECK(m_sideBar->InsertSubItemById(3, L"subitem2", 4, -1));
        // bad parent ID
        CHECK(m_sideBar->InsertSubItemById(4, L"subitem2", 4, -1) == false);
        }
    SECTION("Delete Item")
        {
        m_sideBar->InsertItem(0, L"first", 1, 5);
        m_sideBar->InsertItem(4, L"last", 5, 5);
        m_sideBar->InsertItem(1, L"second", 2, 5);
        CHECK(m_sideBar->GetFolderCount() == 6);
        CHECK(m_sideBar->FindFolder(1).value() == 0);
        CHECK(m_sideBar->FindFolder(2).value() == 1);
        CHECK(m_sideBar->FindFolder(5).value() == 5);
        m_sideBar->DeleteFolder(2);
        m_sideBar->DeleteFolder(2);
        m_sideBar->DeleteFolder(2);
        CHECK(m_sideBar->GetFolderCount() == 3);
        CHECK(m_sideBar->GetFolderText(0) == L"first");
        CHECK(m_sideBar->GetFolderText(1) == L"second");
        CHECK(m_sideBar->GetFolderText(2) == L"last");
        m_sideBar->DeleteAllFolders();
        CHECK(m_sideBar->GetFolderCount() == 0);
        }
    SECTION("Bad Icon Index")
        {
        m_sideBar->InsertItem(0, L"first", 1, 55);
        m_sideBar->InsertItem(1, L"second", 2, 15);
        m_sideBar->InsertItem(2, L"third", 3, 500);
        CHECK(m_sideBar->InsertSubItemById(1, L"third", 4, 458));
        // this will cause a repaint
        m_sideBar->SelectFolder(0, true);
        }
    SECTION("Get Item Text")
        {
        m_sideBar->InsertItem(0, L"first", 1, 55);
        m_sideBar->InsertItem(1, L"second", 2, 15);
        m_sideBar->InsertItem(2, L"third", 3, 500);
        CHECK(m_sideBar->GetFolderText(0) == L"first");
        CHECK(m_sideBar->GetFolderText(1) == L"second");
        CHECK(m_sideBar->GetFolderText(2) == L"third");
        // bogus index
        CHECK(m_sideBar->GetFolderText(5) == L"");
        }
    SECTION("Find Subitem")
        {
        m_sideBar->InsertItem(0, L"first", 1, -1);
        m_sideBar->InsertItem(1, L"second", 2, -1);
        m_sideBar->InsertItem(2, L"third", 3, -1);
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 4, -1));
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 5, -1));
        CHECK(m_sideBar->FindSubItem(4).first.value() == 1);
        CHECK(m_sideBar->FindSubItem(4).second.value() == 0);
        CHECK(m_sideBar->FindSubItem(5).first.value() == 1);
        CHECK(m_sideBar->FindSubItem(5).second.value() == 1);
        // bogus subitem ID
        CHECK(!m_sideBar->FindSubItem(6).first.has_value());
        CHECK(!m_sideBar->FindSubItem(6).second.has_value());
        }
    SECTION("Find Subitem With Parent Id")
        {
        m_sideBar->InsertItem(0, L"first", 1, -1);
        m_sideBar->InsertItem(1, L"second", 2, -1);
        m_sideBar->InsertItem(2, L"third", 3, -1);
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 4, -1));
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 5, -1));
        CHECK(m_sideBar->FindSubItem(2, 4).first.value() == 1);
        CHECK(m_sideBar->FindSubItem(2, 4).second.value() == 0);
        CHECK(m_sideBar->FindSubItem(2, 5).first.value() == 1);
        CHECK(m_sideBar->FindSubItem(2, 5).second.value() == 1);
        // wrong parent IDs
        CHECK(!m_sideBar->FindSubItem(1, 4).first.has_value());
        CHECK(!m_sideBar->FindSubItem(1, 4).second.has_value());
        CHECK(!m_sideBar->FindSubItem(1, 5).first.has_value());
        CHECK(!m_sideBar->FindSubItem(1, 5).second.has_value());
        // parent ID that doesn't exist
        CHECK(!m_sideBar->FindSubItem(99, 5).first.has_value());
        CHECK(!m_sideBar->FindSubItem(99, 5).second.has_value());
        }
    SECTION("Find Subitem By Name")
        {
        m_sideBar->InsertItem(0, L"first", 1, -1);
        m_sideBar->InsertItem(1, L"second", 2, -1);
        m_sideBar->InsertItem(2, L"third", 3, -1);
        // insert under the first item
        CHECK(m_sideBar->InsertSubItemById(1, L"subitem", 4, -1));
        // insert under the first item
        CHECK(m_sideBar->InsertSubItemById(1, L"subitem2", 5, -1));
        // insert under the last item
        CHECK(m_sideBar->InsertSubItemById(3, L"subitem3", 5, -1));
        CHECK(m_sideBar->FindSubItem(L"SUBITEM").first.value() == 0);
        CHECK(m_sideBar->FindSubItem(L"subitem").second.value() == 0);
        CHECK(m_sideBar->FindSubItem(L"subitem2").first.value() == 0);
        CHECK(m_sideBar->FindSubItem(L"SUBITEM2").second.value() == 1);
        CHECK(m_sideBar->FindSubItem(L"subitem3").first.value() == 2);
        CHECK(m_sideBar->FindSubItem(L"SUBITEM3").second.value() == 0);
        }
    SECTION("Select")
        {
        m_sideBar->InsertItem(0, L"first", 1, -1);
        m_sideBar->InsertItem(1, L"second", 2, -1);
        m_sideBar->InsertItem(2, L"third", 3, -1);
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 4, -1));
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 5, -1));
        m_sideBar->SelectFolder(0);
        CHECK(m_sideBar->GetSelectedFolder().value() == 0);
        CHECK(m_sideBar->GetSelectedFolderId().value() == 1);
        CHECK(m_sideBar->IsFolderSelected());
        CHECK(!m_sideBar->GetSelectedSubItemId().first.has_value());
        CHECK(!m_sideBar->GetSelectedSubItemId().second.has_value());
        // select item with subitems
        m_sideBar->SelectFolder(1);
        CHECK(m_sideBar->GetSelectedFolder().value() == 1);
        CHECK(m_sideBar->GetSelectedFolderId().value() == 2);
        CHECK(m_sideBar->IsFolderSelected());
        CHECK(m_sideBar->GetSelectedSubItemId().first.value() == 2);
        CHECK(m_sideBar->GetSelectedSubItemId().second.value() == 4);
        // select bogus item should be ignored
        m_sideBar->SelectFolder(99);
        CHECK(m_sideBar->GetSelectedFolder().value() == 1);
        CHECK(m_sideBar->GetSelectedFolderId().value() == 2);
        CHECK(m_sideBar->IsFolderSelected());
        CHECK(m_sideBar->GetSelectedSubItemId().first.value() == 2);
        CHECK(m_sideBar->GetSelectedSubItemId().second.value() == 4);
        }
    SECTION("Select Subitem")
        {
        m_sideBar->InsertItem(0, L"first", 1, -1);
        m_sideBar->InsertItem(1, L"second", 2, -1);
        m_sideBar->InsertItem(2, L"third", 3, -1);
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 4, -1));
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem2", 5, -1));
        m_sideBar->SelectSubItem(1,1);
        CHECK(m_sideBar->GetSelectedLabel() == L"subitem2");
        CHECK(m_sideBar->GetSelectedFolder().value() == 1);
        CHECK(m_sideBar->GetSelectedFolderId().value() == 2);
        CHECK(m_sideBar->IsFolderSelected());
        CHECK(m_sideBar->GetSelectedSubItemId().first.value() == 2);
        CHECK(m_sideBar->GetSelectedSubItemId().second.value() == 5);
        // selecting bad subitem will keep the previously selected subitem
        m_sideBar->SelectSubItem(1,99);
        CHECK(m_sideBar->GetSelectedLabel() == L"subitem2");
        CHECK(m_sideBar->GetSelectedFolder().value() == 1);
        CHECK(m_sideBar->GetSelectedFolderId().value() == 2);
        CHECK(m_sideBar->IsFolderSelected());
        CHECK(m_sideBar->GetSelectedSubItemId().first.value() == 2);
        CHECK(m_sideBar->GetSelectedSubItemId().second.value() == 5);
        // selecting an item with no subitems should just select that item then
        m_sideBar->SelectSubItem(0,2);
        CHECK(m_sideBar->GetSelectedLabel() == L"first");
        CHECK(m_sideBar->GetSelectedFolder().value() == 0);
        CHECK(m_sideBar->GetSelectedFolderId().value() == 1);
        CHECK(m_sideBar->IsFolderSelected());
        CHECK(!m_sideBar->GetSelectedSubItemId().first.has_value());
        CHECK(!m_sideBar->GetSelectedSubItemId().second.has_value());
        // selecting a bogus item will just be ignored
        m_sideBar->SelectSubItem(99,2);
        CHECK(m_sideBar->GetSelectedLabel() == L"first");
        CHECK(m_sideBar->GetSelectedFolder().value() == 0);
        CHECK(m_sideBar->GetSelectedFolderId().value() == 1);
        CHECK(m_sideBar->IsFolderSelected());
        CHECK(!m_sideBar->GetSelectedSubItemId().first.has_value());
        CHECK(!m_sideBar->GetSelectedSubItemId().second.has_value());
        }
    SECTION("Get Item")
        {
        m_sideBar->InsertItem(0, L"first", 1, -1);
        m_sideBar->InsertItem(1, L"second", 2, -1);
        m_sideBar->InsertItem(2, L"third", 3, -1);
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 4, -1));
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 5, -1));
        m_sideBar->SelectSubItem(1,1);
        }
    SECTION("Select Subitem By Position")
        {
        m_sideBar->InsertItem(0, L"first", 1, -1);
        m_sideBar->InsertItem(1, L"second", 2, -1);
        m_sideBar->InsertItem(2, L"third", 3, -1);
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 4, -1));
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 5, -1));
        m_sideBar->SelectAnyItem(3);
        CHECK(m_sideBar->GetSelectedFolder().value() == 1);
        CHECK(m_sideBar->GetSelectedFolderId().value() == 2);
        CHECK(m_sideBar->IsFolderSelected());
        CHECK(m_sideBar->GetSelectedSubItemId().first.value() == 2);
        CHECK(m_sideBar->GetSelectedSubItemId().second.value() == 5);
        CHECK(m_sideBar->GetSelectedAnyItem().value() == 3);
        m_sideBar->SelectAnyItem(4);
        CHECK(m_sideBar->GetSelectedFolder().value() == 2);
        CHECK(m_sideBar->GetSelectedFolderId().value() == 3);
        CHECK(m_sideBar->IsFolderSelected());
        CHECK(!m_sideBar->GetSelectedSubItemId().first.has_value());
        CHECK(!m_sideBar->GetSelectedSubItemId().second.has_value());
        CHECK(m_sideBar->GetSelectedAnyItem().value() == 4);
        m_sideBar->SelectAnyItem(0);
        CHECK(m_sideBar->GetSelectedAnyItem().value() == 0);
        }
    SECTION("Save State")
        {
        m_sideBar->InsertItem(0, L"first", 1, -1);
        m_sideBar->InsertItem(1, L"second", 2, -1);
        m_sideBar->InsertItem(2, L"third", 3, -1);
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 4, -1));
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 5, -1));
        m_sideBar->ExpandAll();
        m_sideBar->SelectSubItem(1,1);
        m_sideBar->SaveState();
        m_sideBar->DeleteAllFolders();
        m_sideBar->InsertItem(0, L"first", 1, -1);
        m_sideBar->InsertItem(1, L"second", 2, -1);
        m_sideBar->InsertItem(2, L"third", 3, -1);
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 4, -1));
        // insert under the second item
        CHECK(m_sideBar->InsertSubItemById(2, L"subitem", 5, -1));
        m_sideBar->CollapseAll();
        m_sideBar->SelectFolder(0);
        m_sideBar->ResetState();
        CHECK(m_sideBar->GetSelectedFolder().value() == 1);
        CHECK(m_sideBar->GetSelectedFolderId().value() == 2);
        CHECK(m_sideBar->IsFolderSelected());
        CHECK(m_sideBar->GetSelectedSubItemId().first.value() == 2);
        CHECK(m_sideBar->GetSelectedSubItemId().second.value() == 5);
        }

    delete m_sideBar;
    }