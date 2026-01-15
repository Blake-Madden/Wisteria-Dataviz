///////////////////////////////////////////////////////////////////////////////
// Name:        formulaformat.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "formulaformat.h"

//------------------------------------------------------------------
wxString FormulaFormat::FormatMathExpressionFromUS(const wxString& expression)
    {
    wxString formattedExpression = expression;
    for (size_t i = 0; i < formattedExpression.length(); ++i)
        {
        if (formattedExpression[i] == L',')
            {
            formattedExpression[i] = GetListSeparator();
            }
        else if (formattedExpression[i] == L'.')
            {
            // if ellipses
            if ((i + 1 < formattedExpression.length() && formattedExpression[i + 1] == L'.') ||
                (i > 0 && formattedExpression[i - 1] == L'.'))
                {
                continue;
                }
            formattedExpression[i] = wxNumberFormatter::GetDecimalSeparator();
            }
        }
    return formattedExpression;
    }

//------------------------------------------------------------------
wxString FormulaFormat::FormatMathExpressionToUS(const wxString& expression)
    {
    wxString formattedExpression = expression;
    for (size_t i = 0; i < formattedExpression.length(); ++i)
        {
        if (formattedExpression[i] == wxNumberFormatter::GetDecimalSeparator())
            {
            formattedExpression[i] = L'.';
            }
        else if (formattedExpression[i] == GetListSeparator())
            {
            formattedExpression[i] = L',';
            }
        }
    return formattedExpression;
    }
