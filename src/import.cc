#include "motis/import.h"

#include <vector>

#include "fmt/ranges.h"

#include "nigiri/loader/assistance.h"
#include "nigiri/loader/dir.h"
#include "nigiri/loader/gtfs/load_timetable.h"
#include "nigiri/loader/init_finish.h"
#include "nigiri/loader/load.h"
#include "nigiri/clasz.h"
#include "nigiri/common/parse_date.h"
#include "nigiri/rt/create_rt_timetable.h"
#include "nigiri/rt/rt_timetable.h"
#include "nigiri/shape.h"
#include "nigiri/timetable.h"

#include "osr/extract/extract.h"
#include "osr/lookup.h"
#include "osr/platforms.h"
#include "osr/routing/route.h"
#include "osr/ways.h"

#include "adr/adr.h"
#include "adr/typeahead.h"

#include "motis/adr_extend_tt.h"
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
namespace n = nigiri;
namespace nl = nigiri::loader;

namespace motis {

struct task {
  friend std::ostream& operator<<(std::ostream& out, task const& t) {
    return out << t.name_;
  };
  std::string name_;
  std::function<bool()> should_run_;
  std::function<bool()> ready_;
  std::function<void()> run_;
};

}  // namespace motis

template <>
struct fmt::formatter<motis::task> : fmt::ostream_formatter {};

namespace motis {

void import(config const& c, fs::path const& data_path) {
  auto d = data{data_path};

  auto osr =
      task{"osr", [&]() { return c.has_feature(feature::STREET_ROUTING); },
           [&]() { return true; },
           [&]() {
             osr::extract(true, fs::path{*c.osm_}, data_path / "osr");
             d.load_osr();
           }};

  auto adr = task{"adr",
                  [&]() {
                    return c.has_feature(feature::GEOCODING) ||
                           c.has_feature(feature::REVERSE_GEOCODING);
                  },
                  [&]() { return true; },
                  [&]() {
                    adr::extract(*c.osm_, data_path / "adr", data_path / "adr");
                    d.load_geocoder();

                    if (c.has_feature(feature::REVERSE_GEOCODING)) {
                      d.load_reverse_geocoder();
                    }
                  }};

  auto tt = task{
      "tt", [&]() { return c.has_feature(feature::TIMETABLE); },
      [&]() { return true; },
      [&]() {
        auto const to_clasz_bool_array = [&](config::timetable const& t) {
          auto a = std::array<bool, n::kNumClasses>{};
          a.fill(t.default_bikes_allowed_);
          if (t.clasz_bikes_allowed_.has_value()) {
            for (auto const [clasz, allowed] : *t.clasz_bikes_allowed_) {
              a[static_cast<unsigned>(n::to_clasz(clasz))] = allowed;
            }
          }
          return a;
        };

        auto ec = std::error_code{};
        fs::create_directories(data_path / "n" / "shapes", ec);

        auto const first_day = n::parse_date(c.first_day_);
        auto const interval =
            n::interval{first_day, first_day + std::chrono::days{c.num_days_}};

        auto assistance = std::unique_ptr<nl::assistance_times>{};
        if (c.assistance_times_.has_value()) {
          auto const f =
              cista::mmap{c.assistance_times_->generic_string().c_str(),
                          cista::mmap::protection::READ};
          assistance = std::make_unique<nl::assistance_times>(
              nl::read_assistance(f.view()));
        }

        auto shapes = std::unique_ptr<n::shapes_storage_t>();
        if (c.with_shapes_) {
          shapes = std::make_unique<n::shapes_storage_t>(
              n::create_shapes_storage(data_path / "n" / "shapes"));
        }

        d.tt_ = cista::wrapped{cista::raw::make_unique<n::timetable>(nl::load(
            utl::to_vec(
                *c.timetables_,
                [&](auto&& x) -> std::pair<fs::path, nl::loader_config> {
                  auto const& [_, t] = x;
                  return {t.path_,
                          {
                              .link_stop_distance_ = c.link_stop_distance_,
                              .default_tz_ = t.default_timezone_.value_or(
                                  c.default_timezone_.value_or("")),
                              .bikes_allowed_default_ = to_clasz_bool_array(t),
                          }};
                }),
            {.adjust_footpaths_ = c.adjust_footpaths_,
             .merge_dupes_intra_src_ = c.merge_dupes_intra_src_,
             .merge_dupes_inter_src_ = c.merge_dupes_inter_src_,
             .max_footpath_length_ = c.max_footpath_length_},
            interval, assistance.get(), shapes.get(), c.ignore_errors_))};
        d.location_rtee_ =
            std::make_unique<point_rtree<nigiri::location_idx_t>>(
                create_location_rtree(*d.tt_));
        d.tt_->write(data_path / "tt.bin");
      }};

  auto adr_extend = task{
      "adr_extend",
      [&]() {
        return c.has_feature(feature::GEOCODING) &&
               c.has_feature(feature::TIMETABLE);
      },
      [&]() {
        return d.tt_ != nullptr && d.t_ != nullptr && d.area_db_ != nullptr;
      },
      [&]() {
        adr_extend_tt(*d.tt_, *d.area_db_, *d.t_);
        auto mmap = cista::buf{
            cista::mmap{(data_path / "adr" / "t.bin").generic_string().c_str(),
                        cista::mmap::protection::WRITE}};
        cista::serialize<cista::mode::WITH_STATIC_VERSION>(mmap, *d.t_);
      }};

  auto osr_footpaths = task{
      "osr_footpaths", [&]() { return c.has_feature(feature::OSR_FOOTPATHS); },
      [&]() {
        return d.tt_ != nullptr && d.w_ != nullptr && d.l_ != nullptr &&
               d.pl_ != nullptr;
      },
      [&]() {
        auto const elevator_footpath_map =
            compute_footpaths(*d.tt_, *d.w_, *d.l_, *d.pl_, true);
        write(data_path / "elevator_footpath_map.bin", elevator_footpath_map);
        d.tt_->write(data_path / "tt.bin");
      }};

  auto tasks =
      std::vector<task>{std::move(osr), std::move(adr), std::move(tt),
                        std::move(adr_extend), std::move(osr_footpaths)};

  while (!tasks.empty()) {
    auto const task_it = utl::find_if(
        tasks, [](task const& t) { return t.should_run_() && t.ready_(); });
    utl::verify(task_it != end(tasks), "no task to run, remaining tasks: {}",
                tasks);
    task_it->run_();
    tasks.erase(task_it);
  }
}

}  // namespace motis