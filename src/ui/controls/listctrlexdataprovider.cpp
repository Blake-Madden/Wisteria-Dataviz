///////////////////////////////////////////////////////////////////////////////
// Name:        listctrlexdataprovider.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "listctrlexdataprovider.h"

namespace Wisteria::UI
    {
    const wxString ListCtrlLabelManager::m_emptyCell = wxString{};
    const wxString DoubleWithTextCompare::m_emptyCell = wxString{};

    //------------------------------------------------
    int
    DoubleWithTextCompare::Compare(const ListCtrlExDataProviderBase::DoubleWithLabel& cell1,
                                   const ListCtrlExDataProviderBase::DoubleWithLabel& cell2) const
        {
        // if both items are numeric...
        if (!std::isnan(cell1.m_numericValue) && !std::isnan(cell2.m_numericValue))
            {
            if (cell1.m_numericValue == cell2.m_numericValue)
                {
                return 0;
                }
            else if (cell1.m_numericValue < cell2.m_numericValue)
                {
                return -1;
                }
            else
                {
                return 1;
                }
            }
        // or cell1 is text and cell2 is numeric (Text is bigger than numbers [unless empty])...
        else if (std::isnan(cell1.m_numericValue) && !std::isnan(cell2.m_numericValue))
            {
            if (GetLabel(cell1.m_labelCode).empty())
                {
                return -1;
                }
            else
                {
                return 1;
                }
            }
        // or cell1 is numeric and cell2 is text...
        else if (!std::isnan(cell1.m_numericValue) && std::isnan(cell2.m_numericValue))
            {
            if (GetLabel(cell2.m_labelCode).empty())
                {
                return 1;
                }
            else
                {
                return -1;
                }
            }
        // or both items are text
        else
            {
            return string_util::strnatordncasecmp(GetLabel(cell1.m_labelCode).wc_str(),
                                                  GetLabel(cell2.m_labelCode).wc_str());
            }
        }

    //------------------------------------------------
    const wxString& DoubleWithTextCompare::GetLabel(const long id) const
        {
        auto pos = m_labelsMap.find(id);
        if (pos != m_labelsMap.cend())
            {
            return pos->second->first;
            }
        else
            {
            return m_emptyCell;
            }
        }
    } // namespace Wisteria::UI
