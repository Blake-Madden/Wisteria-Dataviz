///////////////////////////////////////////////////////////////////////////////
// Name:        colorbrewer.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "colorbrewer.h"

using namespace Wisteria::Colors;

const std::vector<std::wstring> ColorBrewer::m_colors =
    {
    L"#FBCB78", L"#5D8AA8", L"#598C74", L"#F0F8FF", L"#E32636", L"#EFDECD", L"#E52B50", L"#FFBF00", L"#D28240", L"#FF033E",
    L"#9966CC", L"#A4C639", L"#F2F3F4", L"#CD9575", L"#915C83", L"#FAEBD7", L"#008000", L"#DAB5B4", L"#8DB600", L"#FBCEB1",
    L"#00FFFF", L"#7FFFD4", L"#88ABB4", L"#4B5320", L"#E9D66B", L"#B2BEB5", L"#87A96B", L"#FF9966", L"#A52A2A", L"#FDEE00",
    L"#6E7F80", L"#857C5D", L"#FF2052", L"#007FFF", L"#F0FFFF", L"#89CFF0", L"#A1CAF1", L"#F4C2C2", L"#21ABCD", L"#FAE7B5",
    L"#FFE135", L"#C0A98B", L"#848482", L"#98777B", L"#BCD4E6", L"#9F8170", L"#F5F5DC", L"#F0CDA0", L"#FFE4C4", L"#3D2B1F",
    L"#FE6F5E", L"#000000", L"#FFEBCD", L"#318CE7", L"#ACE5EE", L"#FAF0BE", L"#0000FF", L"#A2A2D0", L"#6699CC", L"#0D98BA",
    L"#014E4C", L"#8A2BE2", L"#ABD1C9", L"#8A2BE2", L"#DE5D83", L"#79443B", L"#0095B6", L"#E3DAC9", L"#CC0000", L"#006A4E",
    L"#873260", L"#0070FF", L"#B5A642", L"#CB4154", L"#1DACD6", L"#66FF00", L"#BF94E4", L"#C32148", L"#FF007F", L"#08E8DE",
    L"#D19FE8", L"#F4BBFF", L"#FF55A3", L"#FB607F", L"#004225", L"#CD7F32", L"#A52A2A", L"#FFC1CC", L"#E7FEFF", L"#F0DC82",
    L"#480607", L"#CDBFB0", L"#800020", L"#DEB887", L"#CC5500", L"#E97451", L"#8A3324", L"#BD33A4", L"#702963", L"#536872",
    L"#5F9EA0", L"#91A3B0", L"#006B3C", L"#ED872D", L"#E30022", L"#FFF600", L"#A67B5B", L"#4B3621", L"#1E4D2B", L"#A3C1AD",
    L"#C19A6B", L"#78866B", L"#FFFF99", L"#FFEF00", L"#FF0800", L"#E4717A", L"#00BFFF", L"#592720", L"#C41E3A", L"#00CC99",
    L"#FF0040", L"#EB4C42", L"#FF0038", L"#FFA6C9", L"#B31B1B", L"#99BADD", L"#ED9121", L"#EDE1CE", L"#ACB19F", L"#ACE1AF",
    L"#B2FFFF", L"#4997D0", L"#DE3163", L"#EC3B83", L"#007BA7", L"#2A52BE", L"#007AA5", L"#E03C31", L"#A0785A", L"#FAD6A5",
    L"#36454F", L"#7FFF00", L"#DE3163", L"#FFB7C5", L"#CD5C5C", L"#9E3E33", L"#D2691E", L"#FFA700", L"#98817B", L"#E34234",
    L"#D2691E", L"#E4D00A", L"#888782", L"#FBCCE7", L"#0047AB", L"#D2691E", L"#6F4E37", L"#C6C0B6", L"#9BDDFF", L"#002E63",
    L"#75B9AE", L"#8C92AC", L"#C2CCC4", L"#B87333", L"#996666", L"#FF3800", L"#FF7F50", L"#F88379", L"#FF4040", L"#893F45",
    L"#FBEC5D", L"#B31B1B", L"#9ACEEB", L"#6495ED", L"#FFF8DC", L"#FFF8E7", L"#FFBCD9", L"#FFFDD0", L"#DC143C", L"#BE0032",
    L"#990000", L"#00FFFF", L"#FFFF31", L"#FED340", L"#F0E130", L"#00008B", L"#654321", L"#5D3954", L"#A40000", L"#08457E",
    L"#986960", L"#CD5B45", L"#008B8B", L"#536878", L"#B8860B", L"#A9A9A9", L"#013220", L"#1A2421", L"#BDB76B", L"#483C32",
    L"#734F96", L"#8B008B", L"#003366", L"#556B2F", L"#FF8C00", L"#9932CC", L"#779ECB", L"#03C03C", L"#966FD6", L"#C23B22",
    L"#E75480", L"#003399", L"#872657", L"#8B0000", L"#E9967A", L"#560319", L"#8FBC8F", L"#3C1414", L"#483D8B", L"#2F4F4F",
    L"#177245", L"#918151", L"#FFA812", L"#483C32", L"#CC4E5C", L"#00CED1", L"#9400D3", L"#00693E", L"#555555", L"#D70A53",
    L"#A9203E", L"#EF3038", L"#E9692C", L"#DA3287", L"#FAD6A5", L"#B94E48", L"#704241", L"#C154C1", L"#004B49", L"#9955BB",
    L"#CC00CC", L"#623F45", L"#FFCBA4", L"#FF1493", L"#FF9933", L"#00BFFF", L"#1560BD", L"#C19A6B", L"#EDC9AF", L"#696969",
    L"#1E90FF", L"#D71868", L"#85BB65", L"#F0EADC", L"#967117", L"#B89D9A", L"#00009C", L"#E1A95F", L"#C2B280", L"#B1975F",
    L"#614051", L"#F0EAD6", L"#1034A6", L"#7DF9FF", L"#FF003F", L"#00FFFF", L"#00FF00", L"#6F00FF", L"#F4BBFF", L"#CCFF00",
    L"#BF00FF", L"#3F00FF", L"#8F00FF", L"#FFFF00", L"#50C878", L"#96C8A2", L"#95978A", L"#B54D7F", L"#C19A6B", L"#801818",
    L"#FF00FF", L"#B53389", L"#F400A1", L"#8AA3B1", L"#E5AA70", L"#4D5D53", L"#71BC78", L"#4F7942", L"#FF2800", L"#6C541E",
    L"#B22222", L"#CE2029", L"#7B3730", L"#E25822", L"#FC8EAC", L"#F4D3B3", L"#F7E98E", L"#EEDC82", L"#FFFAF0", L"#FFBF00",
    L"#FF1493", L"#CCFF00", L"#D69969", L"#FF004F", L"#228B22", L"#716998", L"#A67B5B", L"#0072BB", L"#86608E", L"#F64A8A",
    L"#D9C661", L"#CBD0C2", L"#FF00FF", L"#FF77FF", L"#E48400", L"#CC6666", L"#DCDCDC", L"#E49B0F", L"#F8F8FF", L"#B06500",
    L"#6082B6", L"#E6E8FA", L"#FFD700", L"#996515", L"#FCC200", L"#DAA520", L"#FFDF00", L"#A8E4A0", L"#808080", L"#465945",
    L"#00FF00", L"#1164B4", L"#ADFF2F", L"#A99A86", L"#00FF7F", L"#663854", L"#446CCF", L"#5218FA", L"#E9D66B", L"#3FFF00",
    L"#C90016", L"#DA9100", L"#808000", L"#DF73FF", L"#564537", L"#8AC6BD", L"#F400A1", L"#F0FFF0", L"#49796B", L"#FF1DCE",
    L"#FF69B4", L"#355E3B", L"#FCF75E", L"#B2EC5D", L"#138808", L"#CD5C5C", L"#E3A857", L"#4B0082", L"#002FA7", L"#FF4F00",
    L"#5A4FCF", L"#F4F0EC", L"#009000", L"#FFFFF0", L"#00A86B", L"#F8DE7E", L"#D73B3E", L"#F1BFB1", L"#A50B5E", L"#FADA5E",
    L"#BDDA57", L"#29AB87", L"#815D40", L"#4CBB17", L"#C3B091", L"#D7C5AE", L"#E8000D", L"#D6CADD", L"#26619C", L"#087830",
    L"#FEFE22", L"#BAA185", L"#A9BA9D", L"#CF1020", L"#E6E6FA", L"#CCCCFF", L"#FFF0F5", L"#C4C3D0", L"#9457EB", L"#EE82EE",
    L"#E6E6FA", L"#FBAED2", L"#967BB6", L"#FBA0E3", L"#7CFC00", L"#FFF700", L"#FFFACD", L"#BFFF00", L"#FFF44F", L"#FDD5B1",
    L"#ADD8E6", L"#B5651D", L"#E66771", L"#F08080", L"#93CCEA", L"#F56991", L"#E0FFFF", L"#F984EF", L"#FAFAD2", L"#D3D3D3",
    L"#90EE90", L"#F0E68C", L"#B19CD9", L"#FFB6C1", L"#FFA07A", L"#FF9999", L"#20B2AA", L"#87CEFA", L"#778899", L"#B38B6D",
    L"#E68FAC", L"#FFFFED", L"#C8A2C8", L"#BFFF00", L"#32CD32", L"#195905", L"#FAF0E6", L"#C19A6B", L"#534B4F", L"#E62020",
    L"#FFBD88", L"#FF00FF", L"#AAF0D1", L"#F8F4FF", L"#C04000", L"#FBEC5D", L"#6050DC", L"#0BDA51", L"#979AAA", L"#FF8243",
    L"#74C365", L"#800000", L"#E0B0FF", L"#EF98AA", L"#915F6D", L"#73C2FB", L"#E5B73B", L"#66DDAA", L"#0000CD", L"#E2062C",
    L"#AF4035", L"#F3E5AB", L"#035096", L"#1C352D", L"#DDA0DD", L"#BA55D3", L"#0067A5", L"#9370DB", L"#BB3385", L"#3CB371",
    L"#7B68EE", L"#C9DC87", L"#00FA9A", L"#674C47", L"#0054B4", L"#48D1CC", L"#C71585", L"#FDBCB4", L"#191970", L"#004953",
    L"#FFC40C", L"#3EB489", L"#F5FFFA", L"#98FF98", L"#FFE4E1", L"#FAEBD7", L"#967117", L"#73A9C2", L"#AE0C00", L"#ADDFAD",
    L"#30BA8F", L"#997A8D", L"#18453B", L"#C54B8C", L"#F2F3F4", L"#FFDB58", L"#FFDB58", L"#21421E", L"#F6ADC6", L"#2A8000",
    L"#FADA5E", L"#FFDEAD", L"#EC8430", L"#000080", L"#FFA343", L"#FE59C2", L"#39FF14", L"#A4DDED", L"#059033", L"#0077BE",
    L"#CC7722", L"#B6B8A5", L"#008000", L"#CFB53B", L"#FDF5E6", L"#796878", L"#673147", L"#C08081", L"#808000", L"#6B8E23",
    L"#BAB86C", L"#9AB973", L"#0F0F0F", L"#B784A7", L"#FFA500", L"#FF9F00", L"#FF4500", L"#F8D568", L"#DA70D6", L"#E5E2DA",
    L"#654321", L"#414A4C", L"#FF6E4A", L"#002147", L"#1CA9C9", L"#006600", L"#273BE2", L"#682860", L"#BCD4E6", L"#AFEEEE",
    L"#987654", L"#AF4035", L"#9BC4E2", L"#DDADAF", L"#DA8A67", L"#ABCDEF", L"#E6BE8A", L"#EEE8AA", L"#98FB98", L"#DCD0FF",
    L"#F984E5", L"#FADADD", L"#DDA0DD", L"#DB7093", L"#96DED1", L"#C9C0BB", L"#ECEBBD", L"#BC987E", L"#DB7093", L"#78184A",
    L"#FFEFD5", L"#50C878", L"#AEC6CF", L"#836953", L"#CFCFC4", L"#77DD77", L"#F49AC2", L"#FFB347", L"#FFD1DC", L"#B39EB5",
    L"#FF6961", L"#CB99C9", L"#FDFD96", L"#800080", L"#536878", L"#FFE5B4", L"#FFDAB9", L"#FADFAD", L"#D1E231", L"#EAE0C8",
    L"#88D8C0", L"#E6E200", L"#CCCCFF", L"#1C39BB", L"#32127A", L"#D99058", L"#F77FBE", L"#701C1C", L"#CC3333", L"#FE28A2",
    L"#DF00FF", L"#000F89", L"#123524", L"#FDDDE6", L"#01796F", L"#FFC0CB", L"#FC74FD", L"#E7ACCF", L"#DEC3B9", L"#F78FA7",
    L"#C9AA98", L"#93C572", L"#E5E4E2", L"#DDA0DD", L"#FF5A36", L"#B0E0E6", L"#C9B29C", L"#FF8F00", L"#003153", L"#DF00FF",
    L"#CC8899", L"#FF7518", L"#EDECE6", L"#800080", L"#69359C", L"#9D81BA", L"#FE4EDA", L"#50404D", L"#5D8AA8", L"#A489A0",
    L"#FF355E", L"#6BB7C4", L"#E30B5D", L"#915F6D", L"#E25098", L"#B3446C", L"#D68A59", L"#FF33CC", L"#E3256B", L"#FF0000",
    L"#A52A2A", L"#FF5349", L"#C71585", L"#004040", L"#D70040", L"#0892D0", L"#B666D2", L"#B03060", L"#414833", L"#1FCECB",
    L"#FF007F", L"#F9429E", L"#674846", L"#B76E79", L"#E32636", L"#FF66CC", L"#AA98A9", L"#CD9C85", L"#905D5D", L"#AB4E52",
    L"#65000B", L"#D40000", L"#BC8F8F", L"#EBCECB", L"#0038A8", L"#4169E1", L"#CA2C92", L"#7851A9", L"#E0115F", L"#FF0028",
    L"#BB6528", L"#E18E96", L"#A81C07", L"#80461B", L"#B7410E", L"#00563F", L"#8B4513", L"#FF6700", L"#F4C430", L"#23297A",
    L"#FF8C69", L"#FF91A4", L"#AB7878", L"#C2B280", L"#967117", L"#ECD540", L"#F4A460", L"#967117", L"#507D2A", L"#0F52BA",
    L"#CBA135", L"#FF2400", L"#FFD800", L"#76FF7A", L"#006994", L"#2E8B57", L"#321414", L"#CDD2CA", L"#FFF5EE", L"#FFBA00",
    L"#704214", L"#C8D3E7", L"#8A795D", L"#45CEA2", L"#009E60", L"#FC0FC0", L"#882D17", L"#C0C0C0", L"#CB410B", L"#007474",
    L"#87CEEB", L"#CF71AF", L"#6A5ACD", L"#708090", L"#1A472A", L"#2A623D", L"#5D5D5D", L"#AAAAAA", L"#003399", L"#933D41",
    L"#100C08", L"#E2B6A7", L"#FFFAFA", L"#B0785C", L"#0FC0FC", L"#A7FC00", L"#00FF7F", L"#4682B4", L"#FADA5E", L"#990000",
    L"#008080", L"#E4D96F", L"#C6B9B8", L"#F0D39D", L"#FFCC33", L"#FAD6A5", L"#FD5E53", L"#B2AC96", L"#D2B48C", L"#F94D00",
    L"#F28500", L"#FFCC00", L"#483C32", L"#8B8589", L"#CD5700", L"#D0F0C0", L"#008080", L"#367588", L"#006D5B", L"#F4C2C2",
    L"#E2725B", L"#D8BFD8", L"#DE6FA1", L"#FC89AC", L"#0ABAB5", L"#E08D3C", L"#DBD7D2", L"#EEE600", L"#FF6347", L"#746CC0",
    L"#FFC87C", L"#FD0E35", L"#C2CFCF", L"#2F2F30", L"#808080", L"#00755E", L"#0073CF", L"#417DC1", L"#DEAA88", L"#B57281",
    L"#30D5C8", L"#00FFEF", L"#A0D6B4", L"#66424D", L"#8A496B", L"#66023C", L"#0033AA", L"#D9004C", L"#8878C3", L"#536895",
    L"#FFB300", L"#3CD070", L"#120A8F", L"#4166F5", L"#FF6FFF", L"#635147", L"#5B92E5", L"#B78727", L"#FFFF66", L"#014421",
    L"#7B1113", L"#AE2029", L"#CFC0AB", L"#E1AD21", L"#990000", L"#FFCC00", L"#D3003F", L"#F3E5AB", L"#C5B358", L"#C80815",
    L"#43B3AE", L"#E34234", L"#A020F0", L"#EE82EE", L"#324AB2", L"#F75394", L"#40826D", L"#922724", L"#9F1D35", L"#DA1D81",
    L"#FFA089", L"#9F00FF", L"#4B5645", L"#004242", L"#00FFFF", L"#B4CCC9", L"#645452", L"#F5DEB3", L"#FFFFFF", L"#F5F5F5",
    L"#CDB592", L"#A2ADD0", L"#FF43A4", L"#FC6C85", L"#722F37", L"#C9A0DC", L"#738678", L"#0F4D92", L"#FFFF00", L"#9ACD32",
    L"#FFAE42", L"#0014A8", L"#2C1608"
    };

