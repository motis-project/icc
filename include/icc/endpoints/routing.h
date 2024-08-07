#pragma once

#include "boost/json/value.hpp"

#include "nigiri/routing/query.h"
#include "nigiri/timetable.h"

#include "osr/lookup.h"
#include "osr/platforms.h"
#include "osr/ways.h"

#include "icc-api/icc-api.h"
#include "icc/compute_footpaths.h"
#include "icc/elevators/elevators.h"
#include "icc/point_rtree.h"
#include "icc/types.h"

namespace icc::ep {

struct routing {
  api::plan_response operator()(boost::urls::url_view const& url) const;

  std::vector<nigiri::routing::offset> get_offsets(
      osr::location const&,
      osr::direction const,
      std::vector<api::ModeEnum> const&,
      bool const wheelchair,
      std::chrono::seconds const max) const;

  nigiri::hash_map<nigiri::location_idx_t,
                   std::vector<nigiri::routing::td_offset>>
  get_td_offsets(elevators const&,
                 osr::location const&,
                 osr::direction,
                 std::vector<api::ModeEnum> const&,
                 bool const wheelchair,
                 std::chrono::seconds const max) const;

  osr::ways const& w_;
  osr::lookup const& l_;
  osr::platforms const& pl_;
  nigiri::timetable const& tt_;
  point_rtree<nigiri::location_idx_t> const& loc_tree_;
  rtt_ptr_t const& rtt_;
  elevators_ptr_t const& e_;
  point_rtree<nigiri::location_idx_t> const& rtree_;
  vector_map<nigiri::location_idx_t, osr::platform_idx_t> const& matches_;
};

}  // namespace icc::ep