#include "GpsDemo.h"

#include "DekiGPS.h"

#include <deki/Time.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

namespace
{
// Howard Hinnant's civil_from_days: the date of a day count from 1970-01-01.
void CivilFromDays(int64_t days, int& year, int& month, int& day)
{
    days += 719468;
    const int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(days - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    day = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    month = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    year = static_cast<int>(yoe + era * 400 + (month <= 2));
}

const Deki::Color kGood(0x7D, 0xE0, 0x8C);
const Deki::Color kWaiting(0xFF, 0xC8, 0x57);
const Deki::Color kBad(0xFF, 0x7A, 0x8A);
}  // namespace

void GpsDemo::Start()
{
    Refresh();
}

void GpsDemo::Update()
{
    m_SinceRefreshMs += Deki::Time::GetDeltaTimeF();
    if (m_SinceRefreshMs >= 500.0f)
    {
        m_SinceRefreshMs = 0.0f;
        Refresh();
    }
}

void GpsDemo::Refresh()
{
    auto* statusText = status.Get();
    auto* detailsText = details.Get();
    auto* gps = DekiGps::DekiGPS::GetCurrent();
    if (!gps)
    {
        if (statusText)
        {
            statusText->SetText("No GPS on this board");
            statusText->SetColor(kBad);
        }
        return;
    }

    const DekiGps::DekiGPSStatus s = gps->Status();
    const bool fix = gps->HasLiveFix();

    if (statusText)
    {
        if (fix)
        {
            statusText->SetText("Fix");
            statusText->SetColor(kGood);
        }
        else if (s.sentences == 0)
        {
            statusText->SetText("Waiting for the receiver...");
            statusText->SetColor(kBad);
        }
        else
        {
            statusText->SetText("Searching for satellites...");
            statusText->SetColor(kWaiting);
        }
    }

    if (!detailsText)
    {
        return;
    }
    std::string text;
    char line[80];

    if (fix)
    {
        const DekiGps::DekiGPSLocation at = gps->Current();
        std::snprintf(line, sizeof(line), "%.5f %c  %.5f %c\n", std::fabs(at.latitude), at.latitude >= 0 ? 'N' : 'S',
                      std::fabs(at.longitude), at.longitude >= 0 ? 'E' : 'W');
        text += line;
    }
    else
    {
        text += "No position yet: go outside,\nsky in view\n";
    }

    if (s.satellitesUsed >= 0 || s.satellitesInView >= 0)
    {
        std::snprintf(line, sizeof(line), "Satellites: %d used, %d seen\n", s.satellitesUsed < 0 ? 0 : s.satellitesUsed,
                      s.satellitesInView < 0 ? 0 : s.satellitesInView);
        text += line;
    }
    if (s.hasAltitude)
    {
        std::snprintf(line, sizeof(line), "Altitude: %.0f m\n", s.altitudeMeters);
        text += line;
    }
    if (gps->HasUTC())
    {
        const int64_t t = gps->CurrentUTCEpochSeconds();
        int year = 0, month = 0, day = 0;
        CivilFromDays(t / 86400, year, month, day);
        const int64_t secondOfDay = t % 86400;
        std::snprintf(line, sizeof(line), "UTC %04d-%02d-%02d %02d:%02d:%02d\n", year, month, day,
                      static_cast<int>(secondOfDay / 3600), static_cast<int>(secondOfDay / 60 % 60),
                      static_cast<int>(secondOfDay % 60));
        text += line;
    }
    if (s.sentences > 0)
    {
        std::snprintf(line, sizeof(line), "NMEA: %u at %u baud", static_cast<unsigned>(s.sentences),
                      static_cast<unsigned>(s.baudRate));
    }
    else
    {
        std::snprintf(line, sizeof(line), "No NMEA yet (%u bytes read)", static_cast<unsigned>(s.bytes));
    }
    text += line;
    detailsText->SetText(text);
}