wxColour ColorBrewer::GetColor(const Color color)
    {
    if (static_cast<uint16_t>(color) >= static_cast<uint16_t>(Color::COLOR_COUNT))
        { return wxNullColour; }
    return wxColour(m_colors[static_cast<uint16_t>(color)]);
    }

wxColour ColorBrewer::BrewColor(const double value) const
    {
    // return invalid color or NaN
    if (std::isnan(value))
        { return wxColour(); }
    // verify that we have a valid spectrum initialized
    NON_UNIT_TEST_ASSERT(m_colorSpectrum.size() > 1);
    if (m_colorSpectrum.size() < 2)
        { throw std::length_error("Color scale has not been initialized in color brewer."); }
    double normalizedValue = statistics::normalize<double>(m_range.first, m_range.second, value);

    int idx1{0}, idx2{0};    // |-- Our desired color will be between these two indexes in "colorSpectrum".
    double fractBetween{0};  // Fraction between "idx1" and "idx2" where our value is.

    if (normalizedValue <= 0)
        { idx1 = idx2 = 0; }
    else if (normalizedValue >= 1)
        { idx1 = idx2 = m_colorSpectrum.size()-1; }
    else
        {
        normalizedValue = normalizedValue * (m_colorSpectrum.size()-1);
        idx1 = std::floor(normalizedValue); // Our desired color will be after this index.
        idx2 = idx1+1;                      // ... and before this index (inclusive).
        fractBetween = normalizedValue - static_cast<double>(idx1); // Distance between the two indexes (0-1).
        }

    wxColour brewedColor((m_colorSpectrum[idx2].Red() - m_colorSpectrum[idx1].Red())*fractBetween + m_colorSpectrum[idx1].Red(),
        (m_colorSpectrum[idx2].Green() - m_colorSpectrum[idx1].Green())*fractBetween + m_colorSpectrum[idx1].Green(),
        (m_colorSpectrum[idx2].Blue() - m_colorSpectrum[idx1].Blue())*fractBetween + m_colorSpectrum[idx1].Blue());

    return brewedColor;
    }

wxColour ColorContrast::Contrast(const wxColour& color)
    {
    const auto bgLuminance = m_baseColor.GetLuminance();
    const auto colorLuminance = color.GetLuminance();
    const auto luminanceDifference = std::abs(bgLuminance-colorLuminance);
    const auto adjustmentNeeded = m_tolerance - luminanceDifference;
    if (adjustmentNeeded > 0)
        {
        // if background is lighter or the same...
        if (bgLuminance >= colorLuminance)
            {
            // ...and color can be made darker by the full amount of the adjustment,
            // then make it darker
            if ((colorLuminance-adjustmentNeeded) >= 0)
                { return color.ChangeLightness(100-(adjustmentNeeded*100)); }
            // otherwise, make it lighter. Going the other way will require lightening it
            // up to the other color, then lightening by the full tolerance amount.
            // An example of this would be a dark gray background and even darker gray text
            // that's too close to black to make it useful as black. Instead, you lighten
            // the text color to the same level as the background and then lighten it by the
            // full tolerance value.
            else
                { return color.ChangeLightness(100+((adjustmentNeeded+m_tolerance)*100)); }
            }
        // or if background is darker
        else if (bgLuminance < colorLuminance)
            {
            // make the text lighter
            if ((colorLuminance+adjustmentNeeded) <= 1)
                { return color.ChangeLightness(100+(adjustmentNeeded*100)); }
            // or darker
            else
                { return color.ChangeLightness(100-((adjustmentNeeded+m_tolerance)*100)); }
            }
        }

    return color;
    }
