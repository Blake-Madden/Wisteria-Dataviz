///////////////////////////////////////////////////////////////////////////////
// Name:        textstream.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "textstream.h"

namespace Wisteria
    {
    //------------------------------------------------
    bool TextStream::FixBrokenUtf8Stream(wchar_t* dest, size_t destLength, const char* text,
                                         size_t length)
        {
        wxLogWarning(L"Possible broken UTF-8 stream encountered.");
        assert(text && length && dest && destLength);
        if (text == nullptr || dest == nullptr || length == 0 || destLength == 0)
            {
            return false;
            }
        // skip BOM before reading text
        if (std::memcmp(text, utf8::bom, sizeof(utf8::bom)) == 0)
            {
            text += sizeof(utf8::bom);
            length -= sizeof(utf8::bom);
            }
        const char* startOfCurrentSequence = text;
        const char* invalidSequence = text;
        while (invalidSequence)
            {
            invalidSequence = utf8::find_invalid(invalidSequence, text + length);
            // note that find_invalid returns the item at the end of the string
            // (null terminator) if it can't find a bad sequence
            if (invalidSequence == nullptr || invalidSequence[0] == 0)
                {
                break;
                }
            const size_t copiedSize =
                wxConvUTF8.ToWChar(dest, destLength, startOfCurrentSequence,
                                   (invalidSequence - startOfCurrentSequence));
            assert(copiedSize != wxCONV_FAILED);
            assert(destLength >= copiedSize);
            if (copiedSize == wxCONV_FAILED || copiedSize > destLength)
                {
                return false;
                }
            dest += copiedSize;
            destLength -= copiedSize;
            startOfCurrentSequence = ++invalidSequence;
            }
        // copy over last block
        const size_t copiedSize = wxConvUTF8.ToWChar(dest, destLength, startOfCurrentSequence,
                                                     (text + length) - startOfCurrentSequence);
        assert(copiedSize != wxCONV_FAILED);
        assert(destLength > copiedSize);
        if (copiedSize == wxCONV_FAILED || copiedSize > destLength)
            {
            return false;
            }
        dest += copiedSize;
        *dest = 0; // null terminate the string
        return true;
        }

    //------------------------------------------------
    wxString
    TextStream::CharStreamWithEmbeddedNullsToUnicode(const char* text, const size_t length,
                                                     // cppcheck-suppress passedByValue
                                                     wxString srcCharSet /*= wxEmptyString*/)
        {
        if (length == 0 || text == nullptr)
            {
            return wxEmptyString;
            }
        const char* const endSentinel = text + length;
        wxString fullString;
        while (text < endSentinel)
            {
            const size_t correctLength = string_util::strnlen(text, length);
            // if next text goes into No Man's Land then don't read it
            if (text + correctLength > endSentinel)
                {
                break;
                }
            fullString += CharStreamToUnicode(text, correctLength, srcCharSet);
            // skip any embedded NULLs to get to the next valid text block
            text += correctLength;
            while (text < endSentinel && text[0] == 0)
                {
                ++text;
                }
            }
        return fullString;
        }

    //------------------------------------------------
    bool TextStream::CharStreamToUnicode(wchar_t* dest, const size_t destLength, const char* text,
                                         const size_t length,
                                         wxString srcCharSet /*= wxEmptyString*/)
        {
        if (length == 0 || text == nullptr || destLength == 0 || dest == nullptr)
            {
            return false;
            }
        std::wmemset(dest, 0, destLength);

        size_t conversionResult = wxCONV_FAILED;
        lily_of_the_valley::unicode_extract_text convertUnicodeText;
        // if 16-bit Unicode
        if (convertUnicodeText.is_unicode(text))
            {
            convertUnicodeText(text, length, wxIsPlatformLittleEndian());
            wcsncpy_s(dest, destLength, convertUnicodeText.get_filtered_text(),
                      convertUnicodeText.get_filtered_text_length());
            return true; // already null terminated, so we're done, return from here.
            }
        // if UTF-8 (or simply 7-bit ANSI)
        else if (utf8::is_valid(text, text + length))
            {
            if (length >= 3 && utf8::starts_with_bom(text, text + length))
                {
                conversionResult =
                    wxConvUTF8.ToWChar(dest, destLength, text + sizeof(utf8::bom) /*skip BOM*/,
                                       length - sizeof(utf8::bom));
                }
            else
                {
                conversionResult = wxConvUTF8.ToWChar(dest, destLength, text, length);
                }
            // If UTF-8 conversion fails. Shouldn't happen since it was seen as valid,
            // but just in case.
            if (conversionResult == wxCONV_FAILED)
                {
                // already null terminated, so we're done, return from here.
                return FixBrokenUtf8Stream(dest, destLength, text, length);
                }
            }
        // if just plain ASCII text
        else
            {
            if (srcCharSet.empty())
                {
                // in case this is an ASCII file that incorrectly has a Windows UTF-8
                // signature in front, then chop off that signature set it to load it
                // as UTF-8 (or broken UTF-8).
                if (length > sizeof(utf8::bom) &&
                    std::memcmp(text, utf8::bom, sizeof(utf8::bom)) == 0)
                    {
                    conversionResult =
                        wxConvUTF8.ToWChar(dest, destLength, text + sizeof(utf8::bom) /*skip BOM*/,
                                           length - sizeof(utf8::bom));
                    if (conversionResult == wxCONV_FAILED)
                        {
                        return FixBrokenUtf8Stream(dest, destLength, text, length);
                        }
                    }
                // if XML or HTML, then try to read the encoding from the header
                else if ((srcCharSet =
                              lily_of_the_valley::html_extract_text::parse_charset(text, length)
                                  .c_str())
                             .length())
                    {
                    conversionResult = wxCSConv(srcCharSet).ToWChar(dest, destLength, text, length);
                    }
                // really is plain ASCII text with extended ASCII in it.
                // Just convert using current locale.
                else
                    {
                    conversionResult = wxConvCurrent->ToWChar(dest, destLength, text, length);
                    }
                // if the conversion above failed, then fallback to Windows-1252 (Western European)
                if (conversionResult == wxCONV_FAILED)
                    {
                    conversionResult =
                        wxCSConv(wxFONTENCODING_CP1252).ToWChar(dest, destLength, text, length);
                    }
                // in case our conversion failed, then this might be a broken UTF-8 stream,
                // so try to see if we can get valid text that way.
                if (conversionResult == wxCONV_FAILED)
                    {
                    return FixBrokenUtf8Stream(dest, destLength, text, length);
                    }
                }
            else
                {
                conversionResult = wxCSConv(srcCharSet).ToWChar(dest, destLength, text, length);
                // if conversion fails...
                if (conversionResult == wxCONV_FAILED)
                    {
                    // If broken UTF-8, then try to fix it
                    if (srcCharSet.CmpNoCase(_DT(L"utf-8")) == 0)
                        {
                        return FixBrokenUtf8Stream(dest, destLength, text, length);
                        }
                    // or fall back to the system default
                    else
                        {
                        conversionResult = wxConvCurrent->ToWChar(dest, destLength, text, length);
                        // if the conversion with current locale failed,
                        // then fallback to Windows-1252 (Western European)
                        if (conversionResult == wxCONV_FAILED)
                            {
                            conversionResult = wxCSConv(wxFONTENCODING_CP1252)
                                                   .ToWChar(dest, destLength, text, length);
                            }
                        }
                    }
                }
            }
        // if conversion was successful, verify that the amount written wasn't more
        // than the destination size+space for a null terminator.
        assert(((conversionResult != wxCONV_FAILED) ? (conversionResult < destLength) : true));
        // null terminate the string if we converted it successfully
        if (conversionResult != wxCONV_FAILED && conversionResult < destLength)
            {
            dest += conversionResult;
            *dest = 0;
            return true;
            }
        else
            {
            return false;
            }
        }

    //------------------------------------------------
    wxString TextStream::CharStreamToUnicode(const char* text, size_t length,
                                             // This is correct, it's an [in,out] parameter
                                             // cppcheck-suppress passedByValue
                                             wxString srcCharSet /*= wxEmptyString*/)
        {
        if (length == 0 || text == nullptr)
            {
            return wxEmptyString;
            }

        // length plus null terminator would be enough, but doesn't hurt have a little extra room
        const size_t destLength = (length * 1.5) + 1;
        auto dest = std::make_unique<wchar_t[]>(destLength);

        return (CharStreamToUnicode(dest.get(), destLength, text, length, srcCharSet)) ?
                   wxString(dest.get()) :
                   wxString{};
        }

    //------------------------------------------------
    bool TextStream::ReadFile(wxString& filePath, wxString& textBuffer,
                              const wxString& srcCharSet /*= wxEmptyString*/)
        {
        // can't do anything with an empty path, even prompting the user
        // won't make any sense
        if (filePath.empty())
            {
            return false;
            }
        while (true)
            {
            // if the file doesn't exist, then keep prompting the user for it
            // until an existing file is found or the quit
            if (!wxFile::Exists(filePath))
                {
                if (wxMessageBox(
                        wxString::Format(_(L"%s: file not found.\nDo you wish to search for it?"),
                                         filePath),
                        _(L"File Not Found"), wxYES_NO | wxICON_WARNING) == wxNO)
                    {
                    return false;
                    }
                wxFileName fn(filePath);
                wxFileDialog dialog(nullptr, _(L"Select File"), fn.GetPath(), fn.GetName(),
                                    wxFileSelectorDefaultWildcardStr,
                                    wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);

                if (dialog.ShowModal() != wxID_OK)
                    {
                    return false;
                    }

                filePath = dialog.GetPath();
                }
            else
                {
                break;
                }
            }
        MemoryMappedFile file;
        try
            {
            file.MapFile(filePath);
            textBuffer = TextStream::CharStreamToUnicode(static_cast<const char*>(file.GetStream()),
                                                         file.GetMapSize(), srcCharSet);
            if (textBuffer.empty())
                {
                // uncommon situation, but if file is nothing more than a UTF-8 BOM then it's OK
                if (file.GetMapSize() == sizeof(utf8::bom) &&
                    std::memcmp(file.GetStream(), utf8::bom, sizeof(utf8::bom)) == 0)
                    {
                    return true;
                    }
                wxMessageBox(_(L"Unable to read file."), _(L"Error"), wxOK | wxICON_EXCLAMATION);
                return false;
                }
            }
        catch (...)
            {
            wxFile theFile;
            if (!theFile.Open(filePath, wxFile::read))
                {
                wxMessageBox(_(L"Unable to open file."), _(L"Error"), wxOK | wxICON_EXCLAMATION);
                return false;
                }
            auto fileText = std::make_unique<char[]>(theFile.Length() + 1);
            if (!theFile.Read(fileText.get(), theFile.Length()))
                {
                wxMessageBox(_(L"Unable to read file."), _(L"Error"), wxOK | wxICON_EXCLAMATION);
                return false;
                }
            textBuffer =
                TextStream::CharStreamToUnicode(fileText.get(), theFile.Length(), srcCharSet);
            }
        return true;
        }
    } // namespace Wisteria
