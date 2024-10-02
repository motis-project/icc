#include "gtest/gtest.h"

#include "motis/config.h"

using namespace motis;
using namespace std::string_literals;
using namespace nigiri;

TEST(motis, config) {
  auto const c = config{
      .first_day_ = "2024-10-02",
      .num_days_ = 2U,
      .timetables_ =
          {{{"de",
             {.path_ = "delfi.gtfs.zip",
              .clasz_bikes_allowed_ = {{{"LONG_DISTANCE", false},
                                        {"REGIONAL_FAST", true}}},
              .rt_ =
                  {{{.url_ =
                         R"(https://stc.traines.eu/mirror/german-delfi-gtfs-rt/latest.gtfs-rt.pbf)",
                     .headers_ = {{{"Authorization", "test"}}}}}}}},
            {"nl",
             {.path_ = "nl.gtfs.zip",
              .rt_ = {{{.url_ = R"(https://gtfs.ovapi.nl/nl/trainUpdates.pb)"},
                       {.url_ =
                            R"(https://gtfs.ovapi.nl/nl/tripUpdates.pb)"}}}}}}},
      .osm_ = {"europe-latest.osm.pbf"},
      .assistance_times_ = {"assistance.csv"}};

  EXPECT_EQ(R"(
first_day: 2024-10-02
num_days: 2
merge_dupes_intra_src: false
merge_dupes_inter_src: false
max_footpath_length: 15
timetables:
  de:
    path: delfi.gtfs.zip
    default_bikes_allowed: false
    clasz_bikes_allowed:
      LONG_DISTANCE: false
      REGIONAL_FAST: true
    rt:
      - url: https://stc.traines.eu/mirror/german-delfi-gtfs-rt/latest.gtfs-rt.pbf
        headers:
          Authorization: test
  nl:
    path: nl.gtfs.zip
    default_bikes_allowed: false
    rt:
      - url: https://gtfs.ovapi.nl/nl/trainUpdates.pb
      - url: https://gtfs.ovapi.nl/nl/tripUpdates.pb
osm: europe-latest.osm.pbf
assistance_times: assistance.csv
)"s,
            (std::stringstream{} << "\n"
                                 << c << "\n")
                .str());

  EXPECT_EQ(c, config::read(R"(
first_day: 2024-10-02
num_days: 2
max_footpath_length: 15
timetables:
  de:
    path: delfi.gtfs.zip
    clasz_bikes_allowed:
      LONG_DISTANCE: false
      REGIONAL_FAST: true
    rt:
      - url: https://stc.traines.eu/mirror/german-delfi-gtfs-rt/latest.gtfs-rt.pbf
        headers:
          Authorization: test
  nl:
    path: nl.gtfs.zip
    rt:
      - url: https://gtfs.ovapi.nl/nl/trainUpdates.pb
      - url: https://gtfs.ovapi.nl/nl/tripUpdates.pb
osm: europe-latest.osm.pbf
assistance_times: assistance.csv
)"s));
}