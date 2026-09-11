///////////////////////////////////////////////////////////////////////////////
// Name:        colorschemenames.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "colorschemenames.h"
#include <map>

using namespace Wisteria::Colors::Schemes;

//------------------------------------------------------
const std::vector<std::pair<wxString, wxString>>& ColorSchemeCatalog::GetEntries()
    {
    static const std::vector<std::pair<wxString, wxString>> entries = {
        { _(L"Arctic Chill"), L"arcticchill" },
        { _(L"Back to School"), L"backtoschool" },
        { _(L"Box of Chocolates"), L"boxofchocolates" },
        { _(L"Campfire"), L"campfire" },
        { _(L"Coffee Shop"), L"coffeeshop" },
        { _(L"Cosmopolitan"), L"cosmopolitan" },
        { _(L"Day and Night"), L"dayandnight" },
        { _(L"Decade 1920s"), L"decade1920s" },
        { _(L"Decade 1940s"), L"decade1940s" },
        { _(L"Decade 1950s"), L"decade1950s" },
        { _(L"Decade 1960s"), L"decade1960s" },
        { _(L"Decade 1970s"), L"decade1970s" },
        { _(L"Decade 1980s"), L"decade1980s" },
        { _(L"Decade 1990s"), L"decade1990s" },
        { _(L"Decade 2000s"), L"decade2000s" },
        { _(L"Dusk"), L"dusk" },
        { _(L"Earth Tones"), L"earthtones" },
        { _(L"Fresh Flowers"), L"freshflowers" },
        { _(L"Ice Cream"), L"icecream" },
        { _(L"Meadow Sunset"), L"meadowsunset" },
        { _(L"Nautical"), L"nautical" },
        { _(L"October"), L"october" },
        { _(L"Produce Section"), L"producesection" },
        { _(L"Rolling Thunder"), L"rollingthunder" },
        { _(L"Seasons"), L"seasons" },
        { _(L"Semesters"), L"semesters" },
        { _(L"Shabby Chic"), L"shabbychic" },
        { _(L"Slytherin"), L"slytherin" },
        { _(L"Spring"), L"spring" },
        { _(L"Tasty Waves"), L"tastywaves" },
        { _(L"Typewriter"), L"typewriter" },
        { _(L"Urban Oasis"), L"urbanoasis" },
    };
    return entries;
    }

//------------------------------------------------------
std::shared_ptr<ColorScheme> ColorSchemeCatalog::FromKey(const wxString& key)
    {
    if (key.empty())
        {
        return nullptr;
        }
    static const std::map<wxString, std::shared_ptr<ColorScheme>> schemes = {
        { L"arcticchill", std::make_shared<ArcticChill>() },
        { L"backtoschool", std::make_shared<BackToSchool>() },
        { L"boxofchocolates", std::make_shared<BoxOfChocolates>() },
        { L"campfire", std::make_shared<Campfire>() },
        { L"coffeeshop", std::make_shared<CoffeeShop>() },
        { L"cosmopolitan", std::make_shared<Cosmopolitan>() },
        { L"dayandnight", std::make_shared<DayAndNight>() },
        { L"decade1920s", std::make_shared<Decade1920s>() },
        { L"decade1940s", std::make_shared<Decade1940s>() },
        { L"decade1950s", std::make_shared<Decade1950s>() },
        { L"decade1960s", std::make_shared<Decade1960s>() },
        { L"decade1970s", std::make_shared<Decade1970s>() },
        { L"decade1980s", std::make_shared<Decade1980s>() },
        { L"decade1990s", std::make_shared<Decade1990s>() },
        { L"decade2000s", std::make_shared<Decade2000s>() },
        { L"dusk", std::make_shared<Dusk>() },
        { L"earthtones", std::make_shared<EarthTones>() },
        { L"freshflowers", std::make_shared<FreshFlowers>() },
        { L"icecream", std::make_shared<IceCream>() },
        { L"meadowsunset", std::make_shared<MeadowSunset>() },
        { L"nautical", std::make_shared<Nautical>() },
        { L"october", std::make_shared<October>() },
        { L"producesection", std::make_shared<ProduceSection>() },
        { L"rollingthunder", std::make_shared<RollingThunder>() },
        { L"seasons", std::make_shared<Seasons>() },
        { L"semesters", std::make_shared<Semesters>() },
        { L"shabbychic", std::make_shared<ShabbyChic>() },
        { L"slytherin", std::make_shared<Slytherin>() },
        { L"spring", std::make_shared<Spring>() },
        { L"tastywaves", std::make_shared<TastyWaves>() },
        { L"typewriter", std::make_shared<Typewriter>() },
        { L"urbanoasis", std::make_shared<UrbanOasis>() },
    };
    const auto found{ schemes.find(key.Lower()) };
    return (found != schemes.cend()) ? found->second : nullptr;
    }

//------------------------------------------------------
wxString ColorSchemeCatalog::ToKey(const std::shared_ptr<ColorScheme>& scheme)
    {
    if (scheme == nullptr)
        {
        return {};
        }
    // clang-format off
    if (scheme->IsKindOf(wxCLASSINFO(ArcticChill)))
        { return L"arcticchill"; }
    if (scheme->IsKindOf(wxCLASSINFO(BackToSchool)))
        { return L"backtoschool"; }
    if (scheme->IsKindOf(wxCLASSINFO(BoxOfChocolates)))
        { return L"boxofchocolates"; }
    if (scheme->IsKindOf(wxCLASSINFO(Campfire)))
        { return L"campfire"; }
    if (scheme->IsKindOf(wxCLASSINFO(CoffeeShop)))
        { return L"coffeeshop"; }
    if (scheme->IsKindOf(wxCLASSINFO(Cosmopolitan)))
        { return L"cosmopolitan"; }
    if (scheme->IsKindOf(wxCLASSINFO(DayAndNight)))
        { return L"dayandnight"; }
    if (scheme->IsKindOf(wxCLASSINFO(Decade1920s)))
        { return L"decade1920s"; }
    if (scheme->IsKindOf(wxCLASSINFO(Decade1940s)))
        { return L"decade1940s"; }
    if (scheme->IsKindOf(wxCLASSINFO(Decade1950s)))
        { return L"decade1950s"; }
    if (scheme->IsKindOf(wxCLASSINFO(Decade1960s)))
        { return L"decade1960s"; }
    if (scheme->IsKindOf(wxCLASSINFO(Decade1970s)))
        { return L"decade1970s"; }
    if (scheme->IsKindOf(wxCLASSINFO(Decade1980s)))
        { return L"decade1980s"; }
    if (scheme->IsKindOf(wxCLASSINFO(Decade1990s)))
        { return L"decade1990s"; }
    if (scheme->IsKindOf(wxCLASSINFO(Decade2000s)))
        { return L"decade2000s"; }
    // a plain (sliced) ColorScheme with Dusk's colors, e.g. Settings::GetDefaultColorScheme(),
    // is not a Dusk instance by RTTI, so fall back to a value comparison
    if (scheme->IsKindOf(wxCLASSINFO(Dusk)) || *scheme == Dusk{})
        { return L"dusk"; }
    if (scheme->IsKindOf(wxCLASSINFO(EarthTones)))
        { return L"earthtones"; }
    if (scheme->IsKindOf(wxCLASSINFO(FreshFlowers)))
        { return L"freshflowers"; }
    if (scheme->IsKindOf(wxCLASSINFO(IceCream)))
        { return L"icecream"; }
    if (scheme->IsKindOf(wxCLASSINFO(MeadowSunset)))
        { return L"meadowsunset"; }
    if (scheme->IsKindOf(wxCLASSINFO(Nautical)))
        { return L"nautical"; }
    if (scheme->IsKindOf(wxCLASSINFO(October)))
        { return L"october"; }
    if (scheme->IsKindOf(wxCLASSINFO(ProduceSection)))
        { return L"producesection"; }
    if (scheme->IsKindOf(wxCLASSINFO(RollingThunder)))
        { return L"rollingthunder"; }
    if (scheme->IsKindOf(wxCLASSINFO(Seasons)))
        { return L"seasons"; }
    if (scheme->IsKindOf(wxCLASSINFO(Semesters)))
        { return L"semesters"; }
    if (scheme->IsKindOf(wxCLASSINFO(ShabbyChic)))
        { return L"shabbychic"; }
    if (scheme->IsKindOf(wxCLASSINFO(Slytherin)))
        { return L"slytherin"; }
    if (scheme->IsKindOf(wxCLASSINFO(Spring)))
        { return L"spring"; }
    if (scheme->IsKindOf(wxCLASSINFO(TastyWaves)))
        { return L"tastywaves"; }
    if (scheme->IsKindOf(wxCLASSINFO(Typewriter)))
        { return L"typewriter"; }
    if (scheme->IsKindOf(wxCLASSINFO(UrbanOasis)))
        { return L"urbanoasis"; }
    // clang-format on
    return {};
    }
