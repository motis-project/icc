#include "gtest/gtest.h"

#include "motis/config.h"

using namespace motis;
using namespace std::string_view_literals;

TEST(motis, config) {
  config::read(R"(
timetables:
  de:
    path: delfi.gtfs.zip
    rt:
      - url: https://stc.traines.eu/mirror/german-delfi-gtfs-rt/latest.gtfs-rt.pbf
        headers:
          Authorization: test
  nl:
    path: nl.gtfs.zip
    rt:
      - url: https://gtfs.ovapi.nl/nl/trainUpdates.pb
      - url: https://gtfs.ovapi.nl/nl/tripUpdates.pb
assistance: assistance_times.csv
osm: europe-latest.osm.pbf
)"sv);
}