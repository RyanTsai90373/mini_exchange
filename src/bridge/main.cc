#include <chrono>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>
#include <zmq.hpp>

using json = nlohmann::json;

namespace {

constexpr const char* kTickEndpoint  = "tcp://*:5555";
constexpr const char* kOrderEndpoint = "tcp://localhost:5556";

std::int64_t NowNs() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

}  // namespace

int main() {
  std::cout.setf(std::ios::unitbuf);

  zmq::context_t ctx{1};

  zmq::socket_t tick_in{ctx, zmq::socket_type::pull};
  tick_in.bind(kTickEndpoint);

  zmq::socket_t order_out{ctx, zmq::socket_type::push};
  order_out.connect(kOrderEndpoint);

  std::cout << "[cpp] PULL bind " << kTickEndpoint
            << ", PUSH connect " << kOrderEndpoint << '\n';

  std::uint64_t order_seq = 0;

  while (true) {
    zmq::message_t msg;
    auto recv = tick_in.recv(msg, zmq::recv_flags::none);
    if (!recv) continue;

    json tick;
    try {
      tick = json::parse(msg.to_string());
    } catch (const std::exception& e) {
      std::cerr << "[cpp] json parse error: " << e.what() << '\n';
      continue;
    }

    std::cout << "[cpp] tick recv: " << tick.dump() << '\n';

    json order = {
        {"msg_type", "order_request"},
        {"seq_num", ++order_seq},
        {"ts_ns", NowNs()},
        {"symbol", tick.value("symbol", "UNKNOWN")},
        {"side", "buy"},
        {"qty", 1},
        {"price", tick.value("price", 0.0)},
    };
    const std::string payload = order.dump();
    order_out.send(zmq::buffer(payload), zmq::send_flags::none);
    std::cout << "[cpp] order sent: " << payload << '\n';
  }
}
