#include "motis/endpoints/adr.h"

#include <ranges>

#include "osr/geojson.h"

#include "boost/json.hpp"
#include "boost/thread/tss.hpp"

#include "fmt/chrono.h"
#include "fmt/format.h"

#include "adr/adr.h"
#include "adr/typeahead.h"

#include "motis/elevators/match_elevator.h"

namespace a = adr;

namespace motis::ep {

a::guess_context& get_guess_context(a::typeahead const& t, a::cache& cache) {
  auto static ctx = boost::thread_specific_ptr<a::guess_context>{};
  if (ctx.get() == nullptr) {
    ctx.reset(new a::guess_context{cache});
  }
  ctx->resize(t);
  return *ctx;
}

std::int16_t get_area_lang_idx(a::typeahead const& t,
                               a::language_list_t const& languages,
                               a::area_set_lang_t const matched_area_lang,
                               a::area_idx_t const a) {
  for (auto i = 0U; i != languages.size(); ++i) {
    auto const j = languages.size() - i - 1U;
    auto const lang_idx = a::find_lang(t.area_name_lang_[a], languages[j]);
    if (lang_idx == -1) {
      return lang_idx;
    }
  }
  return -1;
}

api::geocode_response adr::operator()(boost::urls::url_view const& url) const {
  auto const params = api::geocode_params{url.params()};

  auto& ctx = get_guess_context(t_, cache_);
  auto lang_indices = std::basic_string<a::language_idx_t>{{a::kDefaultLang}};

  a::get_suggestions<false>(t_, geo::latlng{0, 0}, params.text_, 10U,
                            lang_indices, ctx);

  return utl::to_vec(ctx.suggestions_, [&](a::suggestion const& s) {
    auto const areas = t_.area_sets_[s.area_set_];
    auto const city_it =
        std::min_element(begin(areas), end(areas), [&](auto&& a, auto&& b) {
          return std::abs(to_idx(t_.area_admin_level_[a]) - 7) <
                 std::abs(to_idx(t_.area_admin_level_[b]) - 7);
        });
    auto const city_idx =
        city_it == end(areas) ? -1 : std::distance(begin(areas), city_it);

    auto type = api::typeEnum{};
    auto street = std::optional<std::string>{};
    auto house_number = std::optional<std::string>{};
    auto const name = std::visit(
        utl::overloaded{
            [&](a::place_idx_t const p) {
              type = api::typeEnum::PLACE;
              return std::string{t_.strings_[s.str_].view()};
            },
            [&](a::address const addr) {
              type = api::typeEnum::ADDRESS;
              if (addr.house_number_ != a::address::kNoHouseNumber) {
                street = t_.strings_[s.str_].view();
                house_number =
                    t_.strings_[t_.house_numbers_[addr.street_]
                                                 [addr.house_number_]]
                        .view();
                return fmt::format("{} {}", *street, *house_number);
              } else {
                return std::string{t_.strings_[s.str_].view()};
              }
            }},
        s.location_);

    return api::Match{
        .type_ = type,
        .tokens_ = {},  // TODO
        .name_ = std::move(name),
        .id_ = "",  // TODO
        .lat_ = s.coordinates_.as_latlng().lat_,
        .lon_ = s.coordinates_.as_latlng().lng_,
        .street_ = std::move(street),
        .house_number_ = std::move(house_number),
        .zip_ = {},  // TODO
        .areas_ = utl::to_vec(utl::enumerate(areas), [&](auto&& el) {
          auto const [i, a] = el;
          auto const admin_lvl = t_.area_admin_level_[a];
          auto const is_matched = (((1U << i) & s.matched_areas_) != 0U);
          auto const language =
              is_matched ? s.matched_area_lang_[i]
                         : get_area_lang_idx(t_, lang_indices,
                                             s.matched_area_lang_, a);
          auto const name =
              t_.strings_[t_.area_names_[a][language == -1 ? a::kDefaultLangIdx
                                                           : language]]
                  .view();
          return api::Area{
              .name_ = std::string{name},
              .admin_level_ = static_cast<double>(to_idx(admin_lvl)),
              .matched_ = is_matched};
        })};
  });
}

}  // namespace motis::ep