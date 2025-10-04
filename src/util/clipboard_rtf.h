/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WX_CLIPBOARD_RTF_H__
#define __WX_CLIPBOARD_RTF_H__

#include <cstring>
#include <memory>
#include <wx/dataobj.h>
#include <wx/string.h>

/** @brief A specialization of @c wxDataObjectSimple for Rich Text Formatted text.
    @details It can be used to paste data into the wxClipboard
        or a @c wxDropSource.*/
class wxRtfDataObject : public wxDataObjectSimple
    {
  public:
    /** @brief Constructor. May be used to initialize the text
            (otherwise, SetText() should be used later).
        @param rtf The RTF data.*/
    explicit wxRtfDataObject(wxString rtf = wxString{})
        :
#ifdef __WXMSW__
          wxDataObjectSimple(L"Rich Text Format"),
#elif defined __WXOSX__
          wxDataObjectSimple(L"public.rtf"),
#else
          wxDataObjectSimple(L"text/rtf"),
#endif
          m_rtf(std::move(rtf))
        {
        }

    /** @brief Sets the (Rich Text Formatted) text.
        @param rtf The RTF data.*/
    void SetText(const wxString& rtf)
        {
        wxASSERT_MSG(rtf.IsAscii(), L"RTF content must be 7-bit ASCII!");
        m_rtf = rtf;
        }

    /** @param format The data's format (not used).
        @returns The size of the RTF data in bytes. */
    [[nodiscard]]
    size_t GetDataSize([[maybe_unused]] const wxDataFormat& format) const final
        {
        return GetDataSize();
        }

    /** @returns The size of the RTF data in bytes. */
    [[nodiscard]]
    size_t GetDataSize() const final
        {
        wxASSERT_MSG(m_rtf.IsAscii(), L"RTF content must be 7-bit ASCII!");
        const wxScopedCharBuffer utf8 = m_rtf.utf8_str();
        return utf8.length();
        }

    /** @brief Copy the data to the buffer.
        @param format The data's format (not used).
        @param[out] buf The data buffer to write to.
        @returns @c true on success. */
    [[nodiscard]]
    bool GetDataHere([[maybe_unused]] const wxDataFormat& format, void* buf) const final
        {
        return GetDataHere(buf);
        }

    /** @brief Copy the data to the buffer.
        @param[out] buf The data buffer to write to.
        @returns @c true on success.*/
    [[nodiscard]]
    bool GetDataHere(void* buf) const final
        {
        if (buf == nullptr)
            {
            return false;
            }
        const wxScopedCharBuffer utf8 = m_rtf.utf8_str();
        std::memcpy(buf, utf8.data(), utf8.length());
        return true;
        }

    /// @brief Receives RTF from clipboard/drop.
    /// @param format Not used.
    /// @param len The length of the content.
    /// @param buf The buffer.
    /// @returns @c true if successful.
    bool SetData([[maybe_unused]] const wxDataFormat& format, size_t len, const void* buf) final
        {
        if (buf == nullptr || len == 0)
            {
            m_rtf.clear();
            return false;
            }
        // RTF bytes should be 7-bit; interpret as UTF-8 best-effort for storage though.
        m_rtf = wxString::FromUTF8(static_cast<const char*>(buf), len);
        wxASSERT_MSG(m_rtf.IsAscii(), L"RTF content must be 7-bit ASCII!");
        return true;
        }

    /// @returns The RTF text.
    [[nodiscard]]
    const wxString& GetText() const
        {
        return m_rtf;
        }

  private:
    wxString m_rtf;
    };

    /** @}*/

#endif //__WX_CLIPBOARD_RTF_H__
