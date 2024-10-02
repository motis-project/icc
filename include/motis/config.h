#pragma once

#include <filesystem>
#include <iosfwd>
#include <map>
#include <optional>

#include "boost/url/url.hpp"

#include "nigiri/clasz.h"

#include "cista/reflection/comparable.h"

namespace motis {

using headers_t = std::map<std::string, std::string>;

struct config {
  struct timetable {
    CISTA_FRIEND_COMPARABLE(timetable)

    struct rt {
      CISTA_FRIEND_COMPARABLE(rt)

      std::string url_;
      std::optional<headers_t> headers_{};
    };

    std::filesystem::path path_;
    bool default_bikes_allowed_{false};
    std::optional<std::map<std::string, bool>> clasz_bikes_allowed_{};
    std::optional<std::vector<rt>> rt_{};
    std::optional<std::string> default_timezone_{};
  };

  CISTA_FRIEND_COMPARABLE(config)

  friend std::ostream& operator<<(std::ostream&, config const&);
  static config read(std::filesystem::path const&);
  static config read(std::string const&);

  std::string first_day_{"TODAY"};
  std::uint16_t num_days_{365U};
  bool merge_dupes_intra_src_{false};
  bool merge_dupes_inter_src_{false};
  unsigned max_footpath_length_{15};
  std::optional<std::string> default_timezone_{};
  std::map<std::string, timetable> timetables_;
  std::optional<std::filesystem::path> osm_;
  std::optional<std::filesystem::path> assistance_times_;
  std::optional<std::vector<std::string>> enabled_features_{};
  std::optional<std::vector<std::string>> disabled_features_{};
};

}  // namespace motis