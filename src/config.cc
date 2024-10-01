#include "motis/config.h"

#include "rfl.hpp"
#include "rfl/yaml.hpp"

namespace motis {

config config::read(std::filesystem::path const&) { return {}; }

config config::read(std::string const& s) {
  return rfl::yaml::read<config>(s).value();
}

}  // namespace motis