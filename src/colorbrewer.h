/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_COLORBREWER_H__
#define __WISTERIA_COLORBREWER_H__

#include <wx/wx.h>
#include <wx/gdicmn.h>
#include <wx/dcgraph.h>
#include <wx/string.h>
#include <vector>
#include <initializer_list>
#include "math/statistics.h"

/// @brief Color management features (building, brewing, contrasting, etc.).
namespace Wisteria::Colors
    {
    /** @brief List of colors to use with ColorBrewer::GetColor().
        @details This list was adapted from <a href='https://www.colorhexa.com'>www.colorhexa.com</a> and Sherman-Williams.
        @internal WARNING for developer:
         Do not edit this enumeration without updating the color list in ColorBrewer.
         This enum and color list are generated from an R script (tools/Build Color List.R);
         use that instead of manually updating this.*/
    enum class Color
        {
        Afternoon,                  ///< \htmlonly <div style='background-color:#FBCB78; width:50px;'>&nbsp;</div> \endhtmlonly
        AirForceBlue,               ///< \htmlonly <div style='background-color:#5D8AA8; width:50px;'>&nbsp;</div> \endhtmlonly
        Alexandrite,                ///< \htmlonly <div style='background-color:#598C74; width:50px;'>&nbsp;</div> \endhtmlonly
        AliceBlue,                  ///< \htmlonly <div style='background-color:#F0F8FF; width:50px;'>&nbsp;</div> \endhtmlonly
        AlizarinCrimson,            ///< \htmlonly <div style='background-color:#E32636; width:50px;'>&nbsp;</div> \endhtmlonly
        Almond,                     ///< \htmlonly <div style='background-color:#EFDECD; width:50px;'>&nbsp;</div> \endhtmlonly
        Amaranth,                   ///< \htmlonly <div style='background-color:#E52B50; width:50px;'>&nbsp;</div> \endhtmlonly
        Amber,                      ///< \htmlonly <div style='background-color:#FFBF00; width:50px;'>&nbsp;</div> \endhtmlonly
        AmberWave,                  ///< \htmlonly <div style='background-color:#D28240; width:50px;'>&nbsp;</div> \endhtmlonly
        AmericanRose,               ///< \htmlonly <div style='background-color:#FF033E; width:50px;'>&nbsp;</div> \endhtmlonly
        Amethyst,                   ///< \htmlonly <div style='background-color:#9966CC; width:50px;'>&nbsp;</div> \endhtmlonly
        AndroidGreen,               ///< \htmlonly <div style='background-color:#A4C639; width:50px;'>&nbsp;</div> \endhtmlonly
        AntiFlashWhite,             ///< \htmlonly <div style='background-color:#F2F3F4; width:50px;'>&nbsp;</div> \endhtmlonly
        AntiqueBrass,               ///< \htmlonly <div style='background-color:#CD9575; width:50px;'>&nbsp;</div> \endhtmlonly
        AntiqueFuchsia,             ///< \htmlonly <div style='background-color:#915C83; width:50px;'>&nbsp;</div> \endhtmlonly
        AntiqueWhite,               ///< \htmlonly <div style='background-color:#FAEBD7; width:50px;'>&nbsp;</div> \endhtmlonly
        Ao,                         ///< \htmlonly <div style='background-color:#008000; width:50px;'>&nbsp;</div> \endhtmlonly
        Appleblossom,               ///< \htmlonly <div style='background-color:#DAB5B4; width:50px;'>&nbsp;</div> \endhtmlonly
        AppleGreen,                 ///< \htmlonly <div style='background-color:#8DB600; width:50px;'>&nbsp;</div> \endhtmlonly
        Apricot,                    ///< \htmlonly <div style='background-color:#FBCEB1; width:50px;'>&nbsp;</div> \endhtmlonly
        Aqua,                       ///< \htmlonly <div style='background-color:#00FFFF; width:50px;'>&nbsp;</div> \endhtmlonly
        Aquamarine,                 ///< \htmlonly <div style='background-color:#7FFFD4; width:50px;'>&nbsp;</div> \endhtmlonly
        Aquitaine,                  ///< \htmlonly <div style='background-color:#88ABB4; width:50px;'>&nbsp;</div> \endhtmlonly
        ArmyGreen,                  ///< \htmlonly <div style='background-color:#4B5320; width:50px;'>&nbsp;</div> \endhtmlonly
        ArylideYellow,              ///< \htmlonly <div style='background-color:#E9D66B; width:50px;'>&nbsp;</div> \endhtmlonly
        AshGrey,                    ///< \htmlonly <div style='background-color:#B2BEB5; width:50px;'>&nbsp;</div> \endhtmlonly
        Asparagus,                  ///< \htmlonly <div style='background-color:#87A96B; width:50px;'>&nbsp;</div> \endhtmlonly
        AtomicTangerine,            ///< \htmlonly <div style='background-color:#FF9966; width:50px;'>&nbsp;</div> \endhtmlonly
        Auburn,                     ///< \htmlonly <div style='background-color:#A52A2A; width:50px;'>&nbsp;</div> \endhtmlonly
        Aureolin,                   ///< \htmlonly <div style='background-color:#FDEE00; width:50px;'>&nbsp;</div> \endhtmlonly
        Aurometalsaurus,            ///< \htmlonly <div style='background-color:#6E7F80; width:50px;'>&nbsp;</div> \endhtmlonly
        Avocado,                    ///< \htmlonly <div style='background-color:#857C5D; width:50px;'>&nbsp;</div> \endhtmlonly
        Awesome,                    ///< \htmlonly <div style='background-color:#FF2052; width:50px;'>&nbsp;</div> \endhtmlonly
        Azure,                      ///< \htmlonly <div style='background-color:#007FFF; width:50px;'>&nbsp;</div> \endhtmlonly
        AzureMist,                  ///< \htmlonly <div style='background-color:#F0FFFF; width:50px;'>&nbsp;</div> \endhtmlonly
        BabyBlue,                   ///< \htmlonly <div style='background-color:#89CFF0; width:50px;'>&nbsp;</div> \endhtmlonly
        BabyBlueEyes,               ///< \htmlonly <div style='background-color:#A1CAF1; width:50px;'>&nbsp;</div> \endhtmlonly
        BabyPink,                   ///< \htmlonly <div style='background-color:#F4C2C2; width:50px;'>&nbsp;</div> \endhtmlonly
        BallBlue,                   ///< \htmlonly <div style='background-color:#21ABCD; width:50px;'>&nbsp;</div> \endhtmlonly
        BananaMania,                ///< \htmlonly <div style='background-color:#FAE7B5; width:50px;'>&nbsp;</div> \endhtmlonly
        BananaYellow,               ///< \htmlonly <div style='background-color:#FFE135; width:50px;'>&nbsp;</div> \endhtmlonly
        BasketBeige,                ///< \htmlonly <div style='background-color:#C0A98B; width:50px;'>&nbsp;</div> \endhtmlonly
        BattleshipGrey,             ///< \htmlonly <div style='background-color:#848482; width:50px;'>&nbsp;</div> \endhtmlonly
        Bazaar,                     ///< \htmlonly <div style='background-color:#98777B; width:50px;'>&nbsp;</div> \endhtmlonly
        BeauBlue,                   ///< \htmlonly <div style='background-color:#BCD4E6; width:50px;'>&nbsp;</div> \endhtmlonly
        Beaver,                     ///< \htmlonly <div style='background-color:#9F8170; width:50px;'>&nbsp;</div> \endhtmlonly
        Beige,                      ///< \htmlonly <div style='background-color:#F5F5DC; width:50px;'>&nbsp;</div> \endhtmlonly
        BelvedereCream,             ///< \htmlonly <div style='background-color:#F0CDA0; width:50px;'>&nbsp;</div> \endhtmlonly
        Bisque,                     ///< \htmlonly <div style='background-color:#FFE4C4; width:50px;'>&nbsp;</div> \endhtmlonly
        Bistre,                     ///< \htmlonly <div style='background-color:#3D2B1F; width:50px;'>&nbsp;</div> \endhtmlonly
        Bittersweet,                ///< \htmlonly <div style='background-color:#FE6F5E; width:50px;'>&nbsp;</div> \endhtmlonly
        Black,                      ///< \htmlonly <div style='background-color:#000000; width:50px;'>&nbsp;</div> \endhtmlonly
        BlanchedAlmond,             ///< \htmlonly <div style='background-color:#FFEBCD; width:50px;'>&nbsp;</div> \endhtmlonly
        BleuDeFrance,               ///< \htmlonly <div style='background-color:#318CE7; width:50px;'>&nbsp;</div> \endhtmlonly
        BlizzardBlue,               ///< \htmlonly <div style='background-color:#ACE5EE; width:50px;'>&nbsp;</div> \endhtmlonly
        Blond,                      ///< \htmlonly <div style='background-color:#FAF0BE; width:50px;'>&nbsp;</div> \endhtmlonly
        Blue,                       ///< \htmlonly <div style='background-color:#0000FF; width:50px;'>&nbsp;</div> \endhtmlonly
        BlueBell,                   ///< \htmlonly <div style='background-color:#A2A2D0; width:50px;'>&nbsp;</div> \endhtmlonly
        BlueGray,                   ///< \htmlonly <div style='background-color:#6699CC; width:50px;'>&nbsp;</div> \endhtmlonly
        BlueGreen,                  ///< \htmlonly <div style='background-color:#0D98BA; width:50px;'>&nbsp;</div> \endhtmlonly
        BluePeacock,                ///< \htmlonly <div style='background-color:#014E4C; width:50px;'>&nbsp;</div> \endhtmlonly
        BluePurple,                 ///< \htmlonly <div style='background-color:#8A2BE2; width:50px;'>&nbsp;</div> \endhtmlonly
        BlueSky,                    ///< \htmlonly <div style='background-color:#ABD1C9; width:50px;'>&nbsp;</div> \endhtmlonly
        BlueViolet,                 ///< \htmlonly <div style='background-color:#8A2BE2; width:50px;'>&nbsp;</div> \endhtmlonly
        Blush,                      ///< \htmlonly <div style='background-color:#DE5D83; width:50px;'>&nbsp;</div> \endhtmlonly
        Bole,                       ///< \htmlonly <div style='background-color:#79443B; width:50px;'>&nbsp;</div> \endhtmlonly
        BondiBlue,                  ///< \htmlonly <div style='background-color:#0095B6; width:50px;'>&nbsp;</div> \endhtmlonly
        Bone,                       ///< \htmlonly <div style='background-color:#E3DAC9; width:50px;'>&nbsp;</div> \endhtmlonly
        BostonUniversityRed,        ///< \htmlonly <div style='background-color:#CC0000; width:50px;'>&nbsp;</div> \endhtmlonly
        BottleGreen,                ///< \htmlonly <div style='background-color:#006A4E; width:50px;'>&nbsp;</div> \endhtmlonly
        Boysenberry,                ///< \htmlonly <div style='background-color:#873260; width:50px;'>&nbsp;</div> \endhtmlonly
        BrandeisBlue,               ///< \htmlonly <div style='background-color:#0070FF; width:50px;'>&nbsp;</div> \endhtmlonly
        Brass,                      ///< \htmlonly <div style='background-color:#B5A642; width:50px;'>&nbsp;</div> \endhtmlonly
        BrickRed,                   ///< \htmlonly <div style='background-color:#CB4154; width:50px;'>&nbsp;</div> \endhtmlonly
        BrightCerulean,             ///< \htmlonly <div style='background-color:#1DACD6; width:50px;'>&nbsp;</div> \endhtmlonly
        BrightGreen,                ///< \htmlonly <div style='background-color:#66FF00; width:50px;'>&nbsp;</div> \endhtmlonly
        BrightLavender,             ///< \htmlonly <div style='background-color:#BF94E4; width:50px;'>&nbsp;</div> \endhtmlonly
        BrightMaroon,               ///< \htmlonly <div style='background-color:#C32148; width:50px;'>&nbsp;</div> \endhtmlonly
        BrightPink,                 ///< \htmlonly <div style='background-color:#FF007F; width:50px;'>&nbsp;</div> \endhtmlonly
        BrightTurquoise,            ///< \htmlonly <div style='background-color:#08E8DE; width:50px;'>&nbsp;</div> \endhtmlonly
        BrightUbe,                  ///< \htmlonly <div style='background-color:#D19FE8; width:50px;'>&nbsp;</div> \endhtmlonly
        BrilliantLavender,          ///< \htmlonly <div style='background-color:#F4BBFF; width:50px;'>&nbsp;</div> \endhtmlonly
        BrilliantRose,              ///< \htmlonly <div style='background-color:#FF55A3; width:50px;'>&nbsp;</div> \endhtmlonly
        BrinkPink,                  ///< \htmlonly <div style='background-color:#FB607F; width:50px;'>&nbsp;</div> \endhtmlonly
        BritishRacingGreen,         ///< \htmlonly <div style='background-color:#004225; width:50px;'>&nbsp;</div> \endhtmlonly
        Bronze,                     ///< \htmlonly <div style='background-color:#CD7F32; width:50px;'>&nbsp;</div> \endhtmlonly
        Brown,                      ///< \htmlonly <div style='background-color:#A52A2A; width:50px;'>&nbsp;</div> \endhtmlonly
        BubbleGum,                  ///< \htmlonly <div style='background-color:#FFC1CC; width:50px;'>&nbsp;</div> \endhtmlonly
        Bubbles,                    ///< \htmlonly <div style='background-color:#E7FEFF; width:50px;'>&nbsp;</div> \endhtmlonly
        Buff,                       ///< \htmlonly <div style='background-color:#F0DC82; width:50px;'>&nbsp;</div> \endhtmlonly
        BulgarianRose,              ///< \htmlonly <div style='background-color:#480607; width:50px;'>&nbsp;</div> \endhtmlonly
        BungalowBeige,              ///< \htmlonly <div style='background-color:#CDBFB0; width:50px;'>&nbsp;</div> \endhtmlonly
        Burgundy,                   ///< \htmlonly <div style='background-color:#800020; width:50px;'>&nbsp;</div> \endhtmlonly
        Burlywood,                  ///< \htmlonly <div style='background-color:#DEB887; width:50px;'>&nbsp;</div> \endhtmlonly
        BurntOrange,                ///< \htmlonly <div style='background-color:#CC5500; width:50px;'>&nbsp;</div> \endhtmlonly
        BurntSienna,                ///< \htmlonly <div style='background-color:#E97451; width:50px;'>&nbsp;</div> \endhtmlonly
        BurntUmber,                 ///< \htmlonly <div style='background-color:#8A3324; width:50px;'>&nbsp;</div> \endhtmlonly
        Byzantine,                  ///< \htmlonly <div style='background-color:#BD33A4; width:50px;'>&nbsp;</div> \endhtmlonly
        Byzantium,                  ///< \htmlonly <div style='background-color:#702963; width:50px;'>&nbsp;</div> \endhtmlonly
        Cadet,                      ///< \htmlonly <div style='background-color:#536872; width:50px;'>&nbsp;</div> \endhtmlonly
        CadetBlue,                  ///< \htmlonly <div style='background-color:#5F9EA0; width:50px;'>&nbsp;</div> \endhtmlonly
        CadetGrey,                  ///< \htmlonly <div style='background-color:#91A3B0; width:50px;'>&nbsp;</div> \endhtmlonly
        CadmiumGreen,               ///< \htmlonly <div style='background-color:#006B3C; width:50px;'>&nbsp;</div> \endhtmlonly
        CadmiumOrange,              ///< \htmlonly <div style='background-color:#ED872D; width:50px;'>&nbsp;</div> \endhtmlonly
        CadmiumRed,                 ///< \htmlonly <div style='background-color:#E30022; width:50px;'>&nbsp;</div> \endhtmlonly
        CadmiumYellow,              ///< \htmlonly <div style='background-color:#FFF600; width:50px;'>&nbsp;</div> \endhtmlonly
        CafeAuLait,                 ///< \htmlonly <div style='background-color:#A67B5B; width:50px;'>&nbsp;</div> \endhtmlonly
        CafeNoir,                   ///< \htmlonly <div style='background-color:#4B3621; width:50px;'>&nbsp;</div> \endhtmlonly
        CalPolyPomonaGreen,         ///< \htmlonly <div style='background-color:#1E4D2B; width:50px;'>&nbsp;</div> \endhtmlonly
        CambridgeBlue,              ///< \htmlonly <div style='background-color:#A3C1AD; width:50px;'>&nbsp;</div> \endhtmlonly
        Camel,                      ///< \htmlonly <div style='background-color:#C19A6B; width:50px;'>&nbsp;</div> \endhtmlonly
        CamouflageGreen,            ///< \htmlonly <div style='background-color:#78866B; width:50px;'>&nbsp;</div> \endhtmlonly
        Canary,                     ///< \htmlonly <div style='background-color:#FFFF99; width:50px;'>&nbsp;</div> \endhtmlonly
        CanaryYellow,               ///< \htmlonly <div style='background-color:#FFEF00; width:50px;'>&nbsp;</div> \endhtmlonly
        CandyAppleRed,              ///< \htmlonly <div style='background-color:#FF0800; width:50px;'>&nbsp;</div> \endhtmlonly
        CandyPink,                  ///< \htmlonly <div style='background-color:#E4717A; width:50px;'>&nbsp;</div> \endhtmlonly
        Capri,                      ///< \htmlonly <div style='background-color:#00BFFF; width:50px;'>&nbsp;</div> \endhtmlonly
        CaputMortuum,               ///< \htmlonly <div style='background-color:#592720; width:50px;'>&nbsp;</div> \endhtmlonly
        Cardinal,                   ///< \htmlonly <div style='background-color:#C41E3A; width:50px;'>&nbsp;</div> \endhtmlonly
        CaribbeanGreen,             ///< \htmlonly <div style='background-color:#00CC99; width:50px;'>&nbsp;</div> \endhtmlonly
        Carmine,                    ///< \htmlonly <div style='background-color:#FF0040; width:50px;'>&nbsp;</div> \endhtmlonly
        CarminePink,                ///< \htmlonly <div style='background-color:#EB4C42; width:50px;'>&nbsp;</div> \endhtmlonly
        CarmineRed,                 ///< \htmlonly <div style='background-color:#FF0038; width:50px;'>&nbsp;</div> \endhtmlonly
        CarnationPink,              ///< \htmlonly <div style='background-color:#FFA6C9; width:50px;'>&nbsp;</div> \endhtmlonly
        Carnelian,                  ///< \htmlonly <div style='background-color:#B31B1B; width:50px;'>&nbsp;</div> \endhtmlonly
        CarolinaBlue,               ///< \htmlonly <div style='background-color:#99BADD; width:50px;'>&nbsp;</div> \endhtmlonly
        CarrotOrange,               ///< \htmlonly <div style='background-color:#ED9121; width:50px;'>&nbsp;</div> \endhtmlonly
        CasaBlanca,                 ///< \htmlonly <div style='background-color:#EDE1CE; width:50px;'>&nbsp;</div> \endhtmlonly
        CascadeGreen,               ///< \htmlonly <div style='background-color:#ACB19F; width:50px;'>&nbsp;</div> \endhtmlonly
        Celadon,                    ///< \htmlonly <div style='background-color:#ACE1AF; width:50px;'>&nbsp;</div> \endhtmlonly
        Celeste,                    ///< \htmlonly <div style='background-color:#B2FFFF; width:50px;'>&nbsp;</div> \endhtmlonly
        CelestialBlue,              ///< \htmlonly <div style='background-color:#4997D0; width:50px;'>&nbsp;</div> \endhtmlonly
        Cerise,                     ///< \htmlonly <div style='background-color:#DE3163; width:50px;'>&nbsp;</div> \endhtmlonly
        CerisePink,                 ///< \htmlonly <div style='background-color:#EC3B83; width:50px;'>&nbsp;</div> \endhtmlonly
        Cerulean,                   ///< \htmlonly <div style='background-color:#007BA7; width:50px;'>&nbsp;</div> \endhtmlonly
        CeruleanBlue,               ///< \htmlonly <div style='background-color:#2A52BE; width:50px;'>&nbsp;</div> \endhtmlonly
        CGBlue,                     ///< \htmlonly <div style='background-color:#007AA5; width:50px;'>&nbsp;</div> \endhtmlonly
        CGRed,                      ///< \htmlonly <div style='background-color:#E03C31; width:50px;'>&nbsp;</div> \endhtmlonly
        Chamoisee,                  ///< \htmlonly <div style='background-color:#A0785A; width:50px;'>&nbsp;</div> \endhtmlonly
        Champagne,                  ///< \htmlonly <div style='background-color:#FAD6A5; width:50px;'>&nbsp;</div> \endhtmlonly
        Charcoal,                   ///< \htmlonly <div style='background-color:#36454F; width:50px;'>&nbsp;</div> \endhtmlonly
        Chartreuse,                 ///< \htmlonly <div style='background-color:#7FFF00; width:50px;'>&nbsp;</div> \endhtmlonly
        Cherry,                     ///< \htmlonly <div style='background-color:#DE3163; width:50px;'>&nbsp;</div> \endhtmlonly
        CherryBlossomPink,          ///< \htmlonly <div style='background-color:#FFB7C5; width:50px;'>&nbsp;</div> \endhtmlonly
        Chestnut,                   ///< \htmlonly <div style='background-color:#CD5C5C; width:50px;'>&nbsp;</div> \endhtmlonly
        ChineseRed,                 ///< \htmlonly <div style='background-color:#9E3E33; width:50px;'>&nbsp;</div> \endhtmlonly
        Chocolate,                  ///< \htmlonly <div style='background-color:#D2691E; width:50px;'>&nbsp;</div> \endhtmlonly
        ChromeYellow,               ///< \htmlonly <div style='background-color:#FFA700; width:50px;'>&nbsp;</div> \endhtmlonly
        Cinereous,                  ///< \htmlonly <div style='background-color:#98817B; width:50px;'>&nbsp;</div> \endhtmlonly
        Cinnabar,                   ///< \htmlonly <div style='background-color:#E34234; width:50px;'>&nbsp;</div> \endhtmlonly
        Cinnamon,                   ///< \htmlonly <div style='background-color:#D2691E; width:50px;'>&nbsp;</div> \endhtmlonly
        Citrine,                    ///< \htmlonly <div style='background-color:#E4D00A; width:50px;'>&nbsp;</div> \endhtmlonly
        ClassicFrenchGray,          ///< \htmlonly <div style='background-color:#888782; width:50px;'>&nbsp;</div> \endhtmlonly
        ClassicRose,                ///< \htmlonly <div style='background-color:#FBCCE7; width:50px;'>&nbsp;</div> \endhtmlonly
        Cobalt,                     ///< \htmlonly <div style='background-color:#0047AB; width:50px;'>&nbsp;</div> \endhtmlonly
        CocoaBrown,                 ///< \htmlonly <div style='background-color:#D2691E; width:50px;'>&nbsp;</div> \endhtmlonly
        Coffee,                     ///< \htmlonly <div style='background-color:#6F4E37; width:50px;'>&nbsp;</div> \endhtmlonly
        ColonnadeGray,              ///< \htmlonly <div style='background-color:#C6C0B6; width:50px;'>&nbsp;</div> \endhtmlonly
        ColumbiaBlue,               ///< \htmlonly <div style='background-color:#9BDDFF; width:50px;'>&nbsp;</div> \endhtmlonly
        CoolBlack,                  ///< \htmlonly <div style='background-color:#002E63; width:50px;'>&nbsp;</div> \endhtmlonly
        CooledBlue,                 ///< \htmlonly <div style='background-color:#75B9AE; width:50px;'>&nbsp;</div> \endhtmlonly
        CoolGrey,                   ///< \htmlonly <div style='background-color:#8C92AC; width:50px;'>&nbsp;</div> \endhtmlonly
        CopenBlue,                  ///< \htmlonly <div style='background-color:#C2CCC4; width:50px;'>&nbsp;</div> \endhtmlonly
        Copper,                     ///< \htmlonly <div style='background-color:#B87333; width:50px;'>&nbsp;</div> \endhtmlonly
        CopperRose,                 ///< \htmlonly <div style='background-color:#996666; width:50px;'>&nbsp;</div> \endhtmlonly
        Coquelicot,                 ///< \htmlonly <div style='background-color:#FF3800; width:50px;'>&nbsp;</div> \endhtmlonly
        Coral,                      ///< \htmlonly <div style='background-color:#FF7F50; width:50px;'>&nbsp;</div> \endhtmlonly
        CoralPink,                  ///< \htmlonly <div style='background-color:#F88379; width:50px;'>&nbsp;</div> \endhtmlonly
        CoralRed,                   ///< \htmlonly <div style='background-color:#FF4040; width:50px;'>&nbsp;</div> \endhtmlonly
        Cordovan,                   ///< \htmlonly <div style='background-color:#893F45; width:50px;'>&nbsp;</div> \endhtmlonly
        Corn,                       ///< \htmlonly <div style='background-color:#FBEC5D; width:50px;'>&nbsp;</div> \endhtmlonly
        CornellRed,                 ///< \htmlonly <div style='background-color:#B31B1B; width:50px;'>&nbsp;</div> \endhtmlonly
        Cornflower,                 ///< \htmlonly <div style='background-color:#9ACEEB; width:50px;'>&nbsp;</div> \endhtmlonly
        CornflowerBlue,             ///< \htmlonly <div style='background-color:#6495ED; width:50px;'>&nbsp;</div> \endhtmlonly
        Cornsilk,                   ///< \htmlonly <div style='background-color:#FFF8DC; width:50px;'>&nbsp;</div> \endhtmlonly
        CosmicLatte,                ///< \htmlonly <div style='background-color:#FFF8E7; width:50px;'>&nbsp;</div> \endhtmlonly
        CottonCandy,                ///< \htmlonly <div style='background-color:#FFBCD9; width:50px;'>&nbsp;</div> \endhtmlonly
        Cream,                      ///< \htmlonly <div style='background-color:#FFFDD0; width:50px;'>&nbsp;</div> \endhtmlonly
        Crimson,                    ///< \htmlonly <div style='background-color:#DC143C; width:50px;'>&nbsp;</div> \endhtmlonly
        CrimsonGlory,               ///< \htmlonly <div style='background-color:#BE0032; width:50px;'>&nbsp;</div> \endhtmlonly
        CrimsonRed,                 ///< \htmlonly <div style='background-color:#990000; width:50px;'>&nbsp;</div> \endhtmlonly
        Cyan,                       ///< \htmlonly <div style='background-color:#00FFFF; width:50px;'>&nbsp;</div> \endhtmlonly
        Daffodil,                   ///< \htmlonly <div style='background-color:#FFFF31; width:50px;'>&nbsp;</div> \endhtmlonly
        Daisy,                      ///< \htmlonly <div style='background-color:#FED340; width:50px;'>&nbsp;</div> \endhtmlonly
        Dandelion,                  ///< \htmlonly <div style='background-color:#F0E130; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkBlue,                   ///< \htmlonly <div style='background-color:#00008B; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkBrown,                  ///< \htmlonly <div style='background-color:#654321; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkByzantium,              ///< \htmlonly <div style='background-color:#5D3954; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkCandyAppleRed,          ///< \htmlonly <div style='background-color:#A40000; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkCerulean,               ///< \htmlonly <div style='background-color:#08457E; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkChestnut,               ///< \htmlonly <div style='background-color:#986960; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkCoral,                  ///< \htmlonly <div style='background-color:#CD5B45; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkCyan,                   ///< \htmlonly <div style='background-color:#008B8B; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkElectricBlue,           ///< \htmlonly <div style='background-color:#536878; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkGoldenrod,              ///< \htmlonly <div style='background-color:#B8860B; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkGray,                   ///< \htmlonly <div style='background-color:#A9A9A9; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkGreen,                  ///< \htmlonly <div style='background-color:#013220; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkJungleGreen,            ///< \htmlonly <div style='background-color:#1A2421; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkKhaki,                  ///< \htmlonly <div style='background-color:#BDB76B; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkLava,                   ///< \htmlonly <div style='background-color:#483C32; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkLavender,               ///< \htmlonly <div style='background-color:#734F96; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkMagenta,                ///< \htmlonly <div style='background-color:#8B008B; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkMidnightBlue,           ///< \htmlonly <div style='background-color:#003366; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkOliveGreen,             ///< \htmlonly <div style='background-color:#556B2F; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkOrange,                 ///< \htmlonly <div style='background-color:#FF8C00; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkOrchid,                 ///< \htmlonly <div style='background-color:#9932CC; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkPastelBlue,             ///< \htmlonly <div style='background-color:#779ECB; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkPastelGreen,            ///< \htmlonly <div style='background-color:#03C03C; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkPastelPurple,           ///< \htmlonly <div style='background-color:#966FD6; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkPastelRed,              ///< \htmlonly <div style='background-color:#C23B22; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkPink,                   ///< \htmlonly <div style='background-color:#E75480; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkPowderBlue,             ///< \htmlonly <div style='background-color:#003399; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkRaspberry,              ///< \htmlonly <div style='background-color:#872657; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkRed,                    ///< \htmlonly <div style='background-color:#8B0000; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkSalmon,                 ///< \htmlonly <div style='background-color:#E9967A; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkScarlet,                ///< \htmlonly <div style='background-color:#560319; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkSeaGreen,               ///< \htmlonly <div style='background-color:#8FBC8F; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkSienna,                 ///< \htmlonly <div style='background-color:#3C1414; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkSlateBlue,              ///< \htmlonly <div style='background-color:#483D8B; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkSlateGray,              ///< \htmlonly <div style='background-color:#2F4F4F; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkSpringGreen,            ///< \htmlonly <div style='background-color:#177245; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkTan,                    ///< \htmlonly <div style='background-color:#918151; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkTangerine,              ///< \htmlonly <div style='background-color:#FFA812; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkTaupe,                  ///< \htmlonly <div style='background-color:#483C32; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkTerraCotta,             ///< \htmlonly <div style='background-color:#CC4E5C; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkTurquoise,              ///< \htmlonly <div style='background-color:#00CED1; width:50px;'>&nbsp;</div> \endhtmlonly
        DarkViolet,                 ///< \htmlonly <div style='background-color:#9400D3; width:50px;'>&nbsp;</div> \endhtmlonly
        DartmouthGreen,             ///< \htmlonly <div style='background-color:#00693E; width:50px;'>&nbsp;</div> \endhtmlonly
        DavyGrey,                   ///< \htmlonly <div style='background-color:#555555; width:50px;'>&nbsp;</div> \endhtmlonly
        DebianRed,                  ///< \htmlonly <div style='background-color:#D70A53; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepCarmine,                ///< \htmlonly <div style='background-color:#A9203E; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepCarminePink,            ///< \htmlonly <div style='background-color:#EF3038; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepCarrotOrange,           ///< \htmlonly <div style='background-color:#E9692C; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepCerise,                 ///< \htmlonly <div style='background-color:#DA3287; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepChampagne,              ///< \htmlonly <div style='background-color:#FAD6A5; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepChestnut,               ///< \htmlonly <div style='background-color:#B94E48; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepCoffee,                 ///< \htmlonly <div style='background-color:#704241; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepFuchsia,                ///< \htmlonly <div style='background-color:#C154C1; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepJungleGreen,            ///< \htmlonly <div style='background-color:#004B49; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepLilac,                  ///< \htmlonly <div style='background-color:#9955BB; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepMagenta,                ///< \htmlonly <div style='background-color:#CC00CC; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepMaroon,                 ///< \htmlonly <div style='background-color:#623F45; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepPeach,                  ///< \htmlonly <div style='background-color:#FFCBA4; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepPink,                   ///< \htmlonly <div style='background-color:#FF1493; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepSaffron,                ///< \htmlonly <div style='background-color:#FF9933; width:50px;'>&nbsp;</div> \endhtmlonly
        DeepSkyBlue,                ///< \htmlonly <div style='background-color:#00BFFF; width:50px;'>&nbsp;</div> \endhtmlonly
        Denim,                      ///< \htmlonly <div style='background-color:#1560BD; width:50px;'>&nbsp;</div> \endhtmlonly
        Desert,                     ///< \htmlonly <div style='background-color:#C19A6B; width:50px;'>&nbsp;</div> \endhtmlonly
        DesertSand,                 ///< \htmlonly <div style='background-color:#EDC9AF; width:50px;'>&nbsp;</div> \endhtmlonly
        DimGray,                    ///< \htmlonly <div style='background-color:#696969; width:50px;'>&nbsp;</div> \endhtmlonly
        DodgerBlue,                 ///< \htmlonly <div style='background-color:#1E90FF; width:50px;'>&nbsp;</div> \endhtmlonly
        DogwoodRose,                ///< \htmlonly <div style='background-color:#D71868; width:50px;'>&nbsp;</div> \endhtmlonly
        DollarBill,                 ///< \htmlonly <div style='background-color:#85BB65; width:50px;'>&nbsp;</div> \endhtmlonly
        DoverWhite,                 ///< \htmlonly <div style='background-color:#F0EADC; width:50px;'>&nbsp;</div> \endhtmlonly
        Drab,                       ///< \htmlonly <div style='background-color:#967117; width:50px;'>&nbsp;</div> \endhtmlonly
        DressyRose,                 ///< \htmlonly <div style='background-color:#B89D9A; width:50px;'>&nbsp;</div> \endhtmlonly
        DukeBlue,                   ///< \htmlonly <div style='background-color:#00009C; width:50px;'>&nbsp;</div> \endhtmlonly
        EarthYellow,                ///< \htmlonly <div style='background-color:#E1A95F; width:50px;'>&nbsp;</div> \endhtmlonly
        Ecru,                       ///< \htmlonly <div style='background-color:#C2B280; width:50px;'>&nbsp;</div> \endhtmlonly
        EdgyGold,                   ///< \htmlonly <div style='background-color:#B1975F; width:50px;'>&nbsp;</div> \endhtmlonly
        Eggplant,                   ///< \htmlonly <div style='background-color:#614051; width:50px;'>&nbsp;</div> \endhtmlonly
        Eggshell,                   ///< \htmlonly <div style='background-color:#F0EAD6; width:50px;'>&nbsp;</div> \endhtmlonly
        EgyptianBlue,               ///< \htmlonly <div style='background-color:#1034A6; width:50px;'>&nbsp;</div> \endhtmlonly
        ElectricBlue,               ///< \htmlonly <div style='background-color:#7DF9FF; width:50px;'>&nbsp;</div> \endhtmlonly
        ElectricCrimson,            ///< \htmlonly <div style='background-color:#FF003F; width:50px;'>&nbsp;</div> \endhtmlonly
        ElectricCyan,               ///< \htmlonly <div style='background-color:#00FFFF; width:50px;'>&nbsp;</div> \endhtmlonly
        ElectricGreen,              ///< \htmlonly <div style='background-color:#00FF00; width:50px;'>&nbsp;</div> \endhtmlonly
        ElectricIndigo,             ///< \htmlonly <div style='background-color:#6F00FF; width:50px;'>&nbsp;</div> \endhtmlonly
        ElectricLavender,           ///< \htmlonly <div style='background-color:#F4BBFF; width:50px;'>&nbsp;</div> \endhtmlonly
        ElectricLime,               ///< \htmlonly <div style='background-color:#CCFF00; width:50px;'>&nbsp;</div> \endhtmlonly
        ElectricPurple,             ///< \htmlonly <div style='background-color:#BF00FF; width:50px;'>&nbsp;</div> \endhtmlonly
        ElectricUltramarine,        ///< \htmlonly <div style='background-color:#3F00FF; width:50px;'>&nbsp;</div> \endhtmlonly
        ElectricViolet,             ///< \htmlonly <div style='background-color:#8F00FF; width:50px;'>&nbsp;</div> \endhtmlonly
        ElectricYellow,             ///< \htmlonly <div style='background-color:#FFFF00; width:50px;'>&nbsp;</div> \endhtmlonly
        Emerald,                    ///< \htmlonly <div style='background-color:#50C878; width:50px;'>&nbsp;</div> \endhtmlonly
        EtonBlue,                   ///< \htmlonly <div style='background-color:#96C8A2; width:50px;'>&nbsp;</div> \endhtmlonly
        EvergreenFog,               ///< \htmlonly <div style='background-color:#95978A; width:50px;'>&nbsp;</div> \endhtmlonly
        ExuberantPink,              ///< \htmlonly <div style='background-color:#B54D7F; width:50px;'>&nbsp;</div> \endhtmlonly
        Fallow,                     ///< \htmlonly <div style='background-color:#C19A6B; width:50px;'>&nbsp;</div> \endhtmlonly
        FaluRed,                    ///< \htmlonly <div style='background-color:#801818; width:50px;'>&nbsp;</div> \endhtmlonly
        Famous,                     ///< \htmlonly <div style='background-color:#FF00FF; width:50px;'>&nbsp;</div> \endhtmlonly
        Fandango,                   ///< \htmlonly <div style='background-color:#B53389; width:50px;'>&nbsp;</div> \endhtmlonly
        FashionFuchsia,             ///< \htmlonly <div style='background-color:#F400A1; width:50px;'>&nbsp;</div> \endhtmlonly
        FavoriteJeans,              ///< \htmlonly <div style='background-color:#8AA3B1; width:50px;'>&nbsp;</div> \endhtmlonly
        Fawn,                       ///< \htmlonly <div style='background-color:#E5AA70; width:50px;'>&nbsp;</div> \endhtmlonly
        Feldgrau,                   ///< \htmlonly <div style='background-color:#4D5D53; width:50px;'>&nbsp;</div> \endhtmlonly
        Fern,                       ///< \htmlonly <div style='background-color:#71BC78; width:50px;'>&nbsp;</div> \endhtmlonly
        FernGreen,                  ///< \htmlonly <div style='background-color:#4F7942; width:50px;'>&nbsp;</div> \endhtmlonly
        FerrariRed,                 ///< \htmlonly <div style='background-color:#FF2800; width:50px;'>&nbsp;</div> \endhtmlonly
        FieldDrab,                  ///< \htmlonly <div style='background-color:#6C541E; width:50px;'>&nbsp;</div> \endhtmlonly
        Firebrick,                  ///< \htmlonly <div style='background-color:#B22222; width:50px;'>&nbsp;</div> \endhtmlonly
        FireEngineRed,              ///< \htmlonly <div style='background-color:#CE2029; width:50px;'>&nbsp;</div> \endhtmlonly
        Fireweed,                   ///< \htmlonly <div style='background-color:#7B3730; width:50px;'>&nbsp;</div> \endhtmlonly
        Flame,                      ///< \htmlonly <div style='background-color:#E25822; width:50px;'>&nbsp;</div> \endhtmlonly
        FlamingoPink,               ///< \htmlonly <div style='background-color:#FC8EAC; width:50px;'>&nbsp;</div> \endhtmlonly
        FlatteringPeach,            ///< \htmlonly <div style='background-color:#F4D3B3; width:50px;'>&nbsp;</div> \endhtmlonly
        Flavescent,                 ///< \htmlonly <div style='background-color:#F7E98E; width:50px;'>&nbsp;</div> \endhtmlonly
        Flax,                       ///< \htmlonly <div style='background-color:#EEDC82; width:50px;'>&nbsp;</div> \endhtmlonly
        FloralWhite,                ///< \htmlonly <div style='background-color:#FFFAF0; width:50px;'>&nbsp;</div> \endhtmlonly
        FluorescentOrange,          ///< \htmlonly <div style='background-color:#FFBF00; width:50px;'>&nbsp;</div> \endhtmlonly
        FluorescentPink,            ///< \htmlonly <div style='background-color:#FF1493; width:50px;'>&nbsp;</div> \endhtmlonly
        FluorescentYellow,          ///< \htmlonly <div style='background-color:#CCFF00; width:50px;'>&nbsp;</div> \endhtmlonly
        FolksyGold,                 ///< \htmlonly <div style='background-color:#D69969; width:50px;'>&nbsp;</div> \endhtmlonly
        Folly,                      ///< \htmlonly <div style='background-color:#FF004F; width:50px;'>&nbsp;</div> \endhtmlonly
        ForestGreen,                ///< \htmlonly <div style='background-color:#228B22; width:50px;'>&nbsp;</div> \endhtmlonly
        ForgetMeNot,                ///< \htmlonly <div style='background-color:#716998; width:50px;'>&nbsp;</div> \endhtmlonly
        FrenchBeige,                ///< \htmlonly <div style='background-color:#A67B5B; width:50px;'>&nbsp;</div> \endhtmlonly
        FrenchBlue,                 ///< \htmlonly <div style='background-color:#0072BB; width:50px;'>&nbsp;</div> \endhtmlonly
        FrenchLilac,                ///< \htmlonly <div style='background-color:#86608E; width:50px;'>&nbsp;</div> \endhtmlonly
        FrenchRose,                 ///< \htmlonly <div style='background-color:#F64A8A; width:50px;'>&nbsp;</div> \endhtmlonly
        Frolic,                     ///< \htmlonly <div style='background-color:#D9C661; width:50px;'>&nbsp;</div> \endhtmlonly
        Frostwork,                  ///< \htmlonly <div style='background-color:#CBD0C2; width:50px;'>&nbsp;</div> \endhtmlonly
        Fuchsia,                    ///< \htmlonly <div style='background-color:#FF00FF; width:50px;'>&nbsp;</div> \endhtmlonly
        FuchsiaPink,                ///< \htmlonly <div style='background-color:#FF77FF; width:50px;'>&nbsp;</div> \endhtmlonly
        Fulvous,                    ///< \htmlonly <div style='background-color:#E48400; width:50px;'>&nbsp;</div> \endhtmlonly
        FuzzyWuzzy,                 ///< \htmlonly <div style='background-color:#CC6666; width:50px;'>&nbsp;</div> \endhtmlonly
        Gainsboro,                  ///< \htmlonly <div style='background-color:#DCDCDC; width:50px;'>&nbsp;</div> \endhtmlonly
        Gamboge,                    ///< \htmlonly <div style='background-color:#E49B0F; width:50px;'>&nbsp;</div> \endhtmlonly
        GhostWhite,                 ///< \htmlonly <div style='background-color:#F8F8FF; width:50px;'>&nbsp;</div> \endhtmlonly
        Ginger,                     ///< \htmlonly <div style='background-color:#B06500; width:50px;'>&nbsp;</div> \endhtmlonly
        Glaucous,                   ///< \htmlonly <div style='background-color:#6082B6; width:50px;'>&nbsp;</div> \endhtmlonly
        Glitter,                    ///< \htmlonly <div style='background-color:#E6E8FA; width:50px;'>&nbsp;</div> \endhtmlonly
        Gold,                       ///< \htmlonly <div style='background-color:#FFD700; width:50px;'>&nbsp;</div> \endhtmlonly
        GoldenBrown,                ///< \htmlonly <div style='background-color:#996515; width:50px;'>&nbsp;</div> \endhtmlonly
        GoldenPoppy,                ///< \htmlonly <div style='background-color:#FCC200; width:50px;'>&nbsp;</div> \endhtmlonly
        Goldenrod,                  ///< \htmlonly <div style='background-color:#DAA520; width:50px;'>&nbsp;</div> \endhtmlonly
        GoldenYellow,               ///< \htmlonly <div style='background-color:#FFDF00; width:50px;'>&nbsp;</div> \endhtmlonly
        GrannySmithApple,           ///< \htmlonly <div style='background-color:#A8E4A0; width:50px;'>&nbsp;</div> \endhtmlonly
        Gray,                       ///< \htmlonly <div style='background-color:#808080; width:50px;'>&nbsp;</div> \endhtmlonly
        GrayAsparagus,              ///< \htmlonly <div style='background-color:#465945; width:50px;'>&nbsp;</div> \endhtmlonly
        Green,                      ///< \htmlonly <div style='background-color:#00FF00; width:50px;'>&nbsp;</div> \endhtmlonly
        GreenBlue,                  ///< \htmlonly <div style='background-color:#1164B4; width:50px;'>&nbsp;</div> \endhtmlonly
        GreenYellow,                ///< \htmlonly <div style='background-color:#ADFF2F; width:50px;'>&nbsp;</div> \endhtmlonly
        Grullo,                     ///< \htmlonly <div style='background-color:#A99A86; width:50px;'>&nbsp;</div> \endhtmlonly
        GuppieGreen,                ///< \htmlonly <div style='background-color:#00FF7F; width:50px;'>&nbsp;</div> \endhtmlonly
        HalayaUbe,                  ///< \htmlonly <div style='background-color:#663854; width:50px;'>&nbsp;</div> \endhtmlonly
        HanBlue,                    ///< \htmlonly <div style='background-color:#446CCF; width:50px;'>&nbsp;</div> \endhtmlonly
        HanPurple,                  ///< \htmlonly <div style='background-color:#5218FA; width:50px;'>&nbsp;</div> \endhtmlonly
        HansaYellow,                ///< \htmlonly <div style='background-color:#E9D66B; width:50px;'>&nbsp;</div> \endhtmlonly
        Harlequin,                  ///< \htmlonly <div style='background-color:#3FFF00; width:50px;'>&nbsp;</div> \endhtmlonly
        HarvardCrimson,             ///< \htmlonly <div style='background-color:#C90016; width:50px;'>&nbsp;</div> \endhtmlonly
        HarvestGold,                ///< \htmlonly <div style='background-color:#DA9100; width:50px;'>&nbsp;</div> \endhtmlonly
        HeartGold,                  ///< \htmlonly <div style='background-color:#808000; width:50px;'>&nbsp;</div> \endhtmlonly
        Heliotrope,                 ///< \htmlonly <div style='background-color:#DF73FF; width:50px;'>&nbsp;</div> \endhtmlonly
        HickorySmoke,               ///< \htmlonly <div style='background-color:#564537; width:50px;'>&nbsp;</div> \endhtmlonly
        HolidayTurquoise,           ///< \htmlonly <div style='background-color:#8AC6BD; width:50px;'>&nbsp;</div> \endhtmlonly
        HollywoodCerise,            ///< \htmlonly <div style='background-color:#F400A1; width:50px;'>&nbsp;</div> \endhtmlonly
        Honeydew,                   ///< \htmlonly <div style='background-color:#F0FFF0; width:50px;'>&nbsp;</div> \endhtmlonly
        HookerGreen,                ///< \htmlonly <div style='background-color:#49796B; width:50px;'>&nbsp;</div> \endhtmlonly
        HotMagenta,                 ///< \htmlonly <div style='background-color:#FF1DCE; width:50px;'>&nbsp;</div> \endhtmlonly
        HotPink,                    ///< \htmlonly <div style='background-color:#FF69B4; width:50px;'>&nbsp;</div> \endhtmlonly
        HunterGreen,                ///< \htmlonly <div style='background-color:#355E3B; width:50px;'>&nbsp;</div> \endhtmlonly
        Icterine,                   ///< \htmlonly <div style='background-color:#FCF75E; width:50px;'>&nbsp;</div> \endhtmlonly
        Inchworm,                   ///< \htmlonly <div style='background-color:#B2EC5D; width:50px;'>&nbsp;</div> \endhtmlonly
        IndiaGreen,                 ///< \htmlonly <div style='background-color:#138808; width:50px;'>&nbsp;</div> \endhtmlonly
        IndianRed,                  ///< \htmlonly <div style='background-color:#CD5C5C; width:50px;'>&nbsp;</div> \endhtmlonly
        IndianYellow,               ///< \htmlonly <div style='background-color:#E3A857; width:50px;'>&nbsp;</div> \endhtmlonly
        Indigo,                     ///< \htmlonly <div style='background-color:#4B0082; width:50px;'>&nbsp;</div> \endhtmlonly
        InternationalKleinBlue,     ///< \htmlonly <div style='background-color:#002FA7; width:50px;'>&nbsp;</div> \endhtmlonly
        InternationalOrange,        ///< \htmlonly <div style='background-color:#FF4F00; width:50px;'>&nbsp;</div> \endhtmlonly
        Iris,                       ///< \htmlonly <div style='background-color:#5A4FCF; width:50px;'>&nbsp;</div> \endhtmlonly
        Isabelline,                 ///< \htmlonly <div style='background-color:#F4F0EC; width:50px;'>&nbsp;</div> \endhtmlonly
        IslamicGreen,               ///< \htmlonly <div style='background-color:#009000; width:50px;'>&nbsp;</div> \endhtmlonly
        Ivory,                      ///< \htmlonly <div style='background-color:#FFFFF0; width:50px;'>&nbsp;</div> \endhtmlonly
        Jade,                       ///< \htmlonly <div style='background-color:#00A86B; width:50px;'>&nbsp;</div> \endhtmlonly
        Jasmine,                    ///< \htmlonly <div style='background-color:#F8DE7E; width:50px;'>&nbsp;</div> \endhtmlonly
        Jasper,                     ///< \htmlonly <div style='background-color:#D73B3E; width:50px;'>&nbsp;</div> \endhtmlonly
        JazzAgeCoral,               ///< \htmlonly <div style='background-color:#F1BFB1; width:50px;'>&nbsp;</div> \endhtmlonly
        JazzberryJam,               ///< \htmlonly <div style='background-color:#A50B5E; width:50px;'>&nbsp;</div> \endhtmlonly
        Jonquil,                    ///< \htmlonly <div style='background-color:#FADA5E; width:50px;'>&nbsp;</div> \endhtmlonly
        JuneBud,                    ///< \htmlonly <div style='background-color:#BDDA57; width:50px;'>&nbsp;</div> \endhtmlonly
        JungleGreen,                ///< \htmlonly <div style='background-color:#29AB87; width:50px;'>&nbsp;</div> \endhtmlonly
        JuteBrown,                  ///< \htmlonly <div style='background-color:#815D40; width:50px;'>&nbsp;</div> \endhtmlonly
        KellyGreen,                 ///< \htmlonly <div style='background-color:#4CBB17; width:50px;'>&nbsp;</div> \endhtmlonly
        Khaki,                      ///< \htmlonly <div style='background-color:#C3B091; width:50px;'>&nbsp;</div> \endhtmlonly
        KilimBeige,                 ///< \htmlonly <div style='background-color:#D7C5AE; width:50px;'>&nbsp;</div> \endhtmlonly
        KuCrimson,                  ///< \htmlonly <div style='background-color:#E8000D; width:50px;'>&nbsp;</div> \endhtmlonly
        LanguidLavender,            ///< \htmlonly <div style='background-color:#D6CADD; width:50px;'>&nbsp;</div> \endhtmlonly
        LapisLazuli,                ///< \htmlonly <div style='background-color:#26619C; width:50px;'>&nbsp;</div> \endhtmlonly
        LaSalleGreen,               ///< \htmlonly <div style='background-color:#087830; width:50px;'>&nbsp;</div> \endhtmlonly
        LaserLemon,                 ///< \htmlonly <div style='background-color:#FEFE22; width:50px;'>&nbsp;</div> \endhtmlonly
        Latte,                      ///< \htmlonly <div style='background-color:#BAA185; width:50px;'>&nbsp;</div> \endhtmlonly
        LaurelGreen,                ///< \htmlonly <div style='background-color:#A9BA9D; width:50px;'>&nbsp;</div> \endhtmlonly
        Lava,                       ///< \htmlonly <div style='background-color:#CF1020; width:50px;'>&nbsp;</div> \endhtmlonly
        Lavender,                   ///< \htmlonly <div style='background-color:#E6E6FA; width:50px;'>&nbsp;</div> \endhtmlonly
        LavenderBlue,               ///< \htmlonly <div style='background-color:#CCCCFF; width:50px;'>&nbsp;</div> \endhtmlonly
        LavenderBlush,              ///< \htmlonly <div style='background-color:#FFF0F5; width:50px;'>&nbsp;</div> \endhtmlonly
        LavenderGray,               ///< \htmlonly <div style='background-color:#C4C3D0; width:50px;'>&nbsp;</div> \endhtmlonly
        LavenderIndigo,             ///< \htmlonly <div style='background-color:#9457EB; width:50px;'>&nbsp;</div> \endhtmlonly
        LavenderMagenta,            ///< \htmlonly <div style='background-color:#EE82EE; width:50px;'>&nbsp;</div> \endhtmlonly
        LavenderMist,               ///< \htmlonly <div style='background-color:#E6E6FA; width:50px;'>&nbsp;</div> \endhtmlonly
        LavenderPink,               ///< \htmlonly <div style='background-color:#FBAED2; width:50px;'>&nbsp;</div> \endhtmlonly
        LavenderPurple,             ///< \htmlonly <div style='background-color:#967BB6; width:50px;'>&nbsp;</div> \endhtmlonly
        LavenderRose,               ///< \htmlonly <div style='background-color:#FBA0E3; width:50px;'>&nbsp;</div> \endhtmlonly
        LawnGreen,                  ///< \htmlonly <div style='background-color:#7CFC00; width:50px;'>&nbsp;</div> \endhtmlonly
        Lemon,                      ///< \htmlonly <div style='background-color:#FFF700; width:50px;'>&nbsp;</div> \endhtmlonly
        LemonChiffon,               ///< \htmlonly <div style='background-color:#FFFACD; width:50px;'>&nbsp;</div> \endhtmlonly
        LemonLime,                  ///< \htmlonly <div style='background-color:#BFFF00; width:50px;'>&nbsp;</div> \endhtmlonly
        LemonYellow,                ///< \htmlonly <div style='background-color:#FFF44F; width:50px;'>&nbsp;</div> \endhtmlonly
        LightApricot,               ///< \htmlonly <div style='background-color:#FDD5B1; width:50px;'>&nbsp;</div> \endhtmlonly
        LightBlue,                  ///< \htmlonly <div style='background-color:#ADD8E6; width:50px;'>&nbsp;</div> \endhtmlonly
        LightBrown,                 ///< \htmlonly <div style='background-color:#B5651D; width:50px;'>&nbsp;</div> \endhtmlonly
        LightCarminePink,           ///< \htmlonly <div style='background-color:#E66771; width:50px;'>&nbsp;</div> \endhtmlonly
        LightCoral,                 ///< \htmlonly <div style='background-color:#F08080; width:50px;'>&nbsp;</div> \endhtmlonly
        LightCornflowerBlue,        ///< \htmlonly <div style='background-color:#93CCEA; width:50px;'>&nbsp;</div> \endhtmlonly
        LightCrimson,               ///< \htmlonly <div style='background-color:#F56991; width:50px;'>&nbsp;</div> \endhtmlonly
        LightCyan,                  ///< \htmlonly <div style='background-color:#E0FFFF; width:50px;'>&nbsp;</div> \endhtmlonly
        LightFuchsiaPink,           ///< \htmlonly <div style='background-color:#F984EF; width:50px;'>&nbsp;</div> \endhtmlonly
        LightGoldenrodYellow,       ///< \htmlonly <div style='background-color:#FAFAD2; width:50px;'>&nbsp;</div> \endhtmlonly
        LightGray,                  ///< \htmlonly <div style='background-color:#D3D3D3; width:50px;'>&nbsp;</div> \endhtmlonly
        LightGreen,                 ///< \htmlonly <div style='background-color:#90EE90; width:50px;'>&nbsp;</div> \endhtmlonly
        LightKhaki,                 ///< \htmlonly <div style='background-color:#F0E68C; width:50px;'>&nbsp;</div> \endhtmlonly
        LightPastelPurple,          ///< \htmlonly <div style='background-color:#B19CD9; width:50px;'>&nbsp;</div> \endhtmlonly
        LightPink,                  ///< \htmlonly <div style='background-color:#FFB6C1; width:50px;'>&nbsp;</div> \endhtmlonly
        LightSalmon,                ///< \htmlonly <div style='background-color:#FFA07A; width:50px;'>&nbsp;</div> \endhtmlonly
        LightSalmonPink,            ///< \htmlonly <div style='background-color:#FF9999; width:50px;'>&nbsp;</div> \endhtmlonly
        LightSeaGreen,              ///< \htmlonly <div style='background-color:#20B2AA; width:50px;'>&nbsp;</div> \endhtmlonly
        LightSkyBlue,               ///< \htmlonly <div style='background-color:#87CEFA; width:50px;'>&nbsp;</div> \endhtmlonly
        LightSlateGray,             ///< \htmlonly <div style='background-color:#778899; width:50px;'>&nbsp;</div> \endhtmlonly
        LightTaupe,                 ///< \htmlonly <div style='background-color:#B38B6D; width:50px;'>&nbsp;</div> \endhtmlonly
        LightThulianPink,           ///< \htmlonly <div style='background-color:#E68FAC; width:50px;'>&nbsp;</div> \endhtmlonly
        LightYellow,                ///< \htmlonly <div style='background-color:#FFFFED; width:50px;'>&nbsp;</div> \endhtmlonly
        Lilac,                      ///< \htmlonly <div style='background-color:#C8A2C8; width:50px;'>&nbsp;</div> \endhtmlonly
        Lime,                       ///< \htmlonly <div style='background-color:#BFFF00; width:50px;'>&nbsp;</div> \endhtmlonly
        LimeGreen,                  ///< \htmlonly <div style='background-color:#32CD32; width:50px;'>&nbsp;</div> \endhtmlonly
        LincolnGreen,               ///< \htmlonly <div style='background-color:#195905; width:50px;'>&nbsp;</div> \endhtmlonly
        Linen,                      ///< \htmlonly <div style='background-color:#FAF0E6; width:50px;'>&nbsp;</div> \endhtmlonly
        Lion,                       ///< \htmlonly <div style='background-color:#C19A6B; width:50px;'>&nbsp;</div> \endhtmlonly
        Liver,                      ///< \htmlonly <div style='background-color:#534B4F; width:50px;'>&nbsp;</div> \endhtmlonly
        Lust,                       ///< \htmlonly <div style='background-color:#E62020; width:50px;'>&nbsp;</div> \endhtmlonly
        MacaroniAndCheese,          ///< \htmlonly <div style='background-color:#FFBD88; width:50px;'>&nbsp;</div> \endhtmlonly
        Magenta,                    ///< \htmlonly <div style='background-color:#FF00FF; width:50px;'>&nbsp;</div> \endhtmlonly
        MagicMint,                  ///< \htmlonly <div style='background-color:#AAF0D1; width:50px;'>&nbsp;</div> \endhtmlonly
        Magnolia,                   ///< \htmlonly <div style='background-color:#F8F4FF; width:50px;'>&nbsp;</div> \endhtmlonly
        Mahogany,                   ///< \htmlonly <div style='background-color:#C04000; width:50px;'>&nbsp;</div> \endhtmlonly
        Maize,                      ///< \htmlonly <div style='background-color:#FBEC5D; width:50px;'>&nbsp;</div> \endhtmlonly
        MajorelleBlue,              ///< \htmlonly <div style='background-color:#6050DC; width:50px;'>&nbsp;</div> \endhtmlonly
        Malachite,                  ///< \htmlonly <div style='background-color:#0BDA51; width:50px;'>&nbsp;</div> \endhtmlonly
        Manatee,                    ///< \htmlonly <div style='background-color:#979AAA; width:50px;'>&nbsp;</div> \endhtmlonly
        MangoTango,                 ///< \htmlonly <div style='background-color:#FF8243; width:50px;'>&nbsp;</div> \endhtmlonly
        Mantis,                     ///< \htmlonly <div style='background-color:#74C365; width:50px;'>&nbsp;</div> \endhtmlonly
        Maroon,                     ///< \htmlonly <div style='background-color:#800000; width:50px;'>&nbsp;</div> \endhtmlonly
        Mauve,                      ///< \htmlonly <div style='background-color:#E0B0FF; width:50px;'>&nbsp;</div> \endhtmlonly
        Mauvelous,                  ///< \htmlonly <div style='background-color:#EF98AA; width:50px;'>&nbsp;</div> \endhtmlonly
        MauveTaupe,                 ///< \htmlonly <div style='background-color:#915F6D; width:50px;'>&nbsp;</div> \endhtmlonly
        MayaBlue,                   ///< \htmlonly <div style='background-color:#73C2FB; width:50px;'>&nbsp;</div> \endhtmlonly
        MeatBrown,                  ///< \htmlonly <div style='background-color:#E5B73B; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumAquamarine,           ///< \htmlonly <div style='background-color:#66DDAA; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumBlue,                 ///< \htmlonly <div style='background-color:#0000CD; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumCandyAppleRed,        ///< \htmlonly <div style='background-color:#E2062C; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumCarmine,              ///< \htmlonly <div style='background-color:#AF4035; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumChampagne,            ///< \htmlonly <div style='background-color:#F3E5AB; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumElectricBlue,         ///< \htmlonly <div style='background-color:#035096; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumJungleGreen,          ///< \htmlonly <div style='background-color:#1C352D; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumLavenderMagenta,      ///< \htmlonly <div style='background-color:#DDA0DD; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumOrchid,               ///< \htmlonly <div style='background-color:#BA55D3; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumPersianBlue,          ///< \htmlonly <div style='background-color:#0067A5; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumPurple,               ///< \htmlonly <div style='background-color:#9370DB; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumRedViolet,            ///< \htmlonly <div style='background-color:#BB3385; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumSeaGreen,             ///< \htmlonly <div style='background-color:#3CB371; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumSlateBlue,            ///< \htmlonly <div style='background-color:#7B68EE; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumSpringBud,            ///< \htmlonly <div style='background-color:#C9DC87; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumSpringGreen,          ///< \htmlonly <div style='background-color:#00FA9A; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumTaupe,                ///< \htmlonly <div style='background-color:#674C47; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumTealBlue,             ///< \htmlonly <div style='background-color:#0054B4; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumTurquoise,            ///< \htmlonly <div style='background-color:#48D1CC; width:50px;'>&nbsp;</div> \endhtmlonly
        MediumVioletRed,            ///< \htmlonly <div style='background-color:#C71585; width:50px;'>&nbsp;</div> \endhtmlonly
        Melon,                      ///< \htmlonly <div style='background-color:#FDBCB4; width:50px;'>&nbsp;</div> \endhtmlonly
        MidnightBlue,               ///< \htmlonly <div style='background-color:#191970; width:50px;'>&nbsp;</div> \endhtmlonly
        MidnightGreen,              ///< \htmlonly <div style='background-color:#004953; width:50px;'>&nbsp;</div> \endhtmlonly
        MikadoYellow,               ///< \htmlonly <div style='background-color:#FFC40C; width:50px;'>&nbsp;</div> \endhtmlonly
        Mint,                       ///< \htmlonly <div style='background-color:#3EB489; width:50px;'>&nbsp;</div> \endhtmlonly
        MintCream,                  ///< \htmlonly <div style='background-color:#F5FFFA; width:50px;'>&nbsp;</div> \endhtmlonly
        MintGreen,                  ///< \htmlonly <div style='background-color:#98FF98; width:50px;'>&nbsp;</div> \endhtmlonly
        MistyRose,                  ///< \htmlonly <div style='background-color:#FFE4E1; width:50px;'>&nbsp;</div> \endhtmlonly
        Moccasin,                   ///< \htmlonly <div style='background-color:#FAEBD7; width:50px;'>&nbsp;</div> \endhtmlonly
        ModeBeige,                  ///< \htmlonly <div style='background-color:#967117; width:50px;'>&nbsp;</div> \endhtmlonly
        MoonstoneBlue,              ///< \htmlonly <div style='background-color:#73A9C2; width:50px;'>&nbsp;</div> \endhtmlonly
        MordantRed19,               ///< \htmlonly <div style='background-color:#AE0C00; width:50px;'>&nbsp;</div> \endhtmlonly
        MossGreen,                  ///< \htmlonly <div style='background-color:#ADDFAD; width:50px;'>&nbsp;</div> \endhtmlonly
        MountainMeadow,             ///< \htmlonly <div style='background-color:#30BA8F; width:50px;'>&nbsp;</div> \endhtmlonly
        MountbattenPink,            ///< \htmlonly <div style='background-color:#997A8D; width:50px;'>&nbsp;</div> \endhtmlonly
        MSUGreen,                   ///< \htmlonly <div style='background-color:#18453B; width:50px;'>&nbsp;</div> \endhtmlonly
        Mulberry,                   ///< \htmlonly <div style='background-color:#C54B8C; width:50px;'>&nbsp;</div> \endhtmlonly
        Munsell,                    ///< \htmlonly <div style='background-color:#F2F3F4; width:50px;'>&nbsp;</div> \endhtmlonly
        Mustard,                    ///< \htmlonly <div style='background-color:#FFDB58; width:50px;'>&nbsp;</div> \endhtmlonly
        MustardYellow,              ///< \htmlonly <div style='background-color:#FFDB58; width:50px;'>&nbsp;</div> \endhtmlonly
        Myrtle,                     ///< \htmlonly <div style='background-color:#21421E; width:50px;'>&nbsp;</div> \endhtmlonly
        NadeshikoPink,              ///< \htmlonly <div style='background-color:#F6ADC6; width:50px;'>&nbsp;</div> \endhtmlonly
        NapierGreen,                ///< \htmlonly <div style='background-color:#2A8000; width:50px;'>&nbsp;</div> \endhtmlonly
        NaplesYellow,               ///< \htmlonly <div style='background-color:#FADA5E; width:50px;'>&nbsp;</div> \endhtmlonly
        NavajoWhite,                ///< \htmlonly <div style='background-color:#FFDEAD; width:50px;'>&nbsp;</div> \endhtmlonly
        Navel,                      ///< \htmlonly <div style='background-color:#EC8430; width:50px;'>&nbsp;</div> \endhtmlonly
        NavyBlue,                   ///< \htmlonly <div style='background-color:#000080; width:50px;'>&nbsp;</div> \endhtmlonly
        NeonCarrot,                 ///< \htmlonly <div style='background-color:#FFA343; width:50px;'>&nbsp;</div> \endhtmlonly
        NeonFuchsia,                ///< \htmlonly <div style='background-color:#FE59C2; width:50px;'>&nbsp;</div> \endhtmlonly
        NeonGreen,                  ///< \htmlonly <div style='background-color:#39FF14; width:50px;'>&nbsp;</div> \endhtmlonly
        NonPhotoBlue,               ///< \htmlonly <div style='background-color:#A4DDED; width:50px;'>&nbsp;</div> \endhtmlonly
        NorthTexasGreen,            ///< \htmlonly <div style='background-color:#059033; width:50px;'>&nbsp;</div> \endhtmlonly
        OceanBoatBlue,              ///< \htmlonly <div style='background-color:#0077BE; width:50px;'>&nbsp;</div> \endhtmlonly
        Ochre,                      ///< \htmlonly <div style='background-color:#CC7722; width:50px;'>&nbsp;</div> \endhtmlonly
        OctoberMist,                ///< \htmlonly <div style='background-color:#B6B8A5; width:50px;'>&nbsp;</div> \endhtmlonly
        OfficeGreen,                ///< \htmlonly <div style='background-color:#008000; width:50px;'>&nbsp;</div> \endhtmlonly
        OldGold,                    ///< \htmlonly <div style='background-color:#CFB53B; width:50px;'>&nbsp;</div> \endhtmlonly
        OldLace,                    ///< \htmlonly <div style='background-color:#FDF5E6; width:50px;'>&nbsp;</div> \endhtmlonly
        OldLavender,                ///< \htmlonly <div style='background-color:#796878; width:50px;'>&nbsp;</div> \endhtmlonly
        OldMauve,                   ///< \htmlonly <div style='background-color:#673147; width:50px;'>&nbsp;</div> \endhtmlonly
        OldRose,                    ///< \htmlonly <div style='background-color:#C08081; width:50px;'>&nbsp;</div> \endhtmlonly
        Olive,                      ///< \htmlonly <div style='background-color:#808000; width:50px;'>&nbsp;</div> \endhtmlonly
        OliveDrab,                  ///< \htmlonly <div style='background-color:#6B8E23; width:50px;'>&nbsp;</div> \endhtmlonly
        OliveGreen,                 ///< \htmlonly <div style='background-color:#BAB86C; width:50px;'>&nbsp;</div> \endhtmlonly
        Olivine,                    ///< \htmlonly <div style='background-color:#9AB973; width:50px;'>&nbsp;</div> \endhtmlonly
        Onyx,                       ///< \htmlonly <div style='background-color:#0F0F0F; width:50px;'>&nbsp;</div> \endhtmlonly
        OperaMauve,                 ///< \htmlonly <div style='background-color:#B784A7; width:50px;'>&nbsp;</div> \endhtmlonly
        Orange,                     ///< \htmlonly <div style='background-color:#FFA500; width:50px;'>&nbsp;</div> \endhtmlonly
        OrangePeel,                 ///< \htmlonly <div style='background-color:#FF9F00; width:50px;'>&nbsp;</div> \endhtmlonly
        OrangeRed,                  ///< \htmlonly <div style='background-color:#FF4500; width:50px;'>&nbsp;</div> \endhtmlonly
        OrangeYellow,               ///< \htmlonly <div style='background-color:#F8D568; width:50px;'>&nbsp;</div> \endhtmlonly
        Orchid,                     ///< \htmlonly <div style='background-color:#DA70D6; width:50px;'>&nbsp;</div> \endhtmlonly
        OrigamiWhite,               ///< \htmlonly <div style='background-color:#E5E2DA; width:50px;'>&nbsp;</div> \endhtmlonly
        OtterBrown,                 ///< \htmlonly <div style='background-color:#654321; width:50px;'>&nbsp;</div> \endhtmlonly
        OuterSpace,                 ///< \htmlonly <div style='background-color:#414A4C; width:50px;'>&nbsp;</div> \endhtmlonly
        OutrageousOrange,           ///< \htmlonly <div style='background-color:#FF6E4A; width:50px;'>&nbsp;</div> \endhtmlonly
        OxfordBlue,                 ///< \htmlonly <div style='background-color:#002147; width:50px;'>&nbsp;</div> \endhtmlonly
        PacificBlue,                ///< \htmlonly <div style='background-color:#1CA9C9; width:50px;'>&nbsp;</div> \endhtmlonly
        PakistanGreen,              ///< \htmlonly <div style='background-color:#006600; width:50px;'>&nbsp;</div> \endhtmlonly
        PalatinateBlue,             ///< \htmlonly <div style='background-color:#273BE2; width:50px;'>&nbsp;</div> \endhtmlonly
        PalatinatePurple,           ///< \htmlonly <div style='background-color:#682860; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleAqua,                   ///< \htmlonly <div style='background-color:#BCD4E6; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleBlue,                   ///< \htmlonly <div style='background-color:#AFEEEE; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleBrown,                  ///< \htmlonly <div style='background-color:#987654; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleCarmine,                ///< \htmlonly <div style='background-color:#AF4035; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleCerulean,               ///< \htmlonly <div style='background-color:#9BC4E2; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleChestnut,               ///< \htmlonly <div style='background-color:#DDADAF; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleCopper,                 ///< \htmlonly <div style='background-color:#DA8A67; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleCornflowerBlue,         ///< \htmlonly <div style='background-color:#ABCDEF; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleGold,                   ///< \htmlonly <div style='background-color:#E6BE8A; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleGoldenrod,              ///< \htmlonly <div style='background-color:#EEE8AA; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleGreen,                  ///< \htmlonly <div style='background-color:#98FB98; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleLavender,               ///< \htmlonly <div style='background-color:#DCD0FF; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleMagenta,                ///< \htmlonly <div style='background-color:#F984E5; width:50px;'>&nbsp;</div> \endhtmlonly
        PalePink,                   ///< \htmlonly <div style='background-color:#FADADD; width:50px;'>&nbsp;</div> \endhtmlonly
        PalePlum,                   ///< \htmlonly <div style='background-color:#DDA0DD; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleRedViolet,              ///< \htmlonly <div style='background-color:#DB7093; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleRobinEggBlue,           ///< \htmlonly <div style='background-color:#96DED1; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleSilver,                 ///< \htmlonly <div style='background-color:#C9C0BB; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleSpringBud,              ///< \htmlonly <div style='background-color:#ECEBBD; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleTaupe,                  ///< \htmlonly <div style='background-color:#BC987E; width:50px;'>&nbsp;</div> \endhtmlonly
        PaleVioletRed,              ///< \htmlonly <div style='background-color:#DB7093; width:50px;'>&nbsp;</div> \endhtmlonly
        PansyPurple,                ///< \htmlonly <div style='background-color:#78184A; width:50px;'>&nbsp;</div> \endhtmlonly
        PapayaWhip,                 ///< \htmlonly <div style='background-color:#FFEFD5; width:50px;'>&nbsp;</div> \endhtmlonly
        ParisGreen,                 ///< \htmlonly <div style='background-color:#50C878; width:50px;'>&nbsp;</div> \endhtmlonly
        PastelBlue,                 ///< \htmlonly <div style='background-color:#AEC6CF; width:50px;'>&nbsp;</div> \endhtmlonly
        PastelBrown,                ///< \htmlonly <div style='background-color:#836953; width:50px;'>&nbsp;</div> \endhtmlonly
        PastelGray,                 ///< \htmlonly <div style='background-color:#CFCFC4; width:50px;'>&nbsp;</div> \endhtmlonly
        PastelGreen,                ///< \htmlonly <div style='background-color:#77DD77; width:50px;'>&nbsp;</div> \endhtmlonly
        PastelMagenta,              ///< \htmlonly <div style='background-color:#F49AC2; width:50px;'>&nbsp;</div> \endhtmlonly
        PastelOrange,               ///< \htmlonly <div style='background-color:#FFB347; width:50px;'>&nbsp;</div> \endhtmlonly
        PastelPink,                 ///< \htmlonly <div style='background-color:#FFD1DC; width:50px;'>&nbsp;</div> \endhtmlonly
        PastelPurple,               ///< \htmlonly <div style='background-color:#B39EB5; width:50px;'>&nbsp;</div> \endhtmlonly
        PastelRed,                  ///< \htmlonly <div style='background-color:#FF6961; width:50px;'>&nbsp;</div> \endhtmlonly
        PastelViolet,               ///< \htmlonly <div style='background-color:#CB99C9; width:50px;'>&nbsp;</div> \endhtmlonly
        PastelYellow,               ///< \htmlonly <div style='background-color:#FDFD96; width:50px;'>&nbsp;</div> \endhtmlonly
        Patriarch,                  ///< \htmlonly <div style='background-color:#800080; width:50px;'>&nbsp;</div> \endhtmlonly
        PayneGrey,                  ///< \htmlonly <div style='background-color:#536878; width:50px;'>&nbsp;</div> \endhtmlonly
        Peach,                      ///< \htmlonly <div style='background-color:#FFE5B4; width:50px;'>&nbsp;</div> \endhtmlonly
        PeachPuff,                  ///< \htmlonly <div style='background-color:#FFDAB9; width:50px;'>&nbsp;</div> \endhtmlonly
        PeachYellow,                ///< \htmlonly <div style='background-color:#FADFAD; width:50px;'>&nbsp;</div> \endhtmlonly
        Pear,                       ///< \htmlonly <div style='background-color:#D1E231; width:50px;'>&nbsp;</div> \endhtmlonly
        Pearl,                      ///< \htmlonly <div style='background-color:#EAE0C8; width:50px;'>&nbsp;</div> \endhtmlonly
        PearlAqua,                  ///< \htmlonly <div style='background-color:#88D8C0; width:50px;'>&nbsp;</div> \endhtmlonly
        Peridot,                    ///< \htmlonly <div style='background-color:#E6E200; width:50px;'>&nbsp;</div> \endhtmlonly
        Periwinkle,                 ///< \htmlonly <div style='background-color:#CCCCFF; width:50px;'>&nbsp;</div> \endhtmlonly
        PersianBlue,                ///< \htmlonly <div style='background-color:#1C39BB; width:50px;'>&nbsp;</div> \endhtmlonly
        PersianIndigo,              ///< \htmlonly <div style='background-color:#32127A; width:50px;'>&nbsp;</div> \endhtmlonly
        PersianOrange,              ///< \htmlonly <div style='background-color:#D99058; width:50px;'>&nbsp;</div> \endhtmlonly
        PersianPink,                ///< \htmlonly <div style='background-color:#F77FBE; width:50px;'>&nbsp;</div> \endhtmlonly
        PersianPlum,                ///< \htmlonly <div style='background-color:#701C1C; width:50px;'>&nbsp;</div> \endhtmlonly
        PersianRed,                 ///< \htmlonly <div style='background-color:#CC3333; width:50px;'>&nbsp;</div> \endhtmlonly
        PersianRose,                ///< \htmlonly <div style='background-color:#FE28A2; width:50px;'>&nbsp;</div> \endhtmlonly
        Phlox,                      ///< \htmlonly <div style='background-color:#DF00FF; width:50px;'>&nbsp;</div> \endhtmlonly
        PhthaloBlue,                ///< \htmlonly <div style='background-color:#000F89; width:50px;'>&nbsp;</div> \endhtmlonly
        PhthaloGreen,               ///< \htmlonly <div style='background-color:#123524; width:50px;'>&nbsp;</div> \endhtmlonly
        PiggyPink,                  ///< \htmlonly <div style='background-color:#FDDDE6; width:50px;'>&nbsp;</div> \endhtmlonly
        PineGreen,                  ///< \htmlonly <div style='background-color:#01796F; width:50px;'>&nbsp;</div> \endhtmlonly
        Pink,                       ///< \htmlonly <div style='background-color:#FFC0CB; width:50px;'>&nbsp;</div> \endhtmlonly
        PinkFlamingo,               ///< \htmlonly <div style='background-color:#FC74FD; width:50px;'>&nbsp;</div> \endhtmlonly
        PinkPearl,                  ///< \htmlonly <div style='background-color:#E7ACCF; width:50px;'>&nbsp;</div> \endhtmlonly
        PinkShadow,                 ///< \htmlonly <div style='background-color:#DEC3B9; width:50px;'>&nbsp;</div> \endhtmlonly
        PinkSherbet,                ///< \htmlonly <div style='background-color:#F78FA7; width:50px;'>&nbsp;</div> \endhtmlonly
        PinkyBeige,                 ///< \htmlonly <div style='background-color:#C9AA98; width:50px;'>&nbsp;</div> \endhtmlonly
        Pistachio,                  ///< \htmlonly <div style='background-color:#93C572; width:50px;'>&nbsp;</div> \endhtmlonly
        Platinum,                   ///< \htmlonly <div style='background-color:#E5E4E2; width:50px;'>&nbsp;</div> \endhtmlonly
        Plum,                       ///< \htmlonly <div style='background-color:#DDA0DD; width:50px;'>&nbsp;</div> \endhtmlonly
        PortlandOrange,             ///< \htmlonly <div style='background-color:#FF5A36; width:50px;'>&nbsp;</div> \endhtmlonly
        PowderBlue,                 ///< \htmlonly <div style='background-color:#B0E0E6; width:50px;'>&nbsp;</div> \endhtmlonly
        PracticalBeige,             ///< \htmlonly <div style='background-color:#C9B29C; width:50px;'>&nbsp;</div> \endhtmlonly
        PrincetonOrange,            ///< \htmlonly <div style='background-color:#FF8F00; width:50px;'>&nbsp;</div> \endhtmlonly
        PrussianBlue,               ///< \htmlonly <div style='background-color:#003153; width:50px;'>&nbsp;</div> \endhtmlonly
        PsychedelicPurple,          ///< \htmlonly <div style='background-color:#DF00FF; width:50px;'>&nbsp;</div> \endhtmlonly
        Puce,                       ///< \htmlonly <div style='background-color:#CC8899; width:50px;'>&nbsp;</div> \endhtmlonly
        Pumpkin,                    ///< \htmlonly <div style='background-color:#FF7518; width:50px;'>&nbsp;</div> \endhtmlonly
        PureWhite,                  ///< \htmlonly <div style='background-color:#EDECE6; width:50px;'>&nbsp;</div> \endhtmlonly
        Purple,                     ///< \htmlonly <div style='background-color:#800080; width:50px;'>&nbsp;</div> \endhtmlonly
        PurpleHeart,                ///< \htmlonly <div style='background-color:#69359C; width:50px;'>&nbsp;</div> \endhtmlonly
        PurpleMountainMajesty,      ///< \htmlonly <div style='background-color:#9D81BA; width:50px;'>&nbsp;</div> \endhtmlonly
        PurplePizzazz,              ///< \htmlonly <div style='background-color:#FE4EDA; width:50px;'>&nbsp;</div> \endhtmlonly
        PurpleTaupe,                ///< \htmlonly <div style='background-color:#50404D; width:50px;'>&nbsp;</div> \endhtmlonly
        Rackley,                    ///< \htmlonly <div style='background-color:#5D8AA8; width:50px;'>&nbsp;</div> \endhtmlonly
        RadiantLilac,               ///< \htmlonly <div style='background-color:#A489A0; width:50px;'>&nbsp;</div> \endhtmlonly
        RadicalRed,                 ///< \htmlonly <div style='background-color:#FF355E; width:50px;'>&nbsp;</div> \endhtmlonly
        Rain,                       ///< \htmlonly <div style='background-color:#6BB7C4; width:50px;'>&nbsp;</div> \endhtmlonly
        Raspberry,                  ///< \htmlonly <div style='background-color:#E30B5D; width:50px;'>&nbsp;</div> \endhtmlonly
        RaspberryGlace,             ///< \htmlonly <div style='background-color:#915F6D; width:50px;'>&nbsp;</div> \endhtmlonly
        RaspberryPink,              ///< \htmlonly <div style='background-color:#E25098; width:50px;'>&nbsp;</div> \endhtmlonly
        RaspberryRose,              ///< \htmlonly <div style='background-color:#B3446C; width:50px;'>&nbsp;</div> \endhtmlonly
        RawSienna,                  ///< \htmlonly <div style='background-color:#D68A59; width:50px;'>&nbsp;</div> \endhtmlonly
        RazzleDazzleRose,           ///< \htmlonly <div style='background-color:#FF33CC; width:50px;'>&nbsp;</div> \endhtmlonly
        Razzmatazz,                 ///< \htmlonly <div style='background-color:#E3256B; width:50px;'>&nbsp;</div> \endhtmlonly
        Red,                        ///< \htmlonly <div style='background-color:#FF0000; width:50px;'>&nbsp;</div> \endhtmlonly
        RedBrown,                   ///< \htmlonly <div style='background-color:#A52A2A; width:50px;'>&nbsp;</div> \endhtmlonly
        RedOrange,                  ///< \htmlonly <div style='background-color:#FF5349; width:50px;'>&nbsp;</div> \endhtmlonly
        RedViolet,                  ///< \htmlonly <div style='background-color:#C71585; width:50px;'>&nbsp;</div> \endhtmlonly
        RichBlack,                  ///< \htmlonly <div style='background-color:#004040; width:50px;'>&nbsp;</div> \endhtmlonly
        RichCarmine,                ///< \htmlonly <div style='background-color:#D70040; width:50px;'>&nbsp;</div> \endhtmlonly
        RichElectricBlue,           ///< \htmlonly <div style='background-color:#0892D0; width:50px;'>&nbsp;</div> \endhtmlonly
        RichLilac,                  ///< \htmlonly <div style='background-color:#B666D2; width:50px;'>&nbsp;</div> \endhtmlonly
        RichMaroon,                 ///< \htmlonly <div style='background-color:#B03060; width:50px;'>&nbsp;</div> \endhtmlonly
        RifleGreen,                 ///< \htmlonly <div style='background-color:#414833; width:50px;'>&nbsp;</div> \endhtmlonly
        RobinEggBlue,               ///< \htmlonly <div style='background-color:#1FCECB; width:50px;'>&nbsp;</div> \endhtmlonly
        Rose,                       ///< \htmlonly <div style='background-color:#FF007F; width:50px;'>&nbsp;</div> \endhtmlonly
        RoseBonbon,                 ///< \htmlonly <div style='background-color:#F9429E; width:50px;'>&nbsp;</div> \endhtmlonly
        RoseEbony,                  ///< \htmlonly <div style='background-color:#674846; width:50px;'>&nbsp;</div> \endhtmlonly
        RoseGold,                   ///< \htmlonly <div style='background-color:#B76E79; width:50px;'>&nbsp;</div> \endhtmlonly
        RoseMadder,                 ///< \htmlonly <div style='background-color:#E32636; width:50px;'>&nbsp;</div> \endhtmlonly
        RosePink,                   ///< \htmlonly <div style='background-color:#FF66CC; width:50px;'>&nbsp;</div> \endhtmlonly
        RoseQuartz,                 ///< \htmlonly <div style='background-color:#AA98A9; width:50px;'>&nbsp;</div> \endhtmlonly
        RoseTan,                    ///< \htmlonly <div style='background-color:#CD9C85; width:50px;'>&nbsp;</div> \endhtmlonly
        RoseTaupe,                  ///< \htmlonly <div style='background-color:#905D5D; width:50px;'>&nbsp;</div> \endhtmlonly
        RoseVale,                   ///< \htmlonly <div style='background-color:#AB4E52; width:50px;'>&nbsp;</div> \endhtmlonly
        Rosewood,                   ///< \htmlonly <div style='background-color:#65000B; width:50px;'>&nbsp;</div> \endhtmlonly
        RossoCorsa,                 ///< \htmlonly <div style='background-color:#D40000; width:50px;'>&nbsp;</div> \endhtmlonly
        RosyBrown,                  ///< \htmlonly <div style='background-color:#BC8F8F; width:50px;'>&nbsp;</div> \endhtmlonly
        RosyOutlook,                ///< \htmlonly <div style='background-color:#EBCECB; width:50px;'>&nbsp;</div> \endhtmlonly
        RoyalAzure,                 ///< \htmlonly <div style='background-color:#0038A8; width:50px;'>&nbsp;</div> \endhtmlonly
        RoyalBlue,                  ///< \htmlonly <div style='background-color:#4169E1; width:50px;'>&nbsp;</div> \endhtmlonly
        RoyalFuchsia,               ///< \htmlonly <div style='background-color:#CA2C92; width:50px;'>&nbsp;</div> \endhtmlonly
        RoyalPurple,                ///< \htmlonly <div style='background-color:#7851A9; width:50px;'>&nbsp;</div> \endhtmlonly
        Ruby,                       ///< \htmlonly <div style='background-color:#E0115F; width:50px;'>&nbsp;</div> \endhtmlonly
        Ruddy,                      ///< \htmlonly <div style='background-color:#FF0028; width:50px;'>&nbsp;</div> \endhtmlonly
        RuddyBrown,                 ///< \htmlonly <div style='background-color:#BB6528; width:50px;'>&nbsp;</div> \endhtmlonly
        RuddyPink,                  ///< \htmlonly <div style='background-color:#E18E96; width:50px;'>&nbsp;</div> \endhtmlonly
        Rufous,                     ///< \htmlonly <div style='background-color:#A81C07; width:50px;'>&nbsp;</div> \endhtmlonly
        Russet,                     ///< \htmlonly <div style='background-color:#80461B; width:50px;'>&nbsp;</div> \endhtmlonly
        Rust,                       ///< \htmlonly <div style='background-color:#B7410E; width:50px;'>&nbsp;</div> \endhtmlonly
        SacramentoStateGreen,       ///< \htmlonly <div style='background-color:#00563F; width:50px;'>&nbsp;</div> \endhtmlonly
        SaddleBrown,                ///< \htmlonly <div style='background-color:#8B4513; width:50px;'>&nbsp;</div> \endhtmlonly
        SafetyOrange,               ///< \htmlonly <div style='background-color:#FF6700; width:50px;'>&nbsp;</div> \endhtmlonly
        Saffron,                    ///< \htmlonly <div style='background-color:#F4C430; width:50px;'>&nbsp;</div> \endhtmlonly
        SaintPatrickBlue,           ///< \htmlonly <div style='background-color:#23297A; width:50px;'>&nbsp;</div> \endhtmlonly
        Salmon,                     ///< \htmlonly <div style='background-color:#FF8C69; width:50px;'>&nbsp;</div> \endhtmlonly
        SalmonPink,                 ///< \htmlonly <div style='background-color:#FF91A4; width:50px;'>&nbsp;</div> \endhtmlonly
        SalonRose,                  ///< \htmlonly <div style='background-color:#AB7878; width:50px;'>&nbsp;</div> \endhtmlonly
        Sand,                       ///< \htmlonly <div style='background-color:#C2B280; width:50px;'>&nbsp;</div> \endhtmlonly
        SandDune,                   ///< \htmlonly <div style='background-color:#967117; width:50px;'>&nbsp;</div> \endhtmlonly
        Sandstorm,                  ///< \htmlonly <div style='background-color:#ECD540; width:50px;'>&nbsp;</div> \endhtmlonly
        SandyBrown,                 ///< \htmlonly <div style='background-color:#F4A460; width:50px;'>&nbsp;</div> \endhtmlonly
        SandyTaupe,                 ///< \htmlonly <div style='background-color:#967117; width:50px;'>&nbsp;</div> \endhtmlonly
        SapGreen,                   ///< \htmlonly <div style='background-color:#507D2A; width:50px;'>&nbsp;</div> \endhtmlonly
        Sapphire,                   ///< \htmlonly <div style='background-color:#0F52BA; width:50px;'>&nbsp;</div> \endhtmlonly
        SatinSheenGold,             ///< \htmlonly <div style='background-color:#CBA135; width:50px;'>&nbsp;</div> \endhtmlonly
        Scarlet,                    ///< \htmlonly <div style='background-color:#FF2400; width:50px;'>&nbsp;</div> \endhtmlonly
        SchoolBusYellow,            ///< \htmlonly <div style='background-color:#FFD800; width:50px;'>&nbsp;</div> \endhtmlonly
        ScreaminGreen,              ///< \htmlonly <div style='background-color:#76FF7A; width:50px;'>&nbsp;</div> \endhtmlonly
        SeaBlue,                    ///< \htmlonly <div style='background-color:#006994; width:50px;'>&nbsp;</div> \endhtmlonly
        SeaGreen,                   ///< \htmlonly <div style='background-color:#2E8B57; width:50px;'>&nbsp;</div> \endhtmlonly
        SealBrown,                  ///< \htmlonly <div style='background-color:#321414; width:50px;'>&nbsp;</div> \endhtmlonly
        SeaSalt,                    ///< \htmlonly <div style='background-color:#CDD2CA; width:50px;'>&nbsp;</div> \endhtmlonly
        Seashell,                   ///< \htmlonly <div style='background-color:#FFF5EE; width:50px;'>&nbsp;</div> \endhtmlonly
        SelectiveYellow,            ///< \htmlonly <div style='background-color:#FFBA00; width:50px;'>&nbsp;</div> \endhtmlonly
        Sepia,                      ///< \htmlonly <div style='background-color:#704214; width:50px;'>&nbsp;</div> \endhtmlonly
        Serenity,                   ///< \htmlonly <div style='background-color:#C8D3E7; width:50px;'>&nbsp;</div> \endhtmlonly
        Shadow,                     ///< \htmlonly <div style='background-color:#8A795D; width:50px;'>&nbsp;</div> \endhtmlonly
        Shamrock,                   ///< \htmlonly <div style='background-color:#45CEA2; width:50px;'>&nbsp;</div> \endhtmlonly
        ShamrockGreen,              ///< \htmlonly <div style='background-color:#009E60; width:50px;'>&nbsp;</div> \endhtmlonly
        ShockingPink,               ///< \htmlonly <div style='background-color:#FC0FC0; width:50px;'>&nbsp;</div> \endhtmlonly
        Sienna,                     ///< \htmlonly <div style='background-color:#882D17; width:50px;'>&nbsp;</div> \endhtmlonly
        Silver,                     ///< \htmlonly <div style='background-color:#C0C0C0; width:50px;'>&nbsp;</div> \endhtmlonly
        Sinopia,                    ///< \htmlonly <div style='background-color:#CB410B; width:50px;'>&nbsp;</div> \endhtmlonly
        Skobeloff,                  ///< \htmlonly <div style='background-color:#007474; width:50px;'>&nbsp;</div> \endhtmlonly
        SkyBlue,                    ///< \htmlonly <div style='background-color:#87CEEB; width:50px;'>&nbsp;</div> \endhtmlonly
        SkyMagenta,                 ///< \htmlonly <div style='background-color:#CF71AF; width:50px;'>&nbsp;</div> \endhtmlonly
        SlateBlue,                  ///< \htmlonly <div style='background-color:#6A5ACD; width:50px;'>&nbsp;</div> \endhtmlonly
        SlateGray,                  ///< \htmlonly <div style='background-color:#708090; width:50px;'>&nbsp;</div> \endhtmlonly
        Slytherin1,                 ///< \htmlonly <div style='background-color:#1A472A; width:50px;'>&nbsp;</div> \endhtmlonly
        Slytherin2,                 ///< \htmlonly <div style='background-color:#2A623D; width:50px;'>&nbsp;</div> \endhtmlonly
        Slytherin3,                 ///< \htmlonly <div style='background-color:#5D5D5D; width:50px;'>&nbsp;</div> \endhtmlonly
        Slytherin4,                 ///< \htmlonly <div style='background-color:#AAAAAA; width:50px;'>&nbsp;</div> \endhtmlonly
        Smalt,                      ///< \htmlonly <div style='background-color:#003399; width:50px;'>&nbsp;</div> \endhtmlonly
        SmokeyTopaz,                ///< \htmlonly <div style='background-color:#933D41; width:50px;'>&nbsp;</div> \endhtmlonly
        SmokyBlack,                 ///< \htmlonly <div style='background-color:#100C08; width:50px;'>&nbsp;</div> \endhtmlonly
        SmokySalmon,                ///< \htmlonly <div style='background-color:#E2B6A7; width:50px;'>&nbsp;</div> \endhtmlonly
        Snow,                       ///< \htmlonly <div style='background-color:#FFFAFA; width:50px;'>&nbsp;</div> \endhtmlonly
        SpicedCider,                ///< \htmlonly <div style='background-color:#B0785C; width:50px;'>&nbsp;</div> \endhtmlonly
        SpiroDiscoBall,             ///< \htmlonly <div style='background-color:#0FC0FC; width:50px;'>&nbsp;</div> \endhtmlonly
        SpringBud,                  ///< \htmlonly <div style='background-color:#A7FC00; width:50px;'>&nbsp;</div> \endhtmlonly
        SpringGreen,                ///< \htmlonly <div style='background-color:#00FF7F; width:50px;'>&nbsp;</div> \endhtmlonly
        SteelBlue,                  ///< \htmlonly <div style='background-color:#4682B4; width:50px;'>&nbsp;</div> \endhtmlonly
        StilDeGrainYellow,          ///< \htmlonly <div style='background-color:#FADA5E; width:50px;'>&nbsp;</div> \endhtmlonly
        Stizza,                     ///< \htmlonly <div style='background-color:#990000; width:50px;'>&nbsp;</div> \endhtmlonly
        Stormcloud,                 ///< \htmlonly <div style='background-color:#008080; width:50px;'>&nbsp;</div> \endhtmlonly
        Straw,                      ///< \htmlonly <div style='background-color:#E4D96F; width:50px;'>&nbsp;</div> \endhtmlonly
        StudioMauve,                ///< \htmlonly <div style='background-color:#C6B9B8; width:50px;'>&nbsp;</div> \endhtmlonly
        SunbeamYellow,              ///< \htmlonly <div style='background-color:#F0D39D; width:50px;'>&nbsp;</div> \endhtmlonly
        Sunglow,                    ///< \htmlonly <div style='background-color:#FFCC33; width:50px;'>&nbsp;</div> \endhtmlonly
        Sunset,                     ///< \htmlonly <div style='background-color:#FAD6A5; width:50px;'>&nbsp;</div> \endhtmlonly
        SunsetOrange,               ///< \htmlonly <div style='background-color:#FD5E53; width:50px;'>&nbsp;</div> \endhtmlonly
        SvelteSage,                 ///< \htmlonly <div style='background-color:#B2AC96; width:50px;'>&nbsp;</div> \endhtmlonly
        Tan,                        ///< \htmlonly <div style='background-color:#D2B48C; width:50px;'>&nbsp;</div> \endhtmlonly
        Tangelo,                    ///< \htmlonly <div style='background-color:#F94D00; width:50px;'>&nbsp;</div> \endhtmlonly
        Tangerine,                  ///< \htmlonly <div style='background-color:#F28500; width:50px;'>&nbsp;</div> \endhtmlonly
        TangerineYellow,            ///< \htmlonly <div style='background-color:#FFCC00; width:50px;'>&nbsp;</div> \endhtmlonly
        Taupe,                      ///< \htmlonly <div style='background-color:#483C32; width:50px;'>&nbsp;</div> \endhtmlonly
        TaupeGray,                  ///< \htmlonly <div style='background-color:#8B8589; width:50px;'>&nbsp;</div> \endhtmlonly
        Tawny,                      ///< \htmlonly <div style='background-color:#CD5700; width:50px;'>&nbsp;</div> \endhtmlonly
        TeaGreen,                   ///< \htmlonly <div style='background-color:#D0F0C0; width:50px;'>&nbsp;</div> \endhtmlonly
        Teal,                       ///< \htmlonly <div style='background-color:#008080; width:50px;'>&nbsp;</div> \endhtmlonly
        TealBlue,                   ///< \htmlonly <div style='background-color:#367588; width:50px;'>&nbsp;</div> \endhtmlonly
        TealGreen,                  ///< \htmlonly <div style='background-color:#006D5B; width:50px;'>&nbsp;</div> \endhtmlonly
        TeaRose,                    ///< \htmlonly <div style='background-color:#F4C2C2; width:50px;'>&nbsp;</div> \endhtmlonly
        TerraCotta,                 ///< \htmlonly <div style='background-color:#E2725B; width:50px;'>&nbsp;</div> \endhtmlonly
        Thistle,                    ///< \htmlonly <div style='background-color:#D8BFD8; width:50px;'>&nbsp;</div> \endhtmlonly
        ThulianPink,                ///< \htmlonly <div style='background-color:#DE6FA1; width:50px;'>&nbsp;</div> \endhtmlonly
        TickleMePink,               ///< \htmlonly <div style='background-color:#FC89AC; width:50px;'>&nbsp;</div> \endhtmlonly
        TiffanyBlue,                ///< \htmlonly <div style='background-color:#0ABAB5; width:50px;'>&nbsp;</div> \endhtmlonly
        TigerEye,                   ///< \htmlonly <div style='background-color:#E08D3C; width:50px;'>&nbsp;</div> \endhtmlonly
        Timberwolf,                 ///< \htmlonly <div style='background-color:#DBD7D2; width:50px;'>&nbsp;</div> \endhtmlonly
        TitaniumYellow,             ///< \htmlonly <div style='background-color:#EEE600; width:50px;'>&nbsp;</div> \endhtmlonly
        Tomato,                     ///< \htmlonly <div style='background-color:#FF6347; width:50px;'>&nbsp;</div> \endhtmlonly
        Toolbox,                    ///< \htmlonly <div style='background-color:#746CC0; width:50px;'>&nbsp;</div> \endhtmlonly
        Topaz,                      ///< \htmlonly <div style='background-color:#FFC87C; width:50px;'>&nbsp;</div> \endhtmlonly
        TractorRed,                 ///< \htmlonly <div style='background-color:#FD0E35; width:50px;'>&nbsp;</div> \endhtmlonly
        Tradewind,                  ///< \htmlonly <div style='background-color:#C2CFCF; width:50px;'>&nbsp;</div> \endhtmlonly
        TricornBlack,               ///< \htmlonly <div style='background-color:#2F2F30; width:50px;'>&nbsp;</div> \endhtmlonly
        TrolleyGrey,                ///< \htmlonly <div style='background-color:#808080; width:50px;'>&nbsp;</div> \endhtmlonly
        TropicalRainForest,         ///< \htmlonly <div style='background-color:#00755E; width:50px;'>&nbsp;</div> \endhtmlonly
        TrueBlue,                   ///< \htmlonly <div style='background-color:#0073CF; width:50px;'>&nbsp;</div> \endhtmlonly
        TuftsBlue,                  ///< \htmlonly <div style='background-color:#417DC1; width:50px;'>&nbsp;</div> \endhtmlonly
        Tumbleweed,                 ///< \htmlonly <div style='background-color:#DEAA88; width:50px;'>&nbsp;</div> \endhtmlonly
        TurkishRose,                ///< \htmlonly <div style='background-color:#B57281; width:50px;'>&nbsp;</div> \endhtmlonly
        Turquoise,                  ///< \htmlonly <div style='background-color:#30D5C8; width:50px;'>&nbsp;</div> \endhtmlonly
        TurquoiseBlue,              ///< \htmlonly <div style='background-color:#00FFEF; width:50px;'>&nbsp;</div> \endhtmlonly
        TurquoiseGreen,             ///< \htmlonly <div style='background-color:#A0D6B4; width:50px;'>&nbsp;</div> \endhtmlonly
        TuscanRed,                  ///< \htmlonly <div style='background-color:#66424D; width:50px;'>&nbsp;</div> \endhtmlonly
        TwilightLavender,           ///< \htmlonly <div style='background-color:#8A496B; width:50px;'>&nbsp;</div> \endhtmlonly
        TyrianPurple,               ///< \htmlonly <div style='background-color:#66023C; width:50px;'>&nbsp;</div> \endhtmlonly
        UABlue,                     ///< \htmlonly <div style='background-color:#0033AA; width:50px;'>&nbsp;</div> \endhtmlonly
        UARed,                      ///< \htmlonly <div style='background-color:#D9004C; width:50px;'>&nbsp;</div> \endhtmlonly
        Ube,                        ///< \htmlonly <div style='background-color:#8878C3; width:50px;'>&nbsp;</div> \endhtmlonly
        UCLABlue,                   ///< \htmlonly <div style='background-color:#536895; width:50px;'>&nbsp;</div> \endhtmlonly
        UCLAGold,                   ///< \htmlonly <div style='background-color:#FFB300; width:50px;'>&nbsp;</div> \endhtmlonly
        UFOGreen,                   ///< \htmlonly <div style='background-color:#3CD070; width:50px;'>&nbsp;</div> \endhtmlonly
        Ultramarine,                ///< \htmlonly <div style='background-color:#120A8F; width:50px;'>&nbsp;</div> \endhtmlonly
        UltramarineBlue,            ///< \htmlonly <div style='background-color:#4166F5; width:50px;'>&nbsp;</div> \endhtmlonly
        UltraPink,                  ///< \htmlonly <div style='background-color:#FF6FFF; width:50px;'>&nbsp;</div> \endhtmlonly
        Umber,                      ///< \htmlonly <div style='background-color:#635147; width:50px;'>&nbsp;</div> \endhtmlonly
        UnitedNationsBlue,          ///< \htmlonly <div style='background-color:#5B92E5; width:50px;'>&nbsp;</div> \endhtmlonly
        UniversityOfCaliforniaGold, ///< \htmlonly <div style='background-color:#B78727; width:50px;'>&nbsp;</div> \endhtmlonly
        UnmellowYellow,             ///< \htmlonly <div style='background-color:#FFFF66; width:50px;'>&nbsp;</div> \endhtmlonly
        UPForestGreen,              ///< \htmlonly <div style='background-color:#014421; width:50px;'>&nbsp;</div> \endhtmlonly
        UPMaroon,                   ///< \htmlonly <div style='background-color:#7B1113; width:50px;'>&nbsp;</div> \endhtmlonly
        UPsdellRed,                 ///< \htmlonly <div style='background-color:#AE2029; width:50px;'>&nbsp;</div> \endhtmlonly
        UrbanPutty,                 ///< \htmlonly <div style='background-color:#CFC0AB; width:50px;'>&nbsp;</div> \endhtmlonly
        Urobilin,                   ///< \htmlonly <div style='background-color:#E1AD21; width:50px;'>&nbsp;</div> \endhtmlonly
        USCCardinal,                ///< \htmlonly <div style='background-color:#990000; width:50px;'>&nbsp;</div> \endhtmlonly
        USCGold,                    ///< \htmlonly <div style='background-color:#FFCC00; width:50px;'>&nbsp;</div> \endhtmlonly
        UtahCrimson,                ///< \htmlonly <div style='background-color:#D3003F; width:50px;'>&nbsp;</div> \endhtmlonly
        Vanilla,                    ///< \htmlonly <div style='background-color:#F3E5AB; width:50px;'>&nbsp;</div> \endhtmlonly
        VegasGold,                  ///< \htmlonly <div style='background-color:#C5B358; width:50px;'>&nbsp;</div> \endhtmlonly
        VenetianRed,                ///< \htmlonly <div style='background-color:#C80815; width:50px;'>&nbsp;</div> \endhtmlonly
        Verdigris,                  ///< \htmlonly <div style='background-color:#43B3AE; width:50px;'>&nbsp;</div> \endhtmlonly
        Vermilion,                  ///< \htmlonly <div style='background-color:#E34234; width:50px;'>&nbsp;</div> \endhtmlonly
        Veronica,                   ///< \htmlonly <div style='background-color:#A020F0; width:50px;'>&nbsp;</div> \endhtmlonly
        Violet,                     ///< \htmlonly <div style='background-color:#EE82EE; width:50px;'>&nbsp;</div> \endhtmlonly
        VioletBlue,                 ///< \htmlonly <div style='background-color:#324AB2; width:50px;'>&nbsp;</div> \endhtmlonly
        VioletRed,                  ///< \htmlonly <div style='background-color:#F75394; width:50px;'>&nbsp;</div> \endhtmlonly
        Viridian,                   ///< \htmlonly <div style='background-color:#40826D; width:50px;'>&nbsp;</div> \endhtmlonly
        VividAuburn,                ///< \htmlonly <div style='background-color:#922724; width:50px;'>&nbsp;</div> \endhtmlonly
        VividBurgundy,              ///< \htmlonly <div style='background-color:#9F1D35; width:50px;'>&nbsp;</div> \endhtmlonly
        VividCerise,                ///< \htmlonly <div style='background-color:#DA1D81; width:50px;'>&nbsp;</div> \endhtmlonly
        VividTangerine,             ///< \htmlonly <div style='background-color:#FFA089; width:50px;'>&nbsp;</div> \endhtmlonly
        VividViolet,                ///< \htmlonly <div style='background-color:#9F00FF; width:50px;'>&nbsp;</div> \endhtmlonly
        VogueGreen,                 ///< \htmlonly <div style='background-color:#4B5645; width:50px;'>&nbsp;</div> \endhtmlonly
        WarmBlack,                  ///< \htmlonly <div style='background-color:#004242; width:50px;'>&nbsp;</div> \endhtmlonly
        Waterspout,                 ///< \htmlonly <div style='background-color:#00FFFF; width:50px;'>&nbsp;</div> \endhtmlonly
        Watery,                     ///< \htmlonly <div style='background-color:#B4CCC9; width:50px;'>&nbsp;</div> \endhtmlonly
        Wenge,                      ///< \htmlonly <div style='background-color:#645452; width:50px;'>&nbsp;</div> \endhtmlonly
        Wheat,                      ///< \htmlonly <div style='background-color:#F5DEB3; width:50px;'>&nbsp;</div> \endhtmlonly
        White,                      ///< \htmlonly <div style='background-color:#FFFFFF; width:50px;'>&nbsp;</div> \endhtmlonly
        WhiteSmoke,                 ///< \htmlonly <div style='background-color:#F5F5F5; width:50px;'>&nbsp;</div> \endhtmlonly
        WholeWheat,                 ///< \htmlonly <div style='background-color:#CDB592; width:50px;'>&nbsp;</div> \endhtmlonly
        WildBlueYonder,             ///< \htmlonly <div style='background-color:#A2ADD0; width:50px;'>&nbsp;</div> \endhtmlonly
        WildStrawberry,             ///< \htmlonly <div style='background-color:#FF43A4; width:50px;'>&nbsp;</div> \endhtmlonly
        WildWatermelon,             ///< \htmlonly <div style='background-color:#FC6C85; width:50px;'>&nbsp;</div> \endhtmlonly
        Wine,                       ///< \htmlonly <div style='background-color:#722F37; width:50px;'>&nbsp;</div> \endhtmlonly
        Wisteria,                   ///< \htmlonly <div style='background-color:#C9A0DC; width:50px;'>&nbsp;</div> \endhtmlonly
        Xanadu,                     ///< \htmlonly <div style='background-color:#738678; width:50px;'>&nbsp;</div> \endhtmlonly
        YaleBlue,                   ///< \htmlonly <div style='background-color:#0F4D92; width:50px;'>&nbsp;</div> \endhtmlonly
        Yellow,                     ///< \htmlonly <div style='background-color:#FFFF00; width:50px;'>&nbsp;</div> \endhtmlonly
        YellowGreen,                ///< \htmlonly <div style='background-color:#9ACD32; width:50px;'>&nbsp;</div> \endhtmlonly
        YellowOrange,               ///< \htmlonly <div style='background-color:#FFAE42; width:50px;'>&nbsp;</div> \endhtmlonly
        Zaffre,                     ///< \htmlonly <div style='background-color:#0014A8; width:50px;'>&nbsp;</div> \endhtmlonly
        ZinnwalditeBrown,           ///< \htmlonly <div style='background-color:#2C1608; width:50px;'>&nbsp;</div> \endhtmlonly
        COLOR_COUNT
        };

    /** @brief Constructs a color scale for a given range of values.
        @details Brews values within that range to a color representing its
         position on the color scale.

        @code
         using namespace Wisteria::Colors;

         ColorBrewer cb;
         cb.SetColorScale(
             {
             // the color for the min value
             *wxBLUE,
             // the color for the max value (because it's the last color added)
             ColorBrewer::GetColor(Color::Red)
             });

         std::vector<double> data =
            {
            50,   // max value (will be red)
            1,    // min value (will be blue)
            25.5  // in between value (will be purple)
            };

         auto res = cb.BrewColors(data.begin(), data.end());

         // an initializer list could also be used:
         // auto res = cb.BrewColors({ 50, 1, 25.5 });

         // res[0] will be red, res[1] will be blue, and res[2] will be purple
        @endcode
       */
    class ColorBrewer
        {
    public:
        /** @brief Creates a color from a Colors::Color value.
            @returns A color from a list of known colors.
            @param color The color ID to use.*/
        [[nodiscard]] static wxColour GetColor(const Colors::Color color);
        /** @brief Creates a color from a Colors::Color value and applies an opacity to it.
            @returns A color from a list of known colors.
            @param color The color ID to use.
            @param opacity The opacity to set the color.*/
        [[nodiscard]] static wxColour GetColor(const Colors::Color color, const uint8_t opacity)
            {
            auto foundColor = GetColor(color);
            return wxColour(foundColor.Red(), foundColor.Green(), foundColor.Blue(), opacity);
            }

        /** @brief Initializes the color scale to map to the range of values.
            @param start The start of a range of color objects (this will be the min value).
            @param end The end of a range of color objects (this will be the max value).*/
        template<typename T>
        void SetColorScale(const T start, const T end)
            {
            m_colorSpectrum.clear();
            m_colorSpectrum.insert(m_colorSpectrum.begin(), start, end);
            }
        /** @brief Initializes the color scale to map to the range of values.
            @param colors The colors to map values to. The first color in the list
             will map to the data's min value, and the last color will map to the data's max value.*/
        void SetColorScale(const std::initializer_list<wxColour>& colors)
            {
            m_colorSpectrum.clear();
            m_colorSpectrum.insert(m_colorSpectrum.begin(), colors.begin(), colors.end());
            }
        /** @brief Converts a range of numbers into a sequence of color values.
            @details The color values for each number represent where it falls on the color scale,
             relative to the overall range of values.
            @param start The start of the data.
            @param end The end of the data.
            @returns A vector of colors respective to each value in the data.
            @note Any NaN values in the range will be mapped to an invalid `wxColour`,
             so be sure to call `IsOk()` when using the returned colors.*/
        template<typename T>
        [[nodiscard]] std::vector<wxColour> BrewColors(const T start, const T end)
            {
            m_range.first = *std::min_element(start, end);
            m_range.second = *std::max_element(start, end);

            const size_t rangeSize = std::distance(start, end);

            std::vector<wxColour> colors(rangeSize, wxColour());
            for (size_t i = 0; i < rangeSize; ++i)
                { colors[i] = BrewColor(start[i]); }
            return colors;
            }
        /** @brief Converts a range of numbers into a sequence of color values.
            @details The color values for each number represent where it falls on the color scale,
             relative to the overall range of values.
            @param values The data to convert into a series of colors.
            @returns A vector of colors respective to each value in the data.
            @note Any NaN values in the range will be mapped to an invalid `wxColour`,
             so be sure to call `IsOk()` when using the returned colors.*/
        template<typename T>
        [[nodiscard]] std::vector<wxColour> BrewColors(const std::initializer_list<T>& values)
            {
            m_range.first = *std::min_element(values.begin(), values.end());
            m_range.second = *std::max_element(values.begin(), values.end());

            std::vector<wxColour> colors;
            colors.reserve(values.size());
            for (const auto& value : values)
                { colors.push_back(BrewColor(value)); }
            return colors;
            }
        /** @brief Returns the calculated min and max of the values from the last
             call to BrewColors().
            @returns The min and max of the values represented by the color scale.
            @sa BrewColors().*/
        [[nodiscard]] std::pair<double, double> GetRange() const noexcept
            { return m_range; }
#ifndef __UNITTEST
    private:
#endif
        /** @brief Converts a value from the range into a color laying on the
             color scale mapped to that range.
            @param value The value (should be within the original range) to convert.
            @returns The color that represents the value on our color scale.
             Will return an empty color if @c value is NaN;
             be sure to call `IsOk()` on returned color.
            @note This code is adapted from http://andrewnoske.com/wiki/Code_-_heatmaps_and_color_gradients.*/
        [[nodiscard]] wxColour BrewColor(const double value) const;
    private:
        std::pair<double,double> m_range{ 0,0 };
        std::vector<wxColour> m_colorSpectrum;
        static const std::vector<std::wstring> m_colors;
        };

    /// @brief Adjusts a color to contrast against another color.
    class ColorContrast
        {
    public:
        /// @brief Constructor.
        /// @param color The base color to contrast other colors against.
        ColorContrast(const wxColour& color) : m_baseColor(color) {}
        /// @returns A variation of @c color that is adjusted to contrast against the base color
        ///  (that was set in the constructor).
        /// @param color The color to adjust so that it contrasts.
        [[nodiscard]] wxColour Contrast(const wxColour& color);
        /// @returns A variation of @c color with a different opacity.
        /// @param color The base color to apply an opacity to.
        /// @param opacity The opacity to use for the new color.
        [[nodiscard]] static wxColour ChangeOpacity(wxColour color, const uint8_t opacity)
            { return wxColor(color.Red(), color.Green(), color.Blue(), opacity); }
        /// @brief Determines whether a color is dark (i.e., luminance is less than 50%).
        /// @param color The color to review.
        /// @returns `true` if the color's luminance is less than 50%.
        [[nodiscard]] static bool IsDark(const wxColour& color)
            { return (color.GetLuminance() < .5f); }
        /// @brief Determines whether a color is light (i.e., luminance is >= 50%).
        /// @param color The color to review.
        /// @returns `true` if the color's luminance is >= 50%.
        [[nodiscard]] static bool IsLight(const wxColour& color)
            { return !IsDark(color); }
        /// @returns A darkened version of a color.
        /// @param color The base color to darken.
        /// @param minimumLuminance The minimum darkness of the color,
        ///  ranging from 0.0 to 1.0 (the lower, the darker).
        [[nodiscard]] static wxColour Shade(wxColour color, const double minimumLuminance = 0.5f)
            {
            int darkenValue{ 100 };
            while (color.GetLuminance() > std::clamp(minimumLuminance, 0.0, 1.0) && darkenValue > 0)
                { color = color.ChangeLightness(--darkenValue); }
            return color;
            }
        /// @brief Returns a darker (shaded) or lighter (tinted) version of a color,
        ///  depending on how dark it is to begin with.
        ///  For example, black will be returned as dark gray, while white will return as an eggshell white.
        /// @param color The color to shade.
        /// @param shadeValue How much to lighten or darken a color. (Should be between 0.0 to 1.0.)
        /// @returns The shaded or tinted color.
        [[nodiscard]] static wxColour ShadeOrTint(const wxColour& color,
                                                  const double shadeValue = .20f)
            {
            return (IsDark(color) ?
                color.ChangeLightness(100 + std::clamp(static_cast<int>(shadeValue*100), 0, 100)) :
                color.ChangeLightness(100 - std::clamp(static_cast<int>(shadeValue*100), 0, 100)));
            }
        /// @brief Returns either black or white, depending on which better contrasts
        ///  against the specified color.
        /// @param color The color to contrast against to see if white or black should go on it.
        /// @returns Black or white; whichever contrasts better against @c color.
        [[nodiscard]] static wxColour BlackOrWhiteContrast(const wxColour& color)
            { return (IsDark(color) ? *wxWHITE : *wxBLACK); }
        /// @returns `true` if two colors' luminance values are close.
        /// @param color1 First color to compare.
        /// @param color2 Second color to compare.
        /// @param delta The difference threshold to use when comparing.
        ///  Should be between 0.0 to 1.0.
        [[nodiscard]] static bool AreColorsClose(const wxColour color1, const wxColour color2,
                                                 const double delta = .1f)
            {
            return (std::abs(color1.GetLuminance()-color2.GetLuminance())) <=
                    std::clamp(delta, 0.0, 1.0);
            }
        /// @brief Shades a color if close to another color (e.g., a background color).
        /// @param mainColor The color to shade (if necessary).
        /// @param secondaryColor The color to compare against.
        ///  If @c mainColor is close to this, then it will be shaded.
        /// @returns If @c mainColor is close to @c secondaryColor,
        ///  then returns a shaded version of @c mainColor; otherwise,
        ///  returns the original @c mainColor.
        [[nodiscard]] static wxColour ShadeOrTintIfClose(const wxColour& mainColor,
                                                         const wxColour& secondaryColor)
            {
            return AreColorsClose(mainColor, secondaryColor) ?
                ShadeOrTint(mainColor, .40f) : mainColor;
            }
    private:
        wxColour m_baseColor{ *wxWHITE };
        constexpr static double m_tolerance{ .5f };
        };

    /// @brief Color schemes to use for grouped data.
    namespace Schemes
        {
        /// @brief Base class for creating a color scheme.
        class ColorScheme
            {
        public:
            /** @brief Constructor.
                @param colors The initializer list of colors to fill the scheme with.
                @note A series of shaded or tinted versions of these colors will also
                 be added to this list of colors, essentially double the color count.*/
            ColorScheme(std::initializer_list<wxColour> colors) : m_colors(colors)
                {}
            /** @brief Gets the list of colors from the scheme.
                @returns The colors.*/
            [[nodiscard]] const std::vector<wxColour>& GetColors() const noexcept
                { return m_colors; }
            /** @brief Gets the color from a given index.
                @param index The index into the color list to return. If index is outside of the color
                 scheme but within double the size of the scheme, then scheme will "wrap around" and return
                 a shaded or tinted version. If outside of twice the number of colors, then returns black.

                 For example, if you have 8 colors and pass in an index of 7, the it will return the last color.
                 If you pass in index 9, then will return a shaded/tinted version of the second color. Index 15 will return
                 a shaded/tinted version of the last color, and index 16 will return black.
                @returns The color from a given index.*/
            [[nodiscard]] wxColour GetColor(const size_t index) const
                {
                return (index < m_colors.size()) ?
                    m_colors.at(index) :
                    (index < m_colors.size()*2) ?
                    ColorContrast::ShadeOrTint(m_colors.at(index%m_colors.size())) :
                    *wxBLACK;
                }
            /** @brief Gets the color from a given index and applies an opacity value to it.
                @param index The index into the color list to return. Returns black if index is invalid.
                @param opacity The opacity to set the color.
                @returns The color from given index.*/
            [[nodiscard]] wxColour GetColor(const size_t index, const uint8_t opacity) const
                {
                auto color = GetColor(index);
                return wxColour(color.Red(), color.Green(), color.Blue(), opacity);
                }
            /** @brief Adds a color to the scheme.
                @param color The color to add.*/
            void AddColor(const wxColour color)
                { m_colors.push_back(color); }
            /// @brief Removes all colors from the collection.
            void Clear() noexcept
                { m_colors.clear(); }
        protected:
            /// @brief The colors in the scheme.
            std::vector<wxColour> m_colors;
            };

        /** @brief An equidistant color scheme, with darker tones reminiscent
             of a sunset.
             \htmlonly
             <div style='background-color:#003F5C; width:50px;'>&nbsp;</div>
             <div style='background-color:#2F4B7C; width:50px;'>&nbsp;</div>
             <div style='background-color:#665191; width:50px;'>&nbsp;</div>
             <div style='background-color:#A05195; width:50px;'>&nbsp;</div>
             <div style='background-color:#D45087; width:50px;'>&nbsp;</div>
             <div style='background-color:#F95D6A; width:50px;'>&nbsp;</div>
             <div style='background-color:#FF7C43; width:50px;'>&nbsp;</div>
             <div style='background-color:#FFA600; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Dusk : public ColorScheme
            {
        public:
            Dusk() : ColorScheme({ wxColour(L"#003F5C"), wxColour(L"#2F4B7C"),
                                   wxColour(L"#665191"), wxColour(L"#A05195"),
                                   wxColour(L"#D45087"), wxColour(L"#F95D6A"),
                                   wxColour(L"#FF7C43"), wxColour(L"#FFA600") })
                {}
            };

        /** @brief An Earth tones themed color scheme.
             \htmlonly
             <div style='background-color:rgb(186,150,155); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(110,80,69); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(202,80,69); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(102,131,145); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(154,131,97); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(41,109,91); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(140,74,86); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(238,221,130); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(176,48,96); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(205,150,205); width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class EarthTones : public ColorScheme
            {
        public:
            EarthTones() : ColorScheme({ wxColour(186,150,155), wxColour(110,80,69),
                                         wxColour(202,80,69), wxColour(102,131,145),
                                         wxColour(154,131,97), wxColour(41,109,91),
                                         wxColour(140,74,86), wxColour(238,221,130),
                                         wxColour(176,48,96), wxColour(205,150,205) })
                {}
            };

        /** @brief A 1920s themed color scheme.
             \htmlonly
             <div style='background-color:#9E3E33; width:50px;'>&nbsp;</div>
             <div style='background-color:#F1BFB1; width:50px;'>&nbsp;</div>
             <div style='background-color:#CBD0C2; width:50px;'>&nbsp;</div>
             <div style='background-color:#598C74; width:50px;'>&nbsp;</div>
             <div style='background-color:#AB7878; width:50px;'>&nbsp;</div>
             <div style='background-color:#C6B9B8; width:50px;'>&nbsp;</div>
             <div style='background-color:#ABD1C9; width:50px;'>&nbsp;</div>
             <div style='background-color:#014E4C; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1920s : public ColorScheme
            {
        public:
            Decade1920s() : ColorScheme({
                ColorBrewer::GetColor(Color::ChineseRed),
                ColorBrewer::GetColor(Color::JazzAgeCoral),
                ColorBrewer::GetColor(Color::Frostwork),
                ColorBrewer::GetColor(Color::Alexandrite),
                ColorBrewer::GetColor(Color::SalonRose),
                ColorBrewer::GetColor(Color::StudioMauve),
                ColorBrewer::GetColor(Color::BlueSky),
                ColorBrewer::GetColor(Color::BluePeacock) })
                {}
            };

        /** @brief A 1940s themed color scheme.
             \htmlonly
             <div style='background-color:#4B5645; width:50px;'>&nbsp;</div>
             <div style='background-color:#ACB19F; width:50px;'>&nbsp;</div>
             <div style='background-color:#F0CDA0; width:50px;'>&nbsp;</div>
             <div style='background-color:#C2CCC4; width:50px;'>&nbsp;</div>
             <div style='background-color:#CD9C85; width:50px;'>&nbsp;</div>
             <div style='background-color:#DEC3B9; width:50px;'>&nbsp;</div>
             <div style='background-color:#BC9C9E; width:50px;'>&nbsp;</div>
             <div style='background-color:#623F45; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1940s : public ColorScheme
            {
        public:
            Decade1940s() : ColorScheme({
                ColorBrewer::GetColor(Color::VogueGreen),
                ColorBrewer::GetColor(Color::CascadeGreen),
                ColorBrewer::GetColor(Color::BelvedereCream),
                ColorBrewer::GetColor(Color::CopenBlue),
                ColorBrewer::GetColor(Color::RoseTan),
                ColorBrewer::GetColor(Color::PinkShadow),
                ColorBrewer::GetColor(Color::Orchid),
                ColorBrewer::GetColor(Color::DeepMaroon) })
                {}
            };

        /** @brief A 1950s themed color scheme.
             \htmlonly
             <div style='background-color:#E1D286; width:50px;'>&nbsp;</div>
             <div style='background-color:#A489A0; width:50px;'>&nbsp;</div>
             <div style='background-color:#8AC6BD; width:50px;'>&nbsp;</div>
             <div style='background-color:#DAB5B4; width:50px;'>&nbsp;</div>
             <div style='background-color:#888782; width:50px;'>&nbsp;</div>
             <div style='background-color:#F0D39D; width:50px;'>&nbsp;</div>
             <div style='background-color:#C9AA98; width:50px;'>&nbsp;</div>
             <div style='background-color:#CD717B; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1950s : public ColorScheme
            {
        public:
            Decade1950s() : ColorScheme({
                ColorBrewer::GetColor(Color::Chartreuse),
                ColorBrewer::GetColor(Color::RadiantLilac),
                ColorBrewer::GetColor(Color::HolidayTurquoise),
                ColorBrewer::GetColor(Color::Appleblossom),
                ColorBrewer::GetColor(Color::ClassicFrenchGray),
                ColorBrewer::GetColor(Color::SunbeamYellow),
                ColorBrewer::GetColor(Color::PinkyBeige),
                ColorBrewer::GetColor(Color::PinkFlamingo) })
                {}
            };

        /** @brief A 1960s themed color scheme.
             \htmlonly
             <div style='background-color:#EC8430; width:50px;'>&nbsp;</div>
             <div style='background-color:#FED340; width:50px;'>&nbsp;</div>
             <div style='background-color:#B54D7F; width:50px;'>&nbsp;</div>
             <div style='background-color:#D9C661; width:50px;'>&nbsp;</div>
             <div style='background-color:#716998; width:50px;'>&nbsp;</div>
             <div style='background-color:#2F2F30; width:50px;'>&nbsp;</div>
             <div style='background-color:#EDECE6; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1960s : public ColorScheme
            {
        public:
            Decade1960s() : ColorScheme({
                ColorBrewer::GetColor(Color::Navel),
                ColorBrewer::GetColor(Color::Daisy),
                ColorBrewer::GetColor(Color::ExuberantPink),
                ColorBrewer::GetColor(Color::Frolic),
                ColorBrewer::GetColor(Color::ForgetMeNot),
                ColorBrewer::GetColor(Color::TricornBlack),
                ColorBrewer::GetColor(Color::PureWhite) })
                {}
            };

        /** @brief A 1970s themed color scheme.
             \htmlonly
             <div style='background-color:#857C5D; width:50px;'>&nbsp;</div>
             <div style='background-color:#D28240; width:50px;'>&nbsp;</div>
             <div style='background-color:#D69969; width:50px;'>&nbsp;</div>
             <div style='background-color:#815D40; width:50px;'>&nbsp;</div>
             <div style='background-color:#C9B29C; width:50px;'>&nbsp;</div>
             <div style='background-color:#FBCB78; width:50px;'>&nbsp;</div>
             <div style='background-color:#B1975F; width:50px;'>&nbsp;</div>
             <div style='background-color:#DA9100; width:50px;'>&nbsp;</div>
             <div style='background-color:#FFDB58; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1970s : public ColorScheme
            {
        public:
            Decade1970s() : ColorScheme({
                ColorBrewer::GetColor(Color::Avocado),
                ColorBrewer::GetColor(Color::AmberWave),
                ColorBrewer::GetColor(Color::FolksyGold),
                ColorBrewer::GetColor(Color::JuteBrown),
                ColorBrewer::GetColor(Color::PracticalBeige),
                ColorBrewer::GetColor(Color::Afternoon),
                ColorBrewer::GetColor(Color::EdgyGold),
                ColorBrewer::GetColor(Color::HarvestGold),
                ColorBrewer::GetColor(Color::MustardYellow) })
                {}
            };

        /** @brief A 1980s themed color scheme.
             \htmlonly
             <div style='background-color:#b89d9a; width:50px;'>&nbsp;</div>
             <div style='background-color:#ede1ce; width:50px;'>&nbsp;</div>
             <div style='background-color:#8aa3b1; width:50px;'>&nbsp;</div>
             <div style='background-color:#c6c0b6; width:50px;'>&nbsp;</div>
             <div style='background-color:#f4d3b3; width:50px;'>&nbsp;</div>
             <div style='background-color:#75b9ae; width:50px;'>&nbsp;</div>
             <div style='background-color:#ebcecb; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1980s : public ColorScheme
            {
        public:
            Decade1980s() : ColorScheme({
                ColorBrewer::GetColor(Color::DressyRose),
                ColorBrewer::GetColor(Color::CasaBlanca),
                ColorBrewer::GetColor(Color::FavoriteJeans),
                ColorBrewer::GetColor(Color::ColonnadeGray),
                ColorBrewer::GetColor(Color::FlatteringPeach),
                ColorBrewer::GetColor(Color::CooledBlue),
                ColorBrewer::GetColor(Color::RosyOutlook) })
                {}
            };

        /** @brief A 1990s themed color scheme.
             \htmlonly
             <div style='background-color:#E2B6A7; width:50px;'>&nbsp;</div>
             <div style='background-color:#CDBFB0; width:50px;'>&nbsp;</div>
             <div style='background-color:#7B3730; width:50px;'>&nbsp;</div>
             <div style='background-color:#F0EADC; width:50px;'>&nbsp;</div>
             <div style='background-color:#CFC0AB; width:50px;'>&nbsp;</div>
             <div style='background-color:#B2AC96; width:50px;'>&nbsp;</div>
             <div style='background-color:#C0A98B; width:50px;'>&nbsp;</div>
             <div style='background-color:#CDB592; width:50px;'>&nbsp;</div>
             <div style='background-color:#B0785C; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1990s : public ColorScheme
            {
        public:
            Decade1990s() : ColorScheme({
                ColorBrewer::GetColor(Color::SmokySalmon),
                ColorBrewer::GetColor(Color::BungalowBeige),
                ColorBrewer::GetColor(Color::Fireweed),
                ColorBrewer::GetColor(Color::DoverWhite),
                ColorBrewer::GetColor(Color::UrbanPutty),
                ColorBrewer::GetColor(Color::SvelteSage),
                ColorBrewer::GetColor(Color::BasketBeige),
                ColorBrewer::GetColor(Color::WholeWheat),
                ColorBrewer::GetColor(Color::SpicedCider) })
                {}
            };

        /** @brief A 2000s themed color scheme.
             \htmlonly
             <div style='background-color:#564537; width:50px;'>&nbsp;</div>
             <div style='background-color:#BAA185; width:50px;'>&nbsp;</div>
             <div style='background-color:#D7C5AE; width:50px;'>&nbsp;</div>
             <div style='background-color:#E5E2DA; width:50px;'>&nbsp;</div>
             <div style='background-color:#88ABB4; width:50px;'>&nbsp;</div>
             <div style='background-color:#CDD2CA; width:50px;'>&nbsp;</div>
             <div style='background-color:#C2CFCF; width:50px;'>&nbsp;</div>
             <div style='background-color:#B4CCC9; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade2000s : public ColorScheme
            {
        public:
            Decade2000s() : ColorScheme({
                ColorBrewer::GetColor(Color::HickorySmoke),
                ColorBrewer::GetColor(Color::Latte),
                ColorBrewer::GetColor(Color::KilimBeige),
                ColorBrewer::GetColor(Color::OrigamiWhite),
                ColorBrewer::GetColor(Color::Aquitaine),
                ColorBrewer::GetColor(Color::SeaSalt),
                ColorBrewer::GetColor(Color::Tradewind),
                ColorBrewer::GetColor(Color::Watery) })
                {}
            };

        /** @brief An %October themed color scheme, including traditional
             Halloween colors.)
             \htmlonly
             <div style='background-color:#FFA500; width:50px;'>&nbsp;</div>
             <div style='background-color:#000000; width:50px;'>&nbsp;</div>
             <div style='background-color:#FFFF00; width:50px;'>&nbsp;</div>
             <div style='background-color:#A52A2A; width:50px;'>&nbsp;</div>
             <div style='background-color:#DC143C; width:50px;'>&nbsp;</div>
             <div style='background-color:#B6B8A5; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class October : public ColorScheme
            {
        public:
            October() : ColorScheme({
                ColorBrewer::GetColor(Color::Orange),
                ColorBrewer::GetColor(Color::Black),
                ColorBrewer::GetColor(Color::Yellow),
                ColorBrewer::GetColor(Color::Brown),
                ColorBrewer::GetColor(Color::Crimson),
                ColorBrewer::GetColor(Color::OctoberMist) })
                {}
            };

        /** @brief %Slytherin's house color scheme.
             \htmlonly
             <div style='background-color:#1A472A; width:50px;'>&nbsp;</div>
             <div style='background-color:#2A623D; width:50px;'>&nbsp;</div>
             <div style='background-color:#5D5D5D; width:50px;'>&nbsp;</div>
             <div style='background-color:#AAAAAA; width:50px;'>&nbsp;</div>
             <div style='background-color:#000000; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Slytherin : public ColorScheme
            {
        public:
            Slytherin() : ColorScheme({
                ColorBrewer::GetColor(Color::Slytherin1),
                ColorBrewer::GetColor(Color::Slytherin2),
                ColorBrewer::GetColor(Color::Slytherin3),
                ColorBrewer::GetColor(Color::Slytherin4),
                ColorBrewer::GetColor(Color::Black) })
                {}
            };
        }

    /// @private
    /// @returns A label used for a grouping label when grouping should not actually be used.
    ///  Basically, a way to bin data without it actually creating a legend on the graph.
    [[nodiscard]] inline wxString NoGroupLabel()
        { return L"__NO_GROUPING__"; }
    }
/** @}*/

#endif //__WISTERIA_COLORBREWER_H__
