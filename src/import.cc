#include "motis/import.h"

#include <vector>

#include "nigiri/loader/gtfs/load_timetable.h"
#include "nigiri/loader/init_finish.h"
#include "nigiri/rt/create_rt_timetable.h"
#include "nigiri/rt/rt_timetable.h"
#include "nigiri/timetable.h"

#include "osr/extract/extract.h"
#include "osr/lookup.h"
#include "osr/platforms.h"
#include "osr/routing/route.h"
#include "osr/ways.h"

#include "motis/compute_footpaths.h"
#include "motis/data.h"
#include "motis/elevators/elevators.h"
#include "motis/elevators/match_elevator.h"
#include "motis/elevators/parse_fasta.h"
#include "motis/endpoints/routing.h"
#include "motis/get_loc.h"
#include "motis/match_platforms.h"
#include "motis/tt_location_rtree.h"
#include "motis/update_rtt_td_footpaths.h"

namespace fs = std::filesystem;

namespace motis {

enum output { kTimetable, kAdr, kOsr, kTiles };

struct task {
  std::function<bool(config const&, data const&)> ready_;
  std::function<void(config const&, fs::path const&, data&)> fn_;
};

auto const osr =
    task{.ready_ = [](config const& c, data const&) -> bool {
           return c.has_feature(feature::STREET_ROUTING);
         },
         .fn_ =
             [](config const& c, fs::path const& data_path, data& d) {
               auto const osr_path = data_path / "osr";
               osr::extract(true, fs::path{*c.osm_}, osr_path);
               d.load_osr(data_path);
             }};

auto const tasks = std::vector<task>{osr};

void import(config const& c, fs::path const& data_path) {}

}  // namespace motis