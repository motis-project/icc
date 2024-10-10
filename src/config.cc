#include "motis/config.h"

#include <iostream>
#include <regex>

#include "boost/program_options.hpp"

#include "fmt/std.h"

#include "utl/parser/split.h"
#include "utl/read_file.h"
#include "utl/verify.h"

#include "nigiri/clasz.h"

#include "rfl.hpp"
#include "rfl/yaml.hpp"

namespace fs = std::filesystem;

namespace motis {

template <rfl::internal::StringLiteral Name, size_t I = 0, char... Chars>
consteval auto drop_last() {
  if constexpr (I == Name.arr_.size() - 2) {
    return rfl::internal::StringLiteral<sizeof...(Chars) + 1>(Chars...);
  } else {
    return drop_last<Name, I + 1, Chars..., Name.arr_[I]>();
  }
}

struct drop_trailing {
public:
  template <typename StructType>
  static auto process(auto&& named_tuple) {
    const auto handle_one = []<typename FieldType>(FieldType&& f) {
      if constexpr (FieldType::name() != "xml_content" &&
                    !rfl::internal::is_rename_v<typename FieldType::Type>) {
        return handle_one_field(std::move(f));
      } else {
        return std::move(f);
      }
    };
    return named_tuple.transform(handle_one);
  }

private:
  template <typename FieldType>
  static auto handle_one_field(FieldType&& _f) {
    using NewFieldType =
        rfl::Field<drop_last<FieldType::name_>(), typename FieldType::Type>;
    return NewFieldType(_f.value());
  }
};

std::ostream& operator<<(std::ostream& out, config const& c) {
  return out << rfl::yaml::write<drop_trailing>(c);
}

config config::read(std::filesystem::path const& p) {
  auto const file_content = utl::read_file(p.generic_string().c_str());
  utl::verify(file_content.has_value(), "could not read config file at {}", p);
  return read(*file_content);
}

config config::read(std::string const& s) {
  auto c =
      rfl::yaml::read<config, drop_trailing, rfl::DefaultIfMissing>(s).value();
  c.verify();
  return c;
}

void config::verify() const {
  utl::verify(!geocoding_ || osm_,
              "feature GEOCODING requires OpenStreetMap data");
  utl::verify(!reverse_geocoding_ || (geocoding_ && osm_),
              "feature REVERSE_GEOCODING requires OpenStreetMap data and "
              "feature GEOCODING");
  utl::verify(!tiles_ || osm_, "feature TILES requires OpenStreetMap data");
  utl::verify(!street_routing_ || osm_,
              "feature STREET_ROUTING requires OpenStreetMap data");
  utl::verify(!timetable_ || !timetable_->datasets_.empty(),
              "feature TIMETABLE requires timetable data");
  utl::verify(
      !osr_footpath_ || (street_routing_ && timetable_),
      "feature OSR_FOOTPATH requires features STREET_ROUTING and TIMETABLE");
  utl::verify(
      !elevators_ || (fasta_ && street_routing_ && timetable_),
      "feature ELEVATORS requires fasta.json and features STREET_ROUTING and "
      "TIMETABLE");
}

void config::verify_input_files_exist() const {
  utl::verify(!osm_ || fs::is_regular_file(*osm_),
              "OpenStreetMap file does not exist: {}",
              osm_.value_or(fs::path{}));

  utl::verify(!tiles_ || fs::is_regular_file(tiles_->profile_),
              "tiles profile {} does not exist",
              tiles_.value_or(tiles{}).profile_);

  utl::verify(!tiles_ || !tiles_->coastline_ ||
                  fs::is_regular_file(*tiles_->coastline_),
              "coastline file {} does not exist",
              tiles_.value_or(tiles{}).coastline_.value_or(""));

  if (timetable_) {
    for (auto const& [_, d] : timetable_->datasets_) {
      utl::verify(d.path_.starts_with("\n#") || fs::is_directory(d.path_) ||
                      fs::is_regular_file(d.path_),
                  "timetable dataset does not exist: {}", d.path_);

      if (d.clasz_bikes_allowed_) {
        for (auto const& c : *d.clasz_bikes_allowed_) {
          nigiri::to_clasz(c.first);
        }
      }
    }
  }
}

bool config::requires_rt_timetable_updates() const {
  return timetable_.has_value() &&
         utl::any_of(timetable_->datasets_, [](auto&& d) {
           return d.second.rt_.has_value() && !d.second.rt_->empty();
         });
}

config config::read_legacy(fs::path const& p) {
  namespace po = boost::program_options;

  struct options {
    // import
    std::vector<std::string> import_paths_;
    std::string data_directory_{"data"};
    bool require_successful_{true};

    // module
    std::vector<std::string> modules_;
    std::vector<std::string> exclude_modules_;

    // nigiri
    bool no_cache_{false};
    bool adjust_footpaths_{true};
    bool merge_dupes_intra_src_{false};
    bool merge_dupes_inter_src_{false};
    std::uint16_t max_footpath_length_{
        std::numeric_limits<std::uint16_t>::max()};
    std::string first_day_{"TODAY"};
    std::string default_timezone_;
    std::uint16_t num_days_{2U};
    bool lookup_{true};
    bool guesser_{true};
    bool railviz_{true};
    bool routing_{true};
    unsigned link_stop_distance_{100U};
    std::vector<std::string> gtfsrt_urls_;
    std::vector<std::string> gtfsrt_paths_;
    unsigned gtfsrt_update_interval_sec_{60U};
    bool gtfsrt_incremental_{false};
    bool debug_{false};
    bool bikes_allowed_default_{false};
  } cfg;

  auto prefix = std::string{};
  auto desc = po::options_description{"Global options"};
  auto const param = [&](auto& ref, std::string const& name,
                         std::string const& doc) {
    auto val = po::value<std::decay_t<decltype(ref)>>(&ref)->default_value(ref);
    desc.add_options()  //
        ((prefix + "." + name).c_str(), std::move(val), doc.c_str());
  };

  prefix = "";
  param(cfg.modules_, "modules", "List of modules to load");
  param(cfg.exclude_modules_, "exclude_modules", "List of modules to exclude");

  prefix = "import";
  param(cfg.import_paths_, "paths",
        "input paths to process. expected format: tag-options:path "
        "(brackets are optional if options empty)");
  param(cfg.data_directory_, "data_dir", "directory for preprocessing output");
  param(cfg.require_successful_, "require_successful",
        "exit if import is not successful for all modules");

  prefix = "nigiri";
  param(cfg.adjust_footpaths_, "adjust_footpaths",
        "adjust footpaths if they are too fast for the distance");
  param(cfg.merge_dupes_inter_src_, "match_duplicates",
        "match and merge duplicate trips from different timetable sources");
  param(cfg.merge_dupes_intra_src_, "merge_dupes_intra_src",
        "match and merge duplicate trips with a single timetable source");
  param(cfg.merge_dupes_inter_src_, "merge_dupes_inter_src",
        "match and merge duplicate trips from different timetable sources");
  param(cfg.max_footpath_length_, "max_footpath_length",
        "maximum footpath length in minutes");
  param(cfg.first_day_, "first_day",
        "YYYY-MM-DD, leave empty to use first day in source data");
  param(cfg.num_days_, "num_days",
        "number of days, ignored if first_day is empty");
  param(cfg.link_stop_distance_, "link_stop_distance",
        "GTFS only: radius to connect stations, 0=skip");
  param(cfg.default_timezone_, "default_timezone",
        "tz for agencies w/o tz or routes w/o agency");
  param(cfg.gtfsrt_urls_, "gtfsrt",
        "list of GTFS-RT endpoints, format: tag|url|authorization");
  param(cfg.gtfsrt_paths_, "gtfsrt_paths",
        "list of GTFS-RT, format: tag|/path/to/file.pb");
  param(cfg.gtfsrt_incremental_, "gtfsrt_incremental",
        "true=incremental updates, false=forget all prev. RT updates");
  param(cfg.bikes_allowed_default_, "bikes_allowed_default",
        "whether bikes are allowed in trips where no information is "
        "available");

  auto ifs = std::ifstream{p};
  if (!ifs) {
    throw utl::fail("could not open file {}", p);
  }

  auto parsed_file_options = po::parse_config_file(ifs, desc, true);
  auto unrecog_file = po::collect_unrecognized(parsed_file_options.options,
                                               po::include_positional);
  auto unrecog = std::vector<std::string>{};
  unrecog.insert(unrecog.end(), unrecog_file.begin(), unrecog_file.end());
  for (auto const& u : unrecog) {
    std::cout << "unrecognized option: " << u << "\n";
  }

  auto vm = po::variables_map{};
  po::store(parsed_file_options, vm);
  po::notify(vm);

  auto const re = std::regex{R"(^(\w+)(?:\-(.*?))?:(.*)$)"};
  auto const split_import_path = [&](std::string const& import_path)
      -> std::tuple<std::string, std::string, std::string> {
    {
      std::smatch m;
      utl::verify(std::regex_match(import_path, m, re) && m.size() == 4,
                  "import_path does not match tag-options:path : {}",
                  import_path);
      utl::verify(fs::exists(m.str(3)), "file does not exist: {}", m.str(3));
      return {m.str(1), m.str(2), m.str(3)};
    }
  };

  auto const is_module_active = [&](std::string const& module) {
    auto const& yes = cfg.modules_;
    auto const& no = cfg.exclude_modules_;
    return std::find(begin(yes), end(yes), module) != end(yes) &&
           std::find(begin(no), end(no), module) == end(no);
  };

  auto c = config{};

  c.timetable_ = is_module_active("nigiri")
                     ? std::optional{timetable{
                           .first_day_ = cfg.first_day_,
                           .num_days_ = cfg.num_days_,
                           .with_shapes_ = true,
                           .ignore_errors_ = true,
                           .adjust_footpaths_ = cfg.adjust_footpaths_,
                           .merge_dupes_intra_src_ = cfg.merge_dupes_intra_src_,
                           .merge_dupes_inter_src_ = cfg.merge_dupes_inter_src_,
                           .link_stop_distance_ = cfg.link_stop_distance_,
                           .update_interval_ = cfg.gtfsrt_update_interval_sec_,
                           .incremental_rt_update_ = cfg.gtfsrt_incremental_,
                           .max_footpath_length_ = cfg.max_footpath_length_,
                           .default_timezone_ = cfg.default_timezone_}}
                     : std::nullopt;
  c.street_routing_ = is_module_active("osr") || is_module_active("osrm") ||
                      is_module_active("ppr");
  c.geocoding_ = is_module_active("adr");
  c.tiles_ = is_module_active("tiles") ? std::optional{tiles{}} : std::nullopt;

  for (auto const& x : cfg.import_paths_) {
    auto const [type, tag, path] = split_import_path(x);
    if (type == "schedule" && c.timetable_.has_value()) {
      c.timetable_->datasets_[tag].path_ = path;
    } else if (type == "osm") {
      c.osm_ = path;
    } else if (type == "coastline" && c.tiles_.has_value()) {
      c.tiles_ = config::tiles{};
      c.tiles_->coastline_ = path;
    }
  }

  if (c.timetable_.has_value()) {
    for (auto const& rt : cfg.gtfsrt_paths_) {
      auto const [tag, url, auth] =
          utl::split<'|', utl::cstr, utl::cstr, utl::cstr>(rt);
      auto const it = c.timetable_->datasets_.find(tag.to_str());
      utl::verify(it != end(c.timetable_->datasets_),
                  "rt: tag not found for {}", rt);
      auto& rts = it->second.rt_;
      if (!rts.has_value()) {
        rts = std::vector<timetable::dataset::rt>{};
      }
      auto& entry =
          rts->emplace_back(timetable::dataset::rt{.url_ = url.to_str()});
      if (!auth.empty()) {
        entry.headers_->emplace("Authorization", auth.to_str());
      }
    }
  }

  return c;
}

}  // namespace motis