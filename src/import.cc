#include "motis/import.h"

#include <fstream>
#include <map>
#include <ostream>
#include <vector>

#include "boost/json.hpp"

#include "fmt/ranges.h"

#include "utl/erase_if.h"
#include "utl/to_vec.h"

#include "nigiri/loader/assistance.h"
#include "nigiri/loader/load.h"
#include "nigiri/loader/loader_interface.h"
#include "nigiri/clasz.h"
#include "nigiri/common/parse_date.h"
#include "nigiri/shape.h"
#include "nigiri/timetable.h"

#include "osr/extract/extract.h"
#include "osr/lookup.h"
#include "osr/platforms.h"
#include "osr/ways.h"

#include "adr/adr.h"
#include "adr/typeahead.h"

#include "motis/adr_extend_tt.h"
#include "motis/clog_redirect.h"
#include "motis/compute_footpaths.h"
#include "motis/data.h"
#include "motis/tt_location_rtree.h"

namespace fs = std::filesystem;
namespace n = nigiri;
namespace nl = nigiri::loader;
using namespace std::string_literals;

namespace motis {

using meta_entry_t = std::pair<std::string, std::uint64_t>;
using meta_t = std::map<std::string, std::uint64_t>;

meta_t read_hashes(fs::path const& data_path, std::string const& name) {
  auto const p = (data_path / "meta" / (name + ".json"));
  if (!exists(p)) {
    return {};
  }
  auto const mmap =
      cista::mmap{p.generic_string().c_str(), cista::mmap::protection::READ};
  return boost::json::value_to<meta_t>(boost::json::parse(mmap.view()));
}

void write_hashes(fs::path const& data_path,
                  std::string const& name,
                  meta_t const& h) {
  auto const p = (data_path / "meta" / (name + ".json"));
  std::ofstream{p} << boost::json::serialize(boost::json::value_from(h));
}

struct task {
  friend std::ostream& operator<<(std::ostream& out, task const& t) {
    return out << t.name_;
  }

  void run(fs::path const& data_path) {
    auto const redirect = clog_redirect{
        (data_path / "logs" / (name_ + ".txt")).generic_string().c_str()};
    auto const pt = utl::activate_progress_tracker(name_);
    auto const existing = read_hashes(data_path, name_);
    if (existing == hashes_) {
      load_();
    } else {
      run_();
      write_hashes(data_path, name_, hashes_);
    }
    pt->out_ = 100;
    pt->status("FINISHED");
  }

  std::string name_;
  std::function<bool()> should_run_;
  std::function<bool()> ready_;
  std::function<void()> run_;
  std::function<void()> load_;
  meta_t hashes_;
  utl::progress_tracker_ptr pt_{utl::activate_progress_tracker(name_)};
};

}  // namespace motis

template <>
struct fmt::formatter<motis::task> : fmt::ostream_formatter {};

namespace motis {

cista::hash_t hash_file(fs::path const& p) {
  auto const mmap =
      cista::mmap{p.generic_string().c_str(), cista::mmap::protection::READ};
  return cista::hash(mmap.view());
}

void import(config const& c, fs::path const& data_path) {
  c.verify();

  auto ec = std::error_code{};
  fs::create_directories(data_path / "logs", ec);
  fs::create_directories(data_path / "meta", ec);

  auto const bars = utl::global_progress_bars{false};

  auto tt_hash = std::pair{"timetable"s, cista::BASE_HASH};
  if (c.timetable_.has_value()) {
    for (auto const& [_, d] : c.timetable_->datasets_) {
      tt_hash.second = cista::hash_combine(tt_hash.second, hash_file(d.path_));
    }
    if (c.timetable_->assistance_times_.has_value()) {
      tt_hash.second = cista::hash_combine(
          tt_hash.second, hash_file(*c.timetable_->assistance_times_));
    }
  }

  auto osm_hash = std::pair{"osm"s, cista::BASE_HASH};
  if (c.osm_.has_value()) {
    osm_hash.second = hash_file(*c.osm_);
  }

  auto const osr_version = meta_entry_t{"osr_bin_ver", osr::kBinaryVersion};
  auto const adr_version = meta_entry_t{"adr_bin_ver", adr::kBinaryVersion};
  auto const n_version = meta_entry_t{"nigiri_bin_ver", n::kBinaryVersion};

  auto d = data{data_path};

  auto osr = task{"osr",
                  [&]() { return c.has_feature(feature::STREET_ROUTING); },
                  [&]() { return true; },
                  [&]() {
                    osr::extract(true, fs::path{*c.osm_}, data_path / "osr");
                    d.load_osr();
                  },
                  [&]() { d.load_osr(); },
                  {osm_hash, {"version"s, osr::kBinaryVersion}}};

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
                  },
                  [&]() {
                    d.load_geocoder();
                    if (c.has_feature(feature::REVERSE_GEOCODING)) {
                      d.load_reverse_geocoder();
                    }
                  },
                  {osm_hash, adr_version}};

  auto tt = task{
      "tt",
      [&]() { return c.has_feature(feature::TIMETABLE); },
      [&]() { return true; },
      [&]() {
        auto const to_clasz_bool_array =
            [&](config::timetable::dataset const& d) {
              auto a = std::array<bool, n::kNumClasses>{};
              a.fill(d.default_bikes_allowed_);
              if (d.clasz_bikes_allowed_.has_value()) {
                for (auto const [clasz, allowed] : *d.clasz_bikes_allowed_) {
                  a[static_cast<unsigned>(n::to_clasz(clasz))] = allowed;
                }
              }
              return a;
            };

        auto ec = std::error_code{};

        auto const& t = *c.timetable_;

        auto const first_day = n::parse_date(t.first_day_);
        auto const interval = n::interval<date::sys_days>{
            first_day, first_day + std::chrono::days{t.num_days_}};

        auto assistance = std::unique_ptr<nl::assistance_times>{};
        if (t.assistance_times_.has_value()) {
          auto const f =
              cista::mmap{t.assistance_times_->generic_string().c_str(),
                          cista::mmap::protection::READ};
          assistance = std::make_unique<nl::assistance_times>(
              nl::read_assistance(f.view()));
        }

        auto shapes = std::unique_ptr<n::shapes_storage_t>();
        if (t.with_shapes_) {
          shapes = std::make_unique<n::shapes_storage_t>(
              n::create_shapes_storage(data_path / "shapes"));
        }

        d.tt_ = cista::wrapped{cista::raw::make_unique<n::timetable>(nl::load(
            utl::to_vec(
                t.datasets_,
                [&](auto&& x) -> std::pair<fs::path, nl::loader_config> {
                  auto const& [_, dc] = x;
                  return {dc.path_,
                          {
                              .link_stop_distance_ = t.link_stop_distance_,
                              .default_tz_ = dc.default_timezone_.value_or(
                                  dc.default_timezone_.value_or("")),
                              .bikes_allowed_default_ = to_clasz_bool_array(dc),
                          }};
                }),
            {.adjust_footpaths_ = t.adjust_footpaths_,
             .merge_dupes_intra_src_ = t.merge_dupes_intra_src_,
             .merge_dupes_inter_src_ = t.merge_dupes_inter_src_,
             .max_footpath_length_ = t.max_footpath_length_},
            interval, assistance.get(), shapes.get(), t.ignore_errors_))};
        d.location_rtee_ =
            std::make_unique<point_rtree<nigiri::location_idx_t>>(
                create_location_rtree(*d.tt_));
        d.tt_->write(data_path / "tt.bin");
      },
      [&]() { d.load_tt(); },
      {tt_hash, n_version}};

  auto adr_extend =
      task{"adr_extend",
           [&]() {
             return c.has_feature(feature::GEOCODING) &&
                    c.has_feature(feature::TIMETABLE);
           },
           [&]() { return d.tt_ && d.t_ && d.area_db_; },
           [&]() {
             adr_extend_tt(*d.tt_, *d.area_db_, *d.t_);
             auto mmap = cista::buf{cista::mmap{
                 (data_path / "adr" / "t.bin").generic_string().c_str(),
                 cista::mmap::protection::WRITE}};
             cista::serialize<cista::mode::WITH_STATIC_VERSION>(mmap, *d.t_);
           },
           [&]() {},
           {tt_hash, osm_hash, adr_version, n_version}};

  auto osr_footpath = task{
      "osr_footpath",
      [&]() { return c.has_feature(feature::OSR_FOOTPATH); },
      [&]() { return d.tt_ && d.w_ && d.l_ && d.pl_; },
      [&]() {
        auto const elevator_footpath_map =
            compute_footpaths(*d.w_, *d.l_, *d.pl_, *d.tt_, true);
        write(data_path / "elevator_footpath_map.bin", elevator_footpath_map);
        d.tt_->write(data_path / "tt.bin");
      },
      [&]() {},
      {tt_hash, osm_hash, osr_version, n_version}};

  auto tasks = std::vector<task>{osr, adr, tt, adr_extend, osr_footpath};
  utl::erase_if(tasks, [&](auto&& t) { return !t.should_run_(); });

  while (!tasks.empty()) {
    auto const task_it =
        utl::find_if(tasks, [](task const& t) { return t.ready_(); });
    utl::verify(task_it != end(tasks), "no task to run, remaining tasks: {}",
                tasks);
    task_it->run(data_path);
    tasks.erase(task_it);
  }

  std::ofstream{(data_path / "config.ini").generic_string()} << c << "\n";
}

}  // namespace motis