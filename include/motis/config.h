#pragma once

#include <filesystem>
#include <map>
#include <optional>

#include "boost/url/url.hpp"

namespace motis {

using headers_t = std::map<std::string, std::string>;

struct gtfsrt {
  std::string url_;
  headers_t headers_;
};

struct timetable {
  std::filesystem::path path_;
  std::vector<gtfsrt> gtfsrt_;
};

struct config {
  static config read(std::filesystem::path const&);
  static config read(std::string const&);

  std::optional<std::map<std::string, timetable>> timetables_;
  std::optional<std::filesystem::path> osm_;
  std::optional<std::filesystem::path> assistance_times_;
};

}  // namespace motis