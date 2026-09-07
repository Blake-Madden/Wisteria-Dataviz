///////////////////////////////////////////////////////////////////////////////
// Name:        geofeature.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "geofeature.h"
#include <algorithm>
#include <cmath>
#include <optional>

namespace Wisteria::Data
    {
    //---------------------------------------------------
    bool GeoBoundingBox::IsOk() const noexcept
        {
        return std::isfinite(m_minLongitude) && std::isfinite(m_maxLongitude) &&
               std::isfinite(m_minLatitude) && std::isfinite(m_maxLatitude) &&
               m_minLongitude <= m_maxLongitude && m_minLatitude <= m_maxLatitude;
        }

    //---------------------------------------------------
    void GeoBoundingBox::Encompass(const GeoCoordinate& coordinate) noexcept
        {
        if (!std::isfinite(coordinate.m_longitude) || !std::isfinite(coordinate.m_latitude))
            {
            return;
            }
        m_minLongitude = std::min(m_minLongitude, coordinate.m_longitude);
        m_maxLongitude = std::max(m_maxLongitude, coordinate.m_longitude);
        m_minLatitude = std::min(m_minLatitude, coordinate.m_latitude);
        m_maxLatitude = std::max(m_maxLatitude, coordinate.m_latitude);
        }

    //---------------------------------------------------
    void GeoBoundingBox::Encompass(const GeoBoundingBox& box) noexcept
        {
        if (!box.IsOk())
            {
            return;
            }
        Encompass(GeoCoordinate{ box.m_minLongitude, box.m_minLatitude });
        Encompass(GeoCoordinate{ box.m_maxLongitude, box.m_maxLatitude });
        }

    //---------------------------------------------------
    double GeoBoundingBox::GetWidth() const noexcept
        {
        return IsOk() ? (m_maxLongitude - m_minLongitude) : 0.0;
        }

    //---------------------------------------------------
    double GeoBoundingBox::GetHeight() const noexcept
        {
        return IsOk() ? (m_maxLatitude - m_minLatitude) : 0.0;
        }

    //---------------------------------------------------
    GeoCoordinate GeoBoundingBox::GetCenter() const noexcept
        {
        return GeoCoordinate{ (m_minLongitude + m_maxLongitude) / 2.0,
                              (m_minLatitude + m_maxLatitude) / 2.0 };
        }

    //---------------------------------------------------
    wxString GeoRegion::GetAttribute(const wxString& fieldName, const wxString& defaultValue) const
        {
        const auto foundAttribute = m_attributes.find(fieldName);
        return (foundAttribute != m_attributes.cend()) ? foundAttribute->second : defaultValue;
        }

    //---------------------------------------------------
    bool MakeGeoCoordinate(const double longitude, const double latitude,
                           GeoCoordinate& coordinateOut) noexcept
        {
        if (!std::isfinite(longitude) || !std::isfinite(latitude))
            {
            return false;
            }
        coordinateOut.m_longitude = std::clamp(longitude, -180.0, 180.0);
        coordinateOut.m_latitude = std::clamp(latitude, -90.0, 90.0);
        return true;
        }

    //---------------------------------------------------
    std::optional<size_t> GeoFeatureReader::FindRegion(const wxString& name) const
        {
        for (size_t index = 0; index < m_regions.size(); ++index)
            {
            if (m_regions[index].m_name == name)
                {
                return index;
                }
            }
        return std::nullopt;
        }
    } // namespace Wisteria::Data
