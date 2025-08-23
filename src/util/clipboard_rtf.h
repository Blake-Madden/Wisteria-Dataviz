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
    void SetText(const wxString& rtf) { m_rtf = rtf; }

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
        const wxCharBuffer cBuffer = m_rtf.mb_str();
        return cBuffer.length();
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
        const wxCharBuffer cBuffer = m_rtf.mb_str();
        std::memcpy(buf, cBuffer.data(), cBuffer.length());
        return true;
        }

  private:
    wxString m_rtf;
    };

    /** @}*/

#endif //__WX_CLIPBOARD_RTF_H__
