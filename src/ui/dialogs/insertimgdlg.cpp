///////////////////////////////////////////////////////////////////////////////
// Name:        insertimgdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertimgdlg.h"
#include "../../app/wisteriaapp.h"
#include <wx/tokenzr.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertImageDlg::InsertImageDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                   wxWindow* parent, const wxString& caption, const wxWindowID id,
                                   const wxPoint& pos, const wxSize& size, const long style,
                                   EditMode editMode, const ImageDlgOptions options)
        : InsertItemDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode),
          m_options(options)
        {
        CreateControls();
        FinalizeControls();
        TransferDataToWindow();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertImageDlg::CreateControls()
        {
        InsertItemDlg::CreateControls();

        auto* imagePage = new wxPanel(GetSideBarBook());
        auto* imageSizer = new wxBoxSizer(wxVERTICAL);
        imagePage->SetSizer(imageSizer);
        GetSideBarBook()->AddPage(imagePage, _(L"Image"), ID_IMAGE_SECTION, true);

        if ((m_options & ImageDlgIncludePageOptions) == 0)
            {
            GetSideBarBook()->DeletePage(0);
            }

        // image file paths
        m_pathListBox = new wxEditableListBox(
            imagePage, wxID_ANY, _(L"Image files:"), wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(120) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        imageSizer->Add(m_pathListBox, wxSizerFlags{ 1 }.Expand().Border());

        // change New button icon to image icon
        m_pathListBox->GetNewButton()->SetBitmapLabel(
            wxGetApp().ReadSvgIcon(L"image.svg", wxSize{ 16, 16 }));

        // override New to open a multi-select file dialog
        m_pathListBox->GetNewButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]] wxCommandEvent&)
            {
                wxFileDialog fileDlg(this, _(L"Select images"), wxString{}, wxString{},
                                     Wisteria::GraphItems::Image::GetImageFileFilter(),
                                     wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
                if (fileDlg.ShowModal() != wxID_OK)
                    {
                    return;
                    }
                wxArrayString newPaths;
                fileDlg.GetPaths(newPaths);

                wxArrayString existing;
                m_pathListBox->GetStrings(existing);
                // remove trailing empty strings left by the control
                while (!existing.empty() && existing.Last().empty())
                    {
                    existing.RemoveAt(existing.GetCount() - 1);
                    }
                for (const auto& path : newPaths)
                    {
                    existing.Add(path);
                    }
                m_pathListBox->SetStrings(existing);
            });

        // override Edit to browse for a replacement file
        m_pathListBox->GetEditButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]] wxCommandEvent&)
            {
                const auto sel = m_pathListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                           wxLIST_STATE_SELECTED);
                if (sel == wxNOT_FOUND)
                    {
                    return;
                    }
                const auto currentPath = m_pathListBox->GetListCtrl()->GetItemText(sel);
                wxFileDialog fileDlg(this, _(L"Select an image"), wxString{}, currentPath,
                                     Wisteria::GraphItems::Image::GetImageFileFilter(),
                                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
                if (fileDlg.ShowModal() != wxID_OK)
                    {
                    return;
                    }
                m_pathListBox->GetListCtrl()->SetItemText(sel, fileDlg.GetPath());
            });

        // stitch direction
        if ((m_options & ImageDlgIncludeStitch) != 0)
            {
            auto* stitchGrid = new wxFlexGridSizer(2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2,
                                                              wxSizerFlags::GetDefaultBorder() });
            stitchGrid->Add(new wxStaticText(imagePage, wxID_ANY, _(L"Stitch direction:")),
                            wxSizerFlags{}.CenterVertical());
            m_stitchChoice = new wxChoice(imagePage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                                          nullptr, 0, wxGenericValidator(&m_stitchDirection));
            m_stitchChoice->Append(_(L"Horizontal"));
            m_stitchChoice->Append(_(L"Vertical"));
            stitchGrid->Add(m_stitchChoice);
            imageSizer->Add(stitchGrid, wxSizerFlags{}.Border());
            }

        // size options
        if ((m_options & ImageDlgIncludeSize) != 0)
            {
            auto* sizeBox = new wxStaticBoxSizer(wxVERTICAL, imagePage, _(L"Size"));

            auto* customSizeCheck = new wxCheckBox(
                sizeBox->GetStaticBox(), wxID_ANY, _(L"Override default size"), wxDefaultPosition,
                wxDefaultSize, 0, wxGenericValidator(&m_customSize));
            sizeBox->Add(customSizeCheck, wxSizerFlags{}.Border());

            auto* sizeGrid = new wxFlexGridSizer(2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2,
                                                            wxSizerFlags::GetDefaultBorder() });

            sizeGrid->Add(new wxStaticText(sizeBox->GetStaticBox(), wxID_ANY, _(L"Width:")),
                          wxSizerFlags{}.CenterVertical());
            m_widthSpin = new wxSpinCtrl(sizeBox->GetStaticBox(), wxID_ANY);
            m_widthSpin->SetRange(1, 10'000);
            m_widthSpin->SetValue(512);
            sizeGrid->Add(m_widthSpin);

            sizeGrid->Add(new wxStaticText(sizeBox->GetStaticBox(), wxID_ANY, _(L"Height:")),
                          wxSizerFlags{}.CenterVertical());
            m_heightSpin = new wxSpinCtrl(sizeBox->GetStaticBox(), wxID_ANY);
            m_heightSpin->SetRange(1, 10'000);
            m_heightSpin->SetValue(512);
            sizeGrid->Add(m_heightSpin);

            sizeBox->Add(sizeGrid, wxSizerFlags{}.Border());
            imageSizer->Add(sizeBox, wxSizerFlags{}.Expand().Border());

            // size spins start disabled
            OnEnableCustomSize(false);

            customSizeCheck->Bind(wxEVT_CHECKBOX,
                                  [this]([[maybe_unused]] wxCommandEvent&)
                                  {
                                      TransferDataFromWindow();
                                      OnEnableCustomSize(m_customSize);
                                  });
            }

        // resize method and effect
        auto* optionsGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        if ((m_options & ImageDlgIncludeResizeMethod) != 0)
            {
            optionsGrid->Add(new wxStaticText(imagePage, wxID_ANY, _(L"Resize method:")),
                             wxSizerFlags{}.CenterVertical());
            auto* resizeChoice = new wxChoice(imagePage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                              0, nullptr, 0, wxGenericValidator(&m_resizeMethod));
            resizeChoice->Append(_(L"Downscale or upscale"));
            resizeChoice->Append(_(L"Downscale only"));
            resizeChoice->Append(_(L"Upscale only"));
            resizeChoice->Append(_(L"No resize"));
            optionsGrid->Add(resizeChoice);
            }

        if ((m_options & ImageDlgIncludeEffect) != 0)
            {
            optionsGrid->Add(new wxStaticText(imagePage, wxID_ANY, _(L"Effect:")),
                             wxSizerFlags{}.CenterVertical());
            auto* effectChoice = new wxChoice(imagePage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                              0, nullptr, 0, wxGenericValidator(&m_imageEffect));
            effectChoice->Append(_(L"None"));
            effectChoice->Append(_(L"Grayscale"));
            effectChoice->Append(_(L"Blur horizontal"));
            effectChoice->Append(_(L"Blur vertical"));
            effectChoice->Append(_(L"Sepia"));
            effectChoice->Append(_(L"Frosted glass"));
            effectChoice->Append(_(L"Oil painting"));
            optionsGrid->Add(effectChoice);
            }

        imageSizer->Add(optionsGrid, wxSizerFlags{}.Border());

        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertImageDlg::OnEnableCustomSize(const bool enable) const
        {
        if (m_widthSpin != nullptr)
            {
            m_widthSpin->Enable(enable);
            }
        if (m_heightSpin != nullptr)
            {
            m_heightSpin->Enable(enable);
            }
        }

    //-------------------------------------------
    void InsertImageDlg::SetImagePaths(const wxArrayString& paths) const
        {
        if (m_pathListBox != nullptr)
            {
            m_pathListBox->SetStrings(paths);
            }
        }

    //-------------------------------------------
    void InsertImageDlg::SetCustomSize(const bool enable, const int width, const int height)
        {
        m_customSize = enable;
        if (m_widthSpin != nullptr)
            {
            m_widthSpin->SetValue(width);
            }
        if (m_heightSpin != nullptr)
            {
            m_heightSpin->SetValue(height);
            }
        OnEnableCustomSize(enable);
        }

    //-------------------------------------------
    wxArrayString InsertImageDlg::GetImagePaths() const
        {
        wxArrayString paths;
        if (m_pathListBox != nullptr)
            {
            m_pathListBox->GetStrings(paths);
            // wxEditableListBox may include trailing empty strings
            while (!paths.empty() && paths.Last().empty())
                {
                paths.RemoveAt(paths.GetCount() - 1);
                }
            }
        return paths;
        }

    //-------------------------------------------
    bool InsertImageDlg::Validate()
        {
        const auto paths = GetImagePaths();

        if (paths.empty())
            {
            wxMessageBox(_(L"Please add at least one image file."), _(L"Image Required"),
                         wxOK | wxICON_WARNING, this);
            m_pathListBox->SetFocus();
            return false;
            }

        for (const auto& path : paths)
            {
            // only validate absolute paths; relative paths are resolved
            // against the project directory at load time
            if (wxFileName{ path }.IsAbsolute() && !wxFileExists(path))
                {
                wxMessageBox(wxString::Format(_(L"The image file \"%s\" does not exist."), path),
                             _(L"File Not Found"), wxOK | wxICON_WARNING, this);
                m_pathListBox->SetFocus();
                return false;
                }
            }

        return true;
        }

    //-------------------------------------------
    int InsertImageDlg::GetImageWidth() const
        {
        return m_widthSpin != nullptr ? m_widthSpin->GetValue() : 512;
        }

    //-------------------------------------------
    int InsertImageDlg::GetImageHeight() const
        {
        return m_heightSpin != nullptr ? m_heightSpin->GetValue() : 512;
        }

    //-------------------------------------------
    void InsertImageDlg::LoadFromImage(const Wisteria::GraphItems::Image& image)
        {
        if ((m_options & ImageDlgIncludePageOptions) != 0)
            {
            LoadPageOptions(image);
            }

        // file paths from property templates
        if (m_pathListBox != nullptr)
            {
            wxArrayString paths;
            const auto pathsTemplate = image.GetPropertyTemplate(L"image-import.paths");
            if (!pathsTemplate.empty())
                {
                // paths stored as tab-separated string
                wxStringTokenizer tokenizer(pathsTemplate, L"\t");
                while (tokenizer.HasMoreTokens())
                    {
                    paths.Add(tokenizer.GetNextToken());
                    }
                }
            else
                {
                const auto pathTemplate = image.GetPropertyTemplate(L"image-import.path");
                if (!pathTemplate.empty())
                    {
                    paths.Add(pathTemplate);
                    }
                }
            m_pathListBox->SetStrings(paths);
            }

        // stitch direction from property template
        const auto stitchStr = image.GetPropertyTemplate(L"image-import.stitch");
        if (stitchStr.CmpNoCase(L"vertical") == 0)
            {
            m_stitchDirection = 1;
            }
        else
            {
            m_stitchDirection = 0;
            }

        // size from property templates
        const auto widthStr = image.GetPropertyTemplate(L"size.width");
        const auto heightStr = image.GetPropertyTemplate(L"size.height");
        if (!widthStr.empty() || !heightStr.empty())
            {
            m_customSize = true;
            if (m_widthSpin != nullptr && !widthStr.empty())
                {
                m_widthSpin->SetValue(wxAtoi(widthStr));
                }
            if (m_heightSpin != nullptr && !heightStr.empty())
                {
                m_heightSpin->SetValue(wxAtoi(heightStr));
                }
            }

        OnEnableCustomSize(m_customSize);

        // resize method
        m_resizeMethod = static_cast<int>(image.GetResizeMethod());

        // effect from property template
        const auto effectStr = image.GetPropertyTemplate(L"image-import.effect");
        if (!effectStr.empty())
            {
            const auto effect = ReportEnumConvert::ConvertImageEffect(effectStr);
            if (effect.has_value())
                {
                m_imageEffect = static_cast<int>(effect.value());
                }
            }

        TransferDataToWindow();
        }

    //-------------------------------------------
    void InsertImageDlg::ApplyToImage(Wisteria::GraphItems::Image& image) const
        {
        image.SetResizeMethod(GetResizeMethod());
        }
    } // namespace Wisteria::UI
