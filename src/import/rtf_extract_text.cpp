///////////////////////////////////////////////////////////////////////////////
// Name:        rtf_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "rtf_extract_text.h"

namespace lily_of_the_valley
    {
    rtf_to_text_symbol_table::rtf_to_text_symbol_table()
        {
        // keyword     dflt    fPassDflt   kwd         idx         printString
        m_symbols = {
            rtf_symbol("b", 1, false, KWD::kwdBold, static_cast<int>(IPROP::ipropBold), L""),
            rtf_symbol("ul", 1, false, KWD::kwdUnderline, static_cast<int>(IPROP::ipropUnderline), L""),
            rtf_symbol("ulnone", 1, false, KWD::kwdUnderline, static_cast<int>(IPROP::ipropUnderline), L""),
            rtf_symbol("i", 1, false, KWD::kwdItalic, static_cast<int>(IPROP::ipropItalic), L""),
            rtf_symbol("strike", 1, false, KWD::kwdStrikeThrough, static_cast<int>(IPROP::ipropStrikeThrough), L""),
            rtf_symbol("strikedl", 1, false, KWD::kwdStrikeThrough, static_cast<int>(IPROP::ipropStrikeThrough), L""),
            rtf_symbol("li", 0, false, KWD::kwdProp, static_cast<int>(IPROP::ipropLeftInd), L""),
            rtf_symbol("ri", 0, false, KWD::kwdProp, static_cast<int>(IPROP::ipropRightInd), L""),
            rtf_symbol("fi", 0, false, KWD::kwdProp, static_cast<int>(IPROP::ipropFirstInd), L""),
            rtf_symbol("cols", 1, false, KWD::kwdProp, static_cast<int>(IPROP::ipropCols), L""),
            rtf_symbol("sbknone", static_cast<int>(SBK::sbkNon), true, KWD::kwdProp, static_cast<int>(IPROP::ipropSbk), L""),
            rtf_symbol("sbkcol", static_cast<int>(SBK::sbkCol), true, KWD::kwdProp, static_cast<int>(IPROP::ipropSbk), L""),
            rtf_symbol("sbkeven", static_cast<int>(SBK::sbkEvn), true, KWD::kwdProp, static_cast<int>(IPROP::ipropSbk), L""),
            rtf_symbol("sbkodd", static_cast<int>(SBK::sbkOdd), true, KWD::kwdProp, static_cast<int>(IPROP::ipropSbk), L""),
            rtf_symbol("sbkpage", static_cast<int>(SBK::sbkPg), true, KWD::kwdProp, static_cast<int>(IPROP::ipropSbk), L""),
            rtf_symbol("pgnx", 0, false, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnX), L""),
            rtf_symbol("pgny", 0, false, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnY), L""),
            rtf_symbol("pgndec", static_cast<int>(PGN::pgDec), true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnFormat), L""),
            rtf_symbol("pgnucrm", static_cast<int>(PGN::pgURom), true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnFormat), L""),
            rtf_symbol("pgnlcrm", static_cast<int>(PGN::pgLRom), true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnFormat), L""),
            rtf_symbol("pgnucltr", static_cast<int>(PGN::pgULtr), true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnFormat), L""),
            rtf_symbol("pgnlcltr", static_cast<int>(PGN::pgLLtr), true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnFormat), L""),
            // for right justification and centered tab, stick a tab in front of it to emulate the formatting
            rtf_symbol("qc", 0, false, KWD::kwdChar, 0x09, L""),
            rtf_symbol("ql", static_cast<int>(JUST::justL), true, KWD::kwdProp, static_cast<int>(IPROP::ipropJust), L""),
            rtf_symbol("qr", 0, false, KWD::kwdChar, 0x09, L""),
            rtf_symbol("qj", static_cast<int>(JUST::justF), true, KWD::kwdProp, static_cast<int>(IPROP::ipropJust), L""),
            rtf_symbol("paperw", 12240, false, KWD::kwdProp, static_cast<int>(IPROP::ipropXaPage), L""),
            rtf_symbol("paperh", 15480, false, KWD::kwdProp, static_cast<int>(IPROP::ipropYaPage), L""),
            rtf_symbol("margl", 1800, false, KWD::kwdProp, static_cast<int>(IPROP::ipropXaLeft), L""),
            rtf_symbol("margr", 1800, false, KWD::kwdProp, static_cast<int>(IPROP::ipropXaRight), L""),
            rtf_symbol("margt", 1440, false, KWD::kwdProp, static_cast<int>(IPROP::ipropYaTop), L""),
            rtf_symbol("margb", 1440, false, KWD::kwdProp, static_cast<int>(IPROP::ipropYaBottom), L""),
            rtf_symbol("pgnstart", 1, true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnStart), L""),
            rtf_symbol("facingp", 1, true, KWD::kwdProp, static_cast<int>(IPROP::ipropFacingp), L""),
            rtf_symbol("landscape",1, true, KWD::kwdProp, static_cast<int>(IPROP::ipropLandscape), L""),
            rtf_symbol("par", 0, false, KWD::kwdChar, 0x0a, L""),
            rtf_symbol("pard", 0, false, KWD::kwdChar, 0x0a, L""),
            rtf_symbol("\n", 0, false, KWD::kwdChar, 0x0a, L""),
            rtf_symbol("\r", 0, false, KWD::kwdChar, 0x0a, L""),
            rtf_symbol("tab", 0, false, KWD::kwdChar, 0x09, L""),
            rtf_symbol("ldblquote",0, false, KWD::kwdChar, 0x201C, L""),
            rtf_symbol("rdblquote",0, false, KWD::kwdChar, 0x201D, L""),
            rtf_symbol("lquote", 0, false, KWD::kwdChar, 0x2018, L""),
            rtf_symbol("rquote", 0, false, KWD::kwdChar, 0x2019, L""),
            rtf_symbol("bin", 0, false, KWD::kwdSpec, static_cast<int>(IPFN::ipfnBin), L""),
            rtf_symbol("*", 0, false, KWD::kwdSpec, static_cast<int>(IPFN::ipfnSkipDest), L""),
            rtf_symbol("'", 0, false, KWD::kwdSpec, static_cast<int>(IPFN::ipfnHex), L""),
            rtf_symbol("author", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("buptim", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("colortbl", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("comment", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("creatim", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("doccomm", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("fonttbl", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("footer", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("footerf", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("footerl", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("footerr", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("ftncn", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("ftnsep", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("ftnsepc", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("header", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("headerf", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("headerl", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("headerr", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("info", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("keywords", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("operator", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("pict", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("printim", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("private1", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("revtim", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("rxe", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("stylesheet", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("subject", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("tc", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("title", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("txe", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("xe", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("leveltext", 0, false, KWD::kwdSectionSkip, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("{", 0, false, KWD::kwdChar, '{', L""),
            rtf_symbol("}", 0, false, KWD::kwdChar, '}', L""),
            rtf_symbol("\\", 0, false, KWD::kwdChar, '\\', L""),
            rtf_symbol("footnote", 0, false, KWD::kwdChar, 0x0a, L""),
            rtf_symbol("line", 0, false, KWD::kwdChar, 0x0a, L""),
            rtf_symbol("sect", 0, false, KWD::kwdChar, 0x0a, L""),
            rtf_symbol("page", 0, false, KWD::kwdChar, 0x0C, L""),
            rtf_symbol("pagebb", 0, false, KWD::kwdChar, 0x0C, L""),
            rtf_symbol("bullet", 0, false, KWD::kwdChar, 0x95, L""),
            rtf_symbol("emdash", 0, false, KWD::kwdChar, 0x2014, L""),
            rtf_symbol("endash", 0, false, KWD::kwdChar, 0x2013, L""),
            // escapes
            rtf_symbol("~", 0, false, KWD::kwdChar, ' ', L" "),
            rtf_symbol("_", 0, false, KWD::kwdChar, '-', L" "),
            //table commands
            rtf_symbol("column", 0, false, KWD::kwdChar, 0x09, L""),
            rtf_symbol("cell", 0, false, KWD::kwdChar, 0x09, L""),
            rtf_symbol("nestcell", 0, false, KWD::kwdChar, 0x09, L""),
            rtf_symbol("row", 0, false, KWD::kwdChar, 0x0a, L""),
            rtf_symbol("nestrow", 0, false, KWD::kwdChar, 0x0a, L"") };
        }

    //------------------------------------------------
    rtf_to_html_symbol_table::rtf_to_html_symbol_table()
        {
        // keyword     dflt    fPassDflt   kwd         idx         printString
        m_symbols = {
            rtf_symbol("b", 1, false, KWD::kwdBold, static_cast<int>(IPROP::ipropBold), L""),
            rtf_symbol("ul", 1, false, KWD::kwdUnderline, static_cast<int>(IPROP::ipropUnderline), L""),
            rtf_symbol("ulnone", 1, false, KWD::kwdUnderline, static_cast<int>(IPROP::ipropUnderline), L""),
            rtf_symbol("i", 1, false, KWD::kwdItalic, static_cast<int>(IPROP::ipropItalic), L""),
            rtf_symbol("strike", 1, false, KWD::kwdStrikeThrough, static_cast<int>(IPROP::ipropStrikeThrough), L""),
            rtf_symbol("strikedl", 1, false, KWD::kwdStrikeThrough, static_cast<int>(IPROP::ipropStrikeThrough), L""),
            rtf_symbol("li", 0, false, KWD::kwdProp, static_cast<int>(IPROP::ipropLeftInd), L""),
            rtf_symbol("ri", 0, false, KWD::kwdProp, static_cast<int>(IPROP::ipropRightInd), L""),
            rtf_symbol("fi", 0, false, KWD::kwdProp, static_cast<int>(IPROP::ipropFirstInd), L""),
            rtf_symbol("cols", 1, false, KWD::kwdProp, static_cast<int>(IPROP::ipropCols), L""),
            rtf_symbol("sbknone", static_cast<int>(SBK::sbkNon), true, KWD::kwdProp, static_cast<int>(IPROP::ipropSbk), L""),
            rtf_symbol("sbkcol", static_cast<int>(SBK::sbkCol), true, KWD::kwdProp, static_cast<int>(IPROP::ipropSbk), L""),
            rtf_symbol("sbkeven", static_cast<int>(SBK::sbkEvn), true, KWD::kwdProp, static_cast<int>(IPROP::ipropSbk), L""),
            rtf_symbol("sbkodd", static_cast<int>(SBK::sbkOdd), true, KWD::kwdProp, static_cast<int>(IPROP::ipropSbk), L""),
            rtf_symbol("sbkpage", static_cast<int>(SBK::sbkPg), true, KWD::kwdProp, static_cast<int>(IPROP::ipropSbk), L""),
            rtf_symbol("pgnx", 0, false, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnX), L""),
            rtf_symbol("pgny", 0, false, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnY), L""),
            rtf_symbol("pgndec", static_cast<int>(PGN::pgDec), true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnFormat), L""),
            rtf_symbol("pgnucrm", static_cast<int>(PGN::pgURom), true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnFormat), L""),
            rtf_symbol("pgnlcrm", static_cast<int>(PGN::pgLRom), true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnFormat), L""),
            rtf_symbol("pgnucltr", static_cast<int>(PGN::pgULtr), true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnFormat), L""),
            rtf_symbol("pgnlcltr", static_cast<int>(PGN::pgLLtr), true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnFormat), L""),
            rtf_symbol("qc", static_cast<int>(JUST::justC), true, KWD::kwdProp, static_cast<int>(IPROP::ipropJust), L""),
            rtf_symbol("ql", static_cast<int>(JUST::justL), true, KWD::kwdProp, static_cast<int>(IPROP::ipropJust), L""),
            rtf_symbol("qr", static_cast<int>(JUST::justR), true, KWD::kwdProp, static_cast<int>(IPROP::ipropJust), L""),
            rtf_symbol("qj", static_cast<int>(JUST::justF), true, KWD::kwdProp, static_cast<int>(IPROP::ipropJust), L""),
            rtf_symbol("paperw", 12240, false, KWD::kwdProp, static_cast<int>(IPROP::ipropXaPage), L""),
            rtf_symbol("paperh", 15480, false, KWD::kwdProp, static_cast<int>(IPROP::ipropYaPage), L""),
            rtf_symbol("margl", 1800, false, KWD::kwdProp, static_cast<int>(IPROP::ipropXaLeft), L""),
            rtf_symbol("margr", 1800, false, KWD::kwdProp, static_cast<int>(IPROP::ipropXaRight), L""),
            rtf_symbol("margt", 1440, false, KWD::kwdProp, static_cast<int>(IPROP::ipropYaTop), L""),
            rtf_symbol("margb", 1440, false, KWD::kwdProp, static_cast<int>(IPROP::ipropYaBottom), L""),
            rtf_symbol("pgnstart", 1, true, KWD::kwdProp, static_cast<int>(IPROP::ipropPgnStart), L""),
            rtf_symbol("facingp", 1, true, KWD::kwdProp, static_cast<int>(IPROP::ipropFacingp), L""),
            rtf_symbol("landscape",1, true, KWD::kwdProp, static_cast<int>(IPROP::ipropLandscape), L""),
            rtf_symbol("par", 0, false, KWD::kwdString, 0x0a, L"<br />\n"),
            rtf_symbol("pard", 0, false, KWD::kwdString, 0x0a, L"<br />\n"),
            rtf_symbol("\n", 0, false, KWD::kwdString, 0x0a, L"<br />\n"),
            rtf_symbol("\r", 0, false, KWD::kwdString, 0x0a, L"<br />\n"),
            rtf_symbol("tab", 0, false, KWD::kwdString, 0x09, L"&nbsp;&nbsp;"),
            rtf_symbol("ldblquote",0, false, KWD::kwdString, '\"', L"&#8220;"),
            rtf_symbol("rdblquote",0, false, KWD::kwdString, '\"', L"&#8221;"),
            rtf_symbol("lquote", 0, false, KWD::kwdString, '\'', L"&#8216;"),
            rtf_symbol("rquote", 0, false, KWD::kwdString, '\'', L"&#8217;"),
            rtf_symbol("bin", 0, false, KWD::kwdSpec, static_cast<int>(IPFN::ipfnBin), L""),
            rtf_symbol("*", 0, false, KWD::kwdSpec, static_cast<int>(IPFN::ipfnSkipDest), L""),
            rtf_symbol("'", 0, false, KWD::kwdSpec, static_cast<int>(IPFN::ipfnHex), L""),
            rtf_symbol("author", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("buptim", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("colortbl", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("comment", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("creatim", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("doccomm", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("fonttbl", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("footer", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("footerf", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("footerl", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("footerr", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("ftncn", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("ftnsep", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("ftnsepc", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("header", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("headerf", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("headerl", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("headerr", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("info", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("keywords", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("operator", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("pict", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("printim", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("private1", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("revtim", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("rxe", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("stylesheet", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("subject", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("tc", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("title", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("txe", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("xe", 0, false, KWD::kwdDest, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("leveltext", 0, false, KWD::kwdSectionSkip, static_cast<int>(IDEST::idestSkip), L""),
            rtf_symbol("{", 0, false, KWD::kwdChar, '{', L""),
            rtf_symbol("}", 0, false, KWD::kwdChar, '}', L""),
            rtf_symbol("\\", 0, false, KWD::kwdChar, '\\', L""),
            rtf_symbol("footnote", 0, false, KWD::kwdString, 0x0a, L"<br />\n"),
            rtf_symbol("line", 0, false, KWD::kwdString, 0x0a, L"<br />\n"),
            rtf_symbol("sect", 0, false, KWD::kwdChar, 0x0a, L""),
            rtf_symbol("page", 0, false, KWD::kwdChar, 0x0C, L""),
            rtf_symbol("pagebb", 0, false, KWD::kwdChar, 0x0C, L""),
            rtf_symbol("bullet", 0, false, KWD::kwdChar, 0x95, L""),
            rtf_symbol("emdash", 0, false, KWD::kwdString, 0x2014, L"&mdash;"),
            rtf_symbol("endash", 0, false, KWD::kwdString, 0x2013, L"&ndash;"),
            rtf_symbol("highlight", 0, false, KWD::kwdHighlight, 0, L""),
            rtf_symbol("cb", 0, false, KWD::kwdHighlight, 0, L""),
            rtf_symbol("cf", 0, false, KWD::kwdFontColor, 0, L""),
            // escapes
            rtf_symbol("~", 0, false, KWD::kwdChar, ' ', L" "),
            rtf_symbol("_", 0, false, KWD::kwdChar, '-', L" "),
            // table commands
            rtf_symbol("column", 0, false, KWD::kwdChar, 0x09, L""),
            rtf_symbol("cell", 0, false, KWD::kwdChar, 0x09, L""),
            rtf_symbol("nestcell", 0, false, KWD::kwdChar, 0x09, L""),
            rtf_symbol("row", 0, false, KWD::kwdChar, 0x0a, L""),
            rtf_symbol("nestrow", 0, false, KWD::kwdChar, 0x0a, L"") };
        }

    /// @private
    const rtf_to_text_symbol_table rtf_extract_text::RTF_TO_TEXT_TABLE;
    /// @private
    const rtf_to_html_symbol_table rtf_extract_text::RTF_TO_HTML_TABLE;

    /// Property descriptions.
    /// @private
    constexpr propmod rgprop[static_cast<int>(IPROP::ipropMax)] =
        {
        {ACTN::actnByte,   PROPTYPE::propChp,    offsetof(char_prop, fBold)},       // ipropBold
        {ACTN::actnByte,   PROPTYPE::propChp,    offsetof(char_prop, fItalic)},     // ipropItalic
        {ACTN::actnByte,   PROPTYPE::propChp,    offsetof(char_prop, fUnderline)},  // ipropUnderline
        {ACTN::actnByte,   PROPTYPE::propChp,    offsetof(char_prop, fStrikeThrough)},  // ipropStrikeThrough
        {ACTN::actnWord,   PROPTYPE::propPap,    offsetof(para_prop, xaLeft)},      // ipropLeftInd
        {ACTN::actnWord,   PROPTYPE::propPap,    offsetof(para_prop, xaRight)},     // ipropRightInd
        {ACTN::actnWord,   PROPTYPE::propPap,    offsetof(para_prop, xaFirst)},     // ipropFirstInd
        {ACTN::actnWord,   PROPTYPE::propSep,    offsetof(SEP, cCols)},       // ipropCols
        {ACTN::actnWord,   PROPTYPE::propSep,    offsetof(SEP, xaPgn)},       // ipropPgnX
        {ACTN::actnWord,   PROPTYPE::propSep,    offsetof(SEP, yaPgn)},       // ipropPgnY
        {ACTN::actnWord,   PROPTYPE::propDop,    offsetof(DOP, xaPage)},      // ipropXaPage
        {ACTN::actnWord,   PROPTYPE::propDop,    offsetof(DOP, yaPage)},      // ipropYaPage
        {ACTN::actnWord,   PROPTYPE::propDop,    offsetof(DOP, xaLeft)},      // ipropXaLeft
        {ACTN::actnWord,   PROPTYPE::propDop,    offsetof(DOP, xaRight)},     // ipropXaRight
        {ACTN::actnWord,   PROPTYPE::propDop,    offsetof(DOP, yaTop)},       // ipropYaTop
        {ACTN::actnWord,   PROPTYPE::propDop,    offsetof(DOP, yaBottom)},    // ipropYaBottom
        {ACTN::actnWord,   PROPTYPE::propDop,    offsetof(DOP, pgnStart)},    // ipropPgnStart
        {ACTN::actnByte,   PROPTYPE::propSep,    offsetof(SEP, sbk)},         // ipropSbk
        {ACTN::actnByte,   PROPTYPE::propSep,    offsetof(SEP, pgnFormat)},   // ipropPgnFormat
        {ACTN::actnByte,   PROPTYPE::propDop,    offsetof(DOP, fFacingp)},    // ipropFacingp
        {ACTN::actnByte,   PROPTYPE::propDop,    offsetof(DOP, fLandscape)},  // ipropLandscape
        {ACTN::actnByte,   PROPTYPE::propPap,    offsetof(para_prop, just)},  // ipropJust
        {ACTN::actnSpec,   PROPTYPE::propPap,    0},                          // ipropPard
        {ACTN::actnSpec,   PROPTYPE::propChp,    0},                          // ipropPlain
        {ACTN::actnSpec,   PROPTYPE::propSep,    0}                           // ipropSectd
        };

    //------------------------------------------------
    rtf_extract_text::rtf_extract_text(
        const rtf_extraction_type& extraction_type /*= rtf_extraction_type::rtf_to_text*/) noexcept :
        m_extraction_type(extraction_type),
        m_ris(RIS::risNorm), m_rds(RDS::rdsNorm),
        m_cGroup(0), m_psave(nullptr),
        m_fSkipDestIfUnk(0), m_cbBin(0), m_lParam(0),
        m_rtf_text(nullptr), m_paragraphCount(0), m_font_size(12),
        m_keyword_command_table(nullptr), m_in_bullet_state(false)
        {
        reset_property(m_chp);
        reset_property(m_pap);
        reset_property(m_sep);
        reset_property(m_dop);
        if (extraction_type == rtf_extraction_type::rtf_to_text)
            { m_keyword_command_table = &RTF_TO_TEXT_TABLE; }
        else
            { m_keyword_command_table = &RTF_TO_HTML_TABLE; }
        }

    //------------------------------------------------
    const wchar_t* rtf_extract_text::operator()(const char* text, const size_t text_length)
        {
        clear_log();
        reset_meta_data();
        m_paragraphCount = 0;
        m_in_bullet_state = false;
        if (text == nullptr || text_length == 0)
            {
            set_filtered_text_length(0);
            return nullptr;
            }
        const char* const endSentinel = text + text_length;
        // read the metadata
            {
            rtf_extract_text parseRtfMetaData;
            const char* infoSection = std::strstr(text, "{\\info");
            if (infoSection)
                {
                ++infoSection;
                const char* const infoSectionEnd =
                    string_util::find_unescaped_matching_close_tag(infoSection, '{', '}');
                if (infoSectionEnd)
                    {
                    try
                        {
                        // title
                        const char* titleSection = std::strstr(infoSection, "{\\title");
                        if (titleSection && titleSection+7 < infoSectionEnd)
                            {
                            titleSection += 7;
                            const char* const titleSectionEnd =
                                string_util::find_unescaped_char(titleSection, '}');
                            if (titleSectionEnd && titleSectionEnd < infoSectionEnd)
                                {
                                auto metaValue = parseRtfMetaData(titleSection,
                                                                  titleSectionEnd-titleSection);
                                if (metaValue)
                                    {
                                    m_title.assign(metaValue);
                                    string_util::replace_all(m_title, L"\\", 1, L"");
                                    string_util::trim(m_title);
                                    }
                                }
                            }
                        // subject
                        const char* subjectSection = std::strstr(infoSection, "{\\subject");
                        if (subjectSection && subjectSection+9 < infoSectionEnd)
                            {
                            subjectSection += 9;
                            const char* const subjectSectionEnd =
                                string_util::find_unescaped_char(subjectSection, '}');
                            if (subjectSectionEnd && subjectSectionEnd < infoSectionEnd)
                                {
                                auto metaValue = parseRtfMetaData(subjectSection,
                                                                  subjectSectionEnd-subjectSection);
                                if (metaValue)
                                    {
                                    m_subject.assign(metaValue);
                                    string_util::replace_all(m_subject, L"\\", 1, L"");
                                    string_util::trim(m_subject);
                                    }
                                }
                            }
                        // author
                        const char* authorSection = std::strstr(infoSection, "{\\author");
                        if (authorSection && authorSection+8 < infoSectionEnd)
                            {
                            authorSection += 8;
                            const char* const authorSectionEnd =
                                string_util::find_unescaped_char(authorSection, '}');
                            if (authorSectionEnd && authorSectionEnd < infoSectionEnd)
                                {
                                auto metaValue = parseRtfMetaData(authorSection,
                                                                  authorSectionEnd-authorSection);
                                if (metaValue)
                                    {
                                    m_author.assign(metaValue);
                                    string_util::replace_all(m_author, L"\\", 1, L"");
                                    string_util::trim(m_author);
                                    }
                                }
                            }
                        // keywords
                        const char* keywordsSection = std::strstr(infoSection, "{\\keywords");
                        if (keywordsSection && keywordsSection+10 < infoSectionEnd)
                            {
                            keywordsSection += 10;
                            const char* const keywordsSectionEnd =
                                string_util::find_unescaped_char(keywordsSection, '}');
                            if (keywordsSectionEnd && keywordsSectionEnd < infoSectionEnd)
                                {
                                auto metaValue = parseRtfMetaData(keywordsSection,
                                                                  keywordsSectionEnd-keywordsSection);
                                if (metaValue)
                                    {
                                    m_keywords.assign(metaValue);
                                    string_util::replace_all(m_keywords, L"\\", 1, L"");
                                    string_util::trim(m_keywords);
                                    }
                                }
                            }
                        // comments
                        const char* commentsSection = std::strstr(infoSection, "{\\doccomm");
                        if (commentsSection && commentsSection+9 < infoSectionEnd)
                            {
                            commentsSection += 9;
                            const char* const commentsSectionEnd =
                                string_util::find_unescaped_char(commentsSection, '}');
                            if (commentsSectionEnd && commentsSectionEnd < infoSectionEnd)
                                {
                                auto metaValue = parseRtfMetaData(commentsSection,
                                                                  commentsSectionEnd-commentsSection);
                                if (metaValue)
                                    {
                                    m_comments.assign(metaValue);
                                    string_util::replace_all(m_comments, L"\\", 1, L"");
                                    string_util::trim(m_comments);
                                    }
                                }
                            }
                        }
                    catch (...)
                        { log_message(L"Error reading metadata from RTF."); }
                    }
                }
            }
        if (m_extraction_type == rtf_extraction_type::rtf_to_html)
            {
            // load the font table
            m_font_table.clear();
            const char* fontTable = std::strstr(text, "{\\fonttbl");
            if (fontTable)
                {
                ++fontTable;
                const char* fontTableEnd =
                    string_util::find_unescaped_matching_close_tag(fontTable, '{', '}');
                if (fontTableEnd)
                    {
                    const char* currentChar = std::strchr(fontTable, '{');
                    while (currentChar && currentChar < fontTableEnd)
                        {
                        // see where this current font ends
                        const char* endOfFont = std::strchr(currentChar, ';');
                        if (!endOfFont || endOfFont >= fontTableEnd)
                            { break; }
                        std::string fontText(currentChar, endOfFont-currentChar);
                        // the face name is at the end after the charset, so substring that out
                        size_t lastSection = fontText.rfind('\\');
                        if (lastSection == std::string::npos)
                            { break; }
                        lastSection = fontText.find(' ', lastSection);
                        if (lastSection == std::string::npos)
                            { break; }
 
                        m_font_table.push_back(fontText.substr(++lastSection));

                        currentChar = std::strchr(endOfFont, '{');
                        }
                    }
                }
            // load the color table
            m_color_table.clear();
            const char* colorTable = std::strstr(text, "{\\colortbl");
            if (colorTable)
                {
                ++colorTable;
                const char* colorTableEnd =
                    string_util::find_unescaped_matching_close_tag(colorTable, '{', '}');
                if (colorTableEnd)
                    {
                    rtf_color color;
                    const char* currentChar = std::strchr(colorTable, ';');
                    while (currentChar && currentChar < colorTableEnd)
                        {
                        // red component
                        currentChar = std::strstr(currentChar, "red");
                        if (!currentChar || currentChar >= colorTableEnd)
                            { break; }
                        color.red = std::atoi(currentChar+3);
                        // green component
                        currentChar = std::strstr(currentChar, "green");
                        if (!currentChar || currentChar >= colorTableEnd)
                            { break; }
                        color.green = std::atoi(currentChar+5);
                        // blue component
                        currentChar = std::strstr(currentChar, "blue");
                        if (!currentChar || currentChar >= colorTableEnd)
                            { break; }
                        color.blue = std::atoi(currentChar+4);
                        // format the color for the HTML converter
                        /// @todo Use new format library from C++20 when the time comes
                        std::wstringstream webColorFormatStream;
                        webColorFormatStream << std::uppercase << std::setfill(L'0') << std::hex << std::setw(2)
                            << color.red
                            << std::uppercase << std::setfill(L'0') << std::hex << std::setw(2)
                            << color.green
                            << std::uppercase << std::setfill(L'0') << std::hex << std::setw(2)
                            << color.blue;
                        color.web_color = webColorFormatStream.str();
                        m_color_table.push_back(color);

                        currentChar = std::strchr(currentChar, ';');
                        }
                    }
                }
            // format style section for background and foreground colors
            // (note this is pre-filled with default white background and black foreground styles).
            m_style_section += L"." + m_style_prefix + L"bc0 {background-color:#FFFFFF;}\n." +
                               m_style_prefix + L"fc0 {color:#000000;}";
            for (auto colorPos = m_color_table.cbegin();
                 colorPos != m_color_table.cend(); ++colorPos)
                {
                m_style_section += L"\n." + m_style_prefix + L"bc" +
                    std::to_wstring((colorPos-m_color_table.begin())+1) +
                    L" {background-color:#" + colorPos->web_color + L";}\n" +
                    L"." + m_style_prefix + L"fc" +
                    std::to_wstring((colorPos-m_color_table.begin())+1) + L" {color:#" +
                    colorPos->web_color + L";}";
                }
            // find the first paragraph color and we will just use this for the rest of the document
            const char* textColor = std::strstr(text, "\\par");
            if (textColor)
                {
                const char* nextSpace = std::strchr(textColor, L' ');
                textColor = std::strstr(textColor, "\\cf");
                if (textColor && nextSpace && (textColor < nextSpace))
                    {
                    const int idx = std::atoi(textColor+3);
                    // color table is one-base indexed
                    if (idx > 0 && idx <= static_cast<int>(m_color_table.size()))
                        { m_text_color = m_color_table[static_cast<size_t>(idx)-1]; }
                    }
                }
            }

        // need extra space if converting to HTML, largest tag is 44 chars long
        if (!allocate_text_buffer(m_extraction_type == rtf_extraction_type::rtf_to_text ?
                                  text_length : text_length*50))
            {
            set_filtered_text_length(0);
            return nullptr;
            }

        m_rtf_text = text;
        // read the contents from the temporary buffer created
        while (*m_rtf_text && m_rtf_text < endSentinel)
            {
            int ch = *(m_rtf_text);
            if (m_cGroup < 0)
                { throw rtfparse_stack_underflow(); }

            if (m_ris == RIS::risBin) // if we're parsing binary data, handle it directly
                {
                ecParseChar(ch);
                }
            else
                {
                switch (ch)
                    {
                case '{':
                    ecPushRtfState();
                    break;
                case '}':
                    ecPopRtfState();
                    break;
                case '\\':
                    ecParseRtfKeyword();
                    // make sure we are not at the end of the stream
                    if (*m_rtf_text == 0)
                        {
                        return get_filtered_text();
                        }
                    break;
                case 0x0d:
                    [[fallthrough]];
                case 0x0a: // CR and LF are noise characters...
                    break;
                default:
                    if (m_ris == RIS::risNorm)
                        {
                        ecParseChar(ch);
                        }
                    else
                        {
                        // parsing hex data
                        if (m_ris != RIS::risHex)
                            { throw rtfparse_assertion(); }
                        ++m_rtf_text;
                        // copy over the next two bytes
                        char hexBuffer[3]{ 0 };
                        hexBuffer[0] = static_cast<char>(ch);
                        hexBuffer[1] = m_rtf_text[0];
                        // convert the two bytes (hex value) to character
                        char* dummy{ nullptr };
                        ch = static_cast<int>(std::strtol(hexBuffer, &dummy, 16));
                        ecParseChar(ch);
                        m_ris = RIS::risNorm;
                        }// end else (m_ris != risNorm)
                    break;
                    }// switch
                }// else (m_ris != risBin)
            ++m_rtf_text;
            }// while
        if (m_cGroup < 0)
            {
            throw rtfparse_stack_underflow();
            }
        if (m_cGroup > 0)
            {
            throw rtfparse_unmatched_brace();
            }

        return get_filtered_text();
        }

    //------------------------------------------------
    void rtf_extract_text::ecPushRtfState()
        {
        SAVE* psaveNew = new SAVE;
        reset_property(*psaveNew);

        psaveNew->pNext = m_psave;
        psaveNew->chp = m_chp;
        psaveNew->pap = m_pap;
        psaveNew->sep = m_sep;
        psaveNew->dop = m_dop;
        psaveNew->rds = m_rds;
        psaveNew->ris = m_ris;
        m_ris = RIS::risNorm;
        m_psave = psaveNew;
        ++m_cGroup;

        m_command_stacks.open_stack();
        }

    //------------------------------------------------
    void rtf_extract_text::ecPopRtfState()
        {
        if (!m_psave)
            { throw rtfparse_stack_underflow(); }

        m_chp = m_psave->chp;
        m_pap = m_psave->pap;
        m_sep = m_psave->sep;
        m_dop = m_psave->dop;
        m_rds = m_psave->rds;
        m_ris = m_psave->ris;

        SAVE* psaveOld = m_psave;
        m_psave = m_psave->pNext;
        --m_cGroup;
        delete psaveOld;

        const auto closedStackSpans = m_command_stacks.close_stack();
        if (m_extraction_type == rtf_extraction_type::rtf_to_html)
            { ecPrintString(closedStackSpans.c_str(), closedStackSpans.length()); }
        }

    //------------------------------------------------
    void rtf_extract_text::ecParseRtfKeyword()
        {
        bool fParam{ false };
        bool fNeg{ false };
        int param{ 0 };
        char* pch{ nullptr };
        char szKeyword[30]{ 0 };
        char szParameter[20]{ 0 };

        ++m_rtf_text;
        if (*m_rtf_text == 0)
            { return; }
        int ch = *m_rtf_text;

        if (!is_alpha_7bit(static_cast<wchar_t>(ch))) // a control symbol; no delimiter.
            {
            szKeyword[0] = static_cast<char>(ch);
            szKeyword[1] = 0;
            ecTranslateKeyword(szKeyword, 0, fParam);
            return;
            }
        for (pch = szKeyword;
            is_alpha_7bit(static_cast<wchar_t>(ch));
            ch = *(++m_rtf_text))
            {
            if (*m_rtf_text == 0)
                { return; }
            *pch++ = static_cast<char>(ch);
            }

        *pch = 0;
        if (ch == '-')
            {
            fNeg = true;
            ch = *(++m_rtf_text);
            if (*m_rtf_text == 0)
                { return; }
            }
        if (is_numeric_7bit(static_cast<wchar_t>(ch)))
            {
            fParam = true; // a digit after the control means we have a parameter
            for (pch = szParameter;
                is_numeric_7bit(static_cast<wchar_t>(ch));
                ch = *(++m_rtf_text))
                {
                if (*m_rtf_text == 0)
                    { return; }
                *pch++ = static_cast<char>(ch);
                }
            *pch = 0;
            param = std::atoi(szParameter);
            if (fNeg)
                { param = -param; }
            m_lParam = std::atol(szParameter);
            if (fNeg)
                { m_lParam = -m_lParam; }
            }
        if (ch != ' ')
            { --m_rtf_text;}

        if (m_extraction_type == rtf_extraction_type::rtf_to_html &&
            (std::strcmp(szKeyword, "par") == 0 || std::strcmp(szKeyword, "pard") == 0))
            {
            ++m_paragraphCount;
            // if even, then this is a terminating paragraph command
            if ((m_paragraphCount % 2) == 0)
                { std::strcpy(szKeyword, "par"); }
            // else, it is either the start of a new paragraph or an empty paragraph
            else
                {
                const char* nextKeyword = m_rtf_text;
                while (++nextKeyword)
                    {
                    // eat up the whitespace
                    if (nextKeyword[0] == 0 ||
                        !std::iswspace(static_cast<wchar_t>(nextKeyword[0])))
                        { break; }
                    }
                /* if the next keyword is a new paragraph then this one must be empty
                   (and not properly terminated!), so change it to a line break*/
                if (std::strncmp(nextKeyword, "\\par", 4) == 0)
                    {
                    --m_paragraphCount; // it's also not a paragraph anymore
                    std::strcpy(szKeyword, "line");
                    }
                else
                    { std::strcpy(szKeyword, "pard"); }
                }
            }

        ecTranslateKeyword(szKeyword, param, fParam);
        }

    //------------------------------------------------
    void rtf_extract_text::ecParseChar(int ch)
        {
        if (ch == 0)
            {
            return;
            }
        if (m_ris == RIS::risBin && --m_cbBin <= 0)
            {
            m_ris = RIS::risNorm;
            }
        switch (m_rds)
            {
        case RDS::rdsSkip:
            // Toss this character.
            break;
        case RDS::rdsNorm:
            // Output a character. Properties are valid at this point.
            if (m_extraction_type == rtf_extraction_type::rtf_to_html)
                {
                if (ch > 127)
                    {
                    const std::wstring encodedChar = L"&#" + std::to_wstring(ch) + L";";
                    ecPrintString(encodedChar.c_str(), encodedChar.length());
                    }
                else if (ch == '\\')
                    {
                    ecPrintChar(static_cast<wchar_t>(ch));
                    }
                else if (ch == '<')
                    { ecPrintString(L"&#60;", 5); }
                else if (ch == '>')
                    { ecPrintString(L"&#62;", 5); }
                else if (ch == '\"')
                    { ecPrintString(L"&#34;", 5); }
                else if (ch == '&')
                    { ecPrintString(L"&#38;", 5); }
                else if (ch == '\'')
                    { ecPrintString(L"&#39;", 5); }
                else if (ch == ' ')
                    {
                    // if following another space or a no break space, then encode it as a no break space
                    if (get_filtered_text_length() > 0 &&
                        get_filtered_text()[get_filtered_text_length() -1] == L' ')
                        { ecPrintString(L"&nbsp;", 6); }
                    else if (get_filtered_text_length() >= 6 &&
                                std::wcsncmp(get_filtered_text() +
                                (get_filtered_text_length()-6), L"&nbsp;", 6) == 0)
                        { ecPrintString(L"&nbsp;", 6); }
                    else
                        { ecPrintChar(static_cast<wchar_t>(ch)); }
                    }
                else
                    { ecPrintChar(static_cast<wchar_t>(ch)); }
                }
            else
                { ecPrintChar(static_cast<wchar_t>(ch)); }
            break;
        default:
            // handle other destinations....
            break;
            }
        }

    //------------------------------------------------
    void rtf_extract_text::ecParseString(const wchar_t* text, const size_t length) noexcept
        {
        if (m_ris == RIS::risBin && --m_cbBin <= 0)
            {
            m_ris = RIS::risNorm;
            }
        switch (m_rds)
            {
        case RDS::rdsSkip:
            // Toss this character.
            break;
        case RDS::rdsNorm:
            // Output a character. Properties are valid at this point.
            ecPrintString(text, length);
            break;
        default:
            // handle other destinations....
            break;
            }
        }

    //------------------------------------------------
    void rtf_extract_text::ecProcessFontProperty(const wchar_t* htmlCmd,
                                                 const size_t htmlCmdLength) noexcept
        {
        assert(htmlCmd && std::wcslen(htmlCmd) == htmlCmdLength);

        if (m_ris == RIS::risBin && --m_cbBin <= 0)
            { m_ris = RIS::risNorm; }
        switch (m_rds)
            {
        case RDS::rdsSkip:
            // Toss it.
            break;
        case RDS::rdsNorm:
            if (m_extraction_type == rtf_extraction_type::rtf_to_html)
                {
                ecPrintString(htmlCmd, htmlCmdLength);
                m_command_stacks.add_command();
                }
            break;
        default:
            // handle other destinations....
            break;
            }
        }

    //------------------------------------------------
    void rtf_extract_text::ecProcessFontColor(const int idx)
        {
        if (m_ris == RIS::risBin && --m_cbBin <= 0)
            { m_ris = RIS::risNorm; }
        switch (m_rds)
            {
        case RDS::rdsSkip:
            // Toss this color.
            break;
        case RDS::rdsNorm:
            if (m_extraction_type == rtf_extraction_type::rtf_to_html)
                {
                // If a valid color.
                // Zero index is just a reset to the control's default color,
                // which is not in the table. Hence color 1 is actually the
                // first color in the table.
                if (idx > 0 && idx <= static_cast<int>(m_color_table.size()))
                    {
                    const std::wstring colorCmd = L"<span class=\"" + m_style_prefix +
                                                  L"fc" + std::to_wstring(idx) + L"\">";
                    ecPrintString(colorCmd.c_str(), colorCmd.length());
                    m_command_stacks.add_command();
                    }
                }
            break;
        default:
            // handle other destinations....
            break;
            }
        }

    //------------------------------------------------
    void rtf_extract_text::ecProcessHighlight(const int idx)
        {
        if (m_ris == RIS::risBin && --m_cbBin <= 0)
            { m_ris = RIS::risNorm; }
        switch (m_rds)
            {
        case RDS::rdsSkip:
            // Toss this color.
            break;
        case RDS::rdsNorm:
            if (m_extraction_type == rtf_extraction_type::rtf_to_html)
                {
                // If a valid color.
                // Zero index is just a reset to the control's default color,
                // which is not in the table. Hence color 1 is actually the
                // first color in the table.
                if (idx > 0 && idx <= static_cast<int>(m_color_table.size()))
                    {
                    const std::wstring colorCmd = L"<span class=\"" + m_style_prefix +
                                                  L"bc" + std::to_wstring(idx) + L"\">";
                    ecPrintString(colorCmd.c_str(), colorCmd.length());
                    m_command_stacks.add_command();
                    }
                }
            break;
        default:
            // handle other destinations....
            break;
            }
        }

    //------------------------------------------------
    void rtf_extract_text::ecApplyPropChange(IPROP iprop, int val)
        {
        char *pb = nullptr;

        if (m_rds == RDS::rdsSkip)     // If we're skipping text,
            return;                    // don't do anything.

        switch (static_cast<int>(rgprop[static_cast<int>(iprop)].prop))
            {
        case static_cast<int>(PROPTYPE::propDop):
            pb = (char*)&m_dop;
            break;
        case static_cast<int>(PROPTYPE::propSep):
            pb = (char*)&m_sep;
            break;
        case static_cast<int>(PROPTYPE::propPap):
            pb = (char*)&m_pap;
            break;
        case static_cast<int>(PROPTYPE::propChp):
            pb = (char*)&m_chp;
            break;
        default:
            if (rgprop[static_cast<int>(iprop)].actn != ACTN::actnSpec)
                { throw rtfparse_bad_table(); }
            break;
            }

        if (pb == nullptr)
            { return; }

        switch (rgprop[static_cast<int>(iprop)].actn)
            {
        case ACTN::actnByte:
            pb[rgprop[static_cast<int>(iprop)].offset] = static_cast<unsigned char>(val);
            break;
        case ACTN::actnWord:
            (*(int*) (pb+rgprop[static_cast<int>(iprop)].offset)) = val;
            break;
        case ACTN::actnSpec:
            ecParseSpecialProperty(iprop);
            break;
        default:
            throw rtfparse_bad_table();
            }
        }

    //------------------------------------------------
    void rtf_extract_text::ecTranslateKeyword(const char* szKeyword, int param, const bool fParam)
        {
        // keyword 'u' is a Unicode character if it has a parameter
        if (fParam && std::strcmp(szKeyword, "u") == 0)
            {
            // Unicode values over 32,768 are signed (negative) in RTF syntax,
            // so convert it back to signed.
            if (param < 0)
                { param += 65'536; }
            if (m_extraction_type == rtf_extraction_type::rtf_to_html && (param > 127))
                {
                const std::wstring encodedChar = L"&#" + std::to_wstring(param) + L";";
                ecPrintString(encodedChar.c_str(), encodedChar.length());
                }
            else
                { ecPrintChar(static_cast<wchar_t>(param)); }
            ++m_rtf_text; // will have a trailing '?' or '*' that we need to skip
            return;
            }
        // set the documents font to the current font (for RTF->HTML conversion)
        else if (fParam && std::strcmp(szKeyword, "fs") == 0)
            { m_font_size = param/2/*RTF font size is in half points*/; }
        // this command causes a paragraph to be indented or bulleted
        else if (std::strcmp(szKeyword, "pntext") == 0)
            {
            m_in_bullet_state = true;
            if (m_extraction_type == rtf_extraction_type::rtf_to_html)
                {
                ecPrintString(L"&nbsp;&nbsp;&nbsp;&nbsp;", 24);
                }
            else
                { ecPrintChar(L'\t'); }
            // this is usually some bullet definition in here, just skip that
            const char* endOfBulletDescription = std::strchr(m_rtf_text, '}');
            if (endOfBulletDescription)
                { m_rtf_text = endOfBulletDescription-1; }
            return;
            }
        // if we are in an indented section then override the "line"
        // command to include a tab for the next line
        else if (m_in_bullet_state && std::strcmp(szKeyword, "line") == 0)
            {
            if (m_extraction_type == rtf_extraction_type::rtf_to_html)
                {
                ecPrintString(L"<br />\n&nbsp;&nbsp;&nbsp;&nbsp;", 31);
                }
            else
                { ecPrintString(L"\n\t", 2); }
            return;
            }
        //paragraph commands turns off indented/bulleted state
        else if (std::strcmp(szKeyword, "par") == 0 || std::strcmp(szKeyword, "pard") == 0)
            { m_in_bullet_state = false; }

        // search for szKeyword in rgsymRtf
        std::set<rtf_symbol>::const_iterator symbolIter = m_keyword_command_table->find(szKeyword);
        if (m_keyword_command_table->is_not_found(symbolIter)) // control word not found
            {
            if (m_fSkipDestIfUnk)             // if this is a new destination
                { m_rds = RDS::rdsSkip; }     // skip the destination
            // else just discard it
            m_fSkipDestIfUnk = false;
            return;
            }

        const char* const rtfStart = m_rtf_text;

        // found it!  use kwd to determine what to do with it.
        m_fSkipDestIfUnk = false;
        switch (symbolIter->kwd)
            {
        case KWD::kwdProp:
            if (symbolIter->fPassDflt || !fParam)
                param = symbolIter->dflt;
            ecApplyPropChange(static_cast<IPROP>(symbolIter->idx), param);
            break;
        // command is actually text being copied to the output
        case KWD::kwdChar:
            ecParseChar(symbolIter->idx);
            break;
        case KWD::kwdString:
            ecParseString(symbolIter->printString.c_str(), symbolIter->printString.length());
            break;
        case KWD::kwdDest:
            ecChangeDest();
            break;
        case KWD::kwdSectionSkip:
            ecChangeDest();
            m_rtf_text = string_util::find_unescaped_matching_close_tag(m_rtf_text, '{', '}');
            // caller will increment m_rtf_text, so step back 1 so that it won't consume this '}'
            // and result in a grouping stack mismatch
            if (m_rtf_text && m_rtf_text > rtfStart)
                { --m_rtf_text; }
            // couldn't find the '}' or we are on a weird spot, so reset to where we were
            else
                { m_rtf_text = rtfStart ;}
            break;
        case KWD::kwdSpec:
            ecParseSpecialKeyword(static_cast<IPFN>(symbolIter->idx));
            break;
        case KWD::kwdHighlight:
            ecProcessHighlight(param);
            break;
        case KWD::kwdFontColor:
            ecProcessFontColor(param);
            break;
        case KWD::kwdBold:
            ecProcessFontProperty(L"<span style='font-weight:bold;'>", 32);
            break;
        case KWD::kwdUnderline:
            // 'ulnone' turns off 'ul'
            if (std::strncmp(szKeyword, "ulnone", 6) == 0)
                { ecProcessFontProperty(L"<span style='text-decoration:none;'>", 36); }
            else
                { ecProcessFontProperty(L"<span style='text-decoration:underline;'>", 41); }
            break;
        case KWD::kwdItalic:
            ecProcessFontProperty(L"<span style='font-style:italic;'>", 33);
            break;
        case KWD::kwdStrikeThrough:
            ecProcessFontProperty(L"<span style='text-decoration:line-through;'>", 44);
            break;
        default:
            throw rtfparse_bad_table();
            }
        }

    //------------------------------------------------
    void rtf_extract_text::ecParseSpecialKeyword(IPFN ipfn)
        {
        // if we're skipping, and it's not
        // the \bin keyword, ignore it.
        if (m_rds == RDS::rdsSkip && ipfn != IPFN::ipfnBin)
            return;

        switch (ipfn)
            {
        case IPFN::ipfnBin:
            m_ris = RIS::risBin;
            m_cbBin = m_lParam;
            break;
        case IPFN::ipfnSkipDest:
            m_fSkipDestIfUnk = true;
            break;
        case IPFN::ipfnHex:
            m_ris = RIS::risHex;
            break;
        default:
            throw rtfparse_bad_table();
            }
        }
    }
