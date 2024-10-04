#include "fmt/core.h"

#include "boost/asio/io_context.hpp"

#include "net/web_server/query_router.h"
#include "net/web_server/web_server.h"

#include "utl/cmd_line_parser.h"
#include "utl/init_from.h"

#include "net/run.h"
#include "net/stop_handler.h"

#include "motis/config.h"
#include "motis/data.h"
#include "motis/elevators/elevators.h"
#include "motis/endpoints/adr/geocode.h"
#include "motis/endpoints/adr/reverse_geocode.h"
#include "motis/endpoints/elevators.h"
#include "motis/endpoints/footpaths.h"
#include "motis/endpoints/graph.h"
#include "motis/endpoints/levels.h"
#include "motis/endpoints/matches.h"
#include "motis/endpoints/osr_routing.h"
#include "motis/endpoints/platforms.h"
#include "motis/endpoints/routing.h"
#include "motis/endpoints/update_elevator.h"

namespace asio = boost::asio;
namespace fs = std::filesystem;
using namespace std::string_view_literals;
using namespace motis;

template <typename T, typename From>
void GET(auto&& r, std::string target, From& from) {
  if (auto const x = utl::init_from<T>(from); x.has_value()) {
    r.get(std::move(target), std::move(*x));
  }
}

template <typename T, typename From>
void POST(auto&& r, std::string target, From& from) {
  if (auto const x = utl::init_from<T>(from); x.has_value()) {
    r.post(std::move(target), std::move(*x));
  }
}

int server(fs::path data_path, config const& c) {
  auto d = data{std::move(data_path), c};
  auto ioc = asio::io_context{};
  auto workers = asio::io_context{};
  auto s = net::web_server{ioc};
  auto qr = net::query_router{net::asio_exec({ioc, workers})};

  POST<ep::matches>(qr, "/api/matches", d);
  POST<ep::elevators>(qr, "/api/elevators", d);
  POST<ep::osr_routing>(qr, "/api/route", d);
  POST<ep::platforms>(qr, "/api/platforms", d);
  POST<ep::graph>(qr, "/api/graph", d);
  POST<ep::update_elevator>(qr, "/api/update_elevator", d);
  GET<ep::footpaths>(qr, "/api/debug/footpaths", d);
  GET<ep::levels>(qr, "/api/v1/levels", d);
  GET<ep::reverse_geocode>(qr, "/api/v1/reverse-geocode", d);
  GET<ep::geocode>(qr, "/api/v1/geocode", d);
  GET<ep::routing>(qr, "/api/v1/plan", d);

  qr.serve_files("ui/build");
  qr.enable_cors();
  s.on_http_request(std::move(qr));

  auto ec = boost::system::error_code{};
  s.init(c.host_, c.port_, ec);
  s.run();
  if (ec) {
    std::cerr << "error: " << ec << "\n";
    return 1;
  }

  auto work_guard = asio::make_work_guard(workers);
  auto threads = std::vector<std::thread>(
      static_cast<unsigned>(std::max(1U, std::thread::hardware_concurrency())));
  for (auto& t : threads) {
    t = std::thread(net::run(workers));
  }

  auto const stop = net::stop_handler(ioc, [&]() { s.stop(); });

  fmt::println("listening on {}:{}", c.host_, c.port_);
  net::run(ioc)();

  workers.stop();
  for (auto& t : threads) {
    t.join();
  }

  return 0;
}

int main(int ac, char const** av) {
  constexpr auto const kDefaultMode = "server";
  auto const subcommand =
      ac <= 1U ? std::string_view{kDefaultMode} : std::string_view{av[1]};

  if (subcommand == "server") {
    struct flags {
      utl::cmd_line_flag<fs::path,
                         UTL_SHORT("-d"),
                         UTL_LONG("--data_path"),
                         UTL_DESC("data path")>
          data_path_{"data"};
      utl::cmd_line_flag<fs::path,
                         UTL_SHORT("-c"),
                         UTL_LONG("--config"),
                         UTL_DESC("target file")>
          config_path_{"config.ini"};
      utl::cmd_line_flag<std::size_t,
                         UTL_LONG("--num_threads"),
                         UTL_DESC("thread pool size")>
          n_threads_{std::thread::hardware_concurrency()};
    };
    auto const f = utl::parse_flags<flags>(ac - 1, av + 1);
    return server(std::move(*f.data_path_), config::read(*f.config_path_));
  } else {
  }
}