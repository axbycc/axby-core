// Purpose of the time client aka, time_sync
// ==========================
//
// The time client synchronizes with a time server (implemented by
// server.cpp in this folder). When synchronized, one should be able
// to call estimate_time_server_timestamps_us() to get the time server's
// microsecond timestamp. We aim for a +/- 1 millsecond accuracy so we
// can do accurate latency measurements over all devices on a local
// wireless network, using a particular device as the time server.

// Message protocol and mechanics
// ==============================
//
// The send thread periodically sends packets to the time
// server. The content of each packet is simply the local process
// time in microseconds at which the packet is sent, denoted
// send_time_us (current time in microseconds).
//
// When the server receives the packet containing { send_time_us
// }, it notes the server time server_time_us, and responds to the
// pair { send_time_us, server_time_us }.
//
// The receive thread periodically checks for such pairs from the
// server.  Upon receipt, it notes the local process time, denoted
// receive_time_us, and creates the triple { send_time_us,
// server_time_us, receive_time_us } and writes it to the ingest
// buffer for processing. Offloading to the ingest buffer allows
// the receive thread to immediately check for another pair and
// therefore associate it to a corresponding receive_time_us as
// soon as possible. This is important for minimizing the inferred
// round trip time (receive_time_us - send_time_us).

// How the offset and round trip time are estimated
// ================================================
//
// The ingest buffer is periodically processed. It contains a list of
// triples of the form { send_time_us, server_time_us, receive_time_us
// } as previously described.
//
// Define the following variables.
//
//   delta_us_1 = transport time between server and client (unknown)
//   delta_us_2 = transport time between client and server (unknown)
//   offset_us = the constant offset between server and client clocks (unknown)
//
// Note the following relationships.
//
//   (a) server_time_us - send_time_us = delta_us_1 + offset_us
//   (b) receive_time_us - server_time_us = delta_us_2 - offset_us
//
// Setting delta_us_1 and delta_us_2 to zero allow us to place an
// upper and lower bound respectively for offset_us, for each
// triple. Merging these upper and lower bounds together gives a
// better estimate of offset_us.
//
// The minimum round trip time is then the width of the merged bound
// around offset_us.

// clang-format off
#include <kissnet.hpp> // for some reason kissnet needs to be first
// clang-format on

#include "time_sync.h"

#include <chrono>
#include <cstdint>
#include <deque>
#include <limits>
#include <mutex>
#include <optional>
#include <thread>
#include <zmq.hpp>

#include "absl/container/flat_hash_map.h"
#include "app/process_id.h"
#include "app/pubsub.h"
#include "app/stop_all.h"
#include "app/timing.h"
#include "concurrency/ring_buffer.h"
#include "debug/log.h"

namespace axby {
namespace time_sync {

struct TimeSyncState {
    int64_t offset_estimate_us;
    uint64_t observed_round_trip_time_us;
    uint64_t min_round_trip_time_us;
};

std::atomic<bool> got_first_sync_message_ = false;
std::atomic<int64_t> offset_estimate_us_ = 0;
std::atomic<uint64_t> min_round_trip_time_us_ = UINT64_MAX;
std::atomic<uint64_t> observed_round_trip_time_us_ = UINT64_MAX;

std::mutex process_id_to_time_sync_state_mutex_;
absl::flat_hash_map<uint64_t, TimeSyncState> process_id_to_time_sync_state_;

uint64_t estimate_time_server_timestamp_us() {
    if (!got_first_sync_message_) return 0;

    // offset_estimate_us_ is int64_t. Conversion to uint64_t is by
    // two's complement if the value is negative, which is guaranteed
    // by the standard. Therefore the following addition, whether
    // offset_estimate_us_ is positive or negative, is correct mod
    // 2^64. We know that the time_server timestamp is positive. So this
    // will give the a sensible answer as offset_estimate_us_ is well
    // estimated.

    return get_process_time_us() + uint64_t(offset_estimate_us_);
}

uint64_t estimate_time_server_timestamp_ms() {
    return estimate_time_server_timestamp_us() / 1000;
}

int64_t estimate_offset_us() {
    if (!got_first_sync_message_) return 0;
    return offset_estimate_us_;
}

uint64_t estimate_time_server_timestamp_us(uint64_t process_id,
                                           uint64_t process_time_us) {
    std::lock_guard<std::mutex> lock{process_id_to_time_sync_state_mutex_};
    if (!process_id_to_time_sync_state_.count(process_id)) return 0;

    // see the comment about conversion int64_t -> uint64_t in the
    // other overload of estimate_time_server_timestamp_us
    const int64_t offset_estimate_us =
        process_id_to_time_sync_state_.at(process_id).offset_estimate_us;
    return process_time_us + uint64_t(offset_estimate_us);
};

uint64_t estimate_time_server_timestamp_ms(uint64_t process_id,
                                           uint64_t process_time_ms) {
    return estimate_time_server_timestamp_us(process_id, process_time_ms) /
           1000;
};

int64_t estimate_offset_ms() { return estimate_offset_us() / 1000; }
kissnet::udp_socket socket_;
std::jthread send_thread_;
std::jthread receive_thread_;
std::jthread publish_thread_;

kissnet::addr_collection to_kissnet_addr_collection(const std::string& ip,
                                                    uint16_t port) {
    // OS socket API
    addrinfo hints = {};
    hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_ADDRCONFIG;

    addrinfo* addrinfo = nullptr;
    if (getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints,
                    &addrinfo) != 0) {
        LOG(FATAL) << "getaddrinfo failed for " << ip << ":" << port;
    }

    // Convert to kissnet API
    kissnet::addr_collection addr;
    std::memcpy(&addr.adrinf, addrinfo->ai_addr, sizeof(sockaddr));
    addr.sock_size = sizeof(sockaddr);

    return addr;
}

using TimingTriplet = std::array<uint64_t, 3>;

struct SlidingWindowSample {
    uint64_t time_ms;
    TimingTriplet timing_triplet;
};

struct TemporalSlidingWindow {
    void add(const std::array<uint64_t, 3>& timing_triplet,
             uint64_t current_time_ms) {
        samples.push_back({
            .time_ms = current_time_ms,
            .timing_triplet = timing_triplet,
        });
    };

    void remove_old_samples(uint64_t current_time_ms) {
        const uint64_t min_time = get_min_time(current_time_ms);
        std::erase_if(samples, [min_time](const auto& sample) {
            return (sample.time_ms < min_time);
        });
    }

    // minimum time allowed by the sliding window, given the current
    // time in milliseconds
    uint64_t get_min_time(uint64_t current_time_ms) {
        const uint64_t duration_ms = 1000 * duration_sec;
        uint64_t min_time_ms = 0;
        if (duration_ms < current_time_ms) {
            min_time_ms = current_time_ms - duration_ms;
        }
        return min_time_ms;
    }

    double duration_sec = 5.0;  // seconds
    std::vector<SlidingWindowSample> samples;
};

using IngestBuffer = RingBuffer<TimingTriplet, 40>;
IngestBuffer ingest_buffer_;

struct IngestContext {
    uint64_t clock_drift_us_per_sec;  // estimated clock drift (ppm)
    TemporalSlidingWindow window;
};

std::optional<TimeSyncState> process_ingest_buffer(
    IngestContext& ctx, IngestBuffer& ingest_buffer) {
    // LOG(INFO) << "PROCESSING INGEST ";

    // add latest samples into sliding windows
    {
        const uint64_t current_time_ms = get_process_time_ms();
        TimingTriplet times;
        while (ingest_buffer.read(&times, /*blocking=*/false)) {
            ctx.window.add(times, current_time_ms);
        }
        ctx.window.remove_old_samples(current_time_ms);
    }

    int64_t offset_us_lb = INT64_MIN;
    int64_t offset_us_ub = INT64_MAX;
    uint64_t observed_round_trip_time_us = UINT64_MAX;
    const uint64_t current_time_us = get_process_time_us();

    for (auto& s : ctx.window.samples) {
        auto [send, serv, recv] = s.timing_triplet;

        // send is the local timestamp when the packet was sent to the server
        // serv is the server timestamp when it received our packet
        // recv is the local timestamp when when got the packet back from the
        // server

        // sanity checks
        CHECK(send <= recv);
        CHECK(recv <= current_time_us)
            << "recv: " << recv << ", current " << current_time_us;

        // if delta1 is the transit time to the server from us and
        // delta2 is the transit time from the server to us and offset
        // is the clock offset between server and us during the round
        // trip, then we can deduce upper and lower bounds on the
        // clock offset implied by this round trip

        // serv - send = delta1 + offset
        // offset upper bound is when delta1 = 0
        const int64_t this_offset_us_ub = serv - send;

        // recv - serv = delta2 - offset
        // serv - recv = offset - delta2
        // offset lower bound is when delta2 = 0
        const int64_t this_offset_us_lb = safe_minus(serv, recv);

        CHECK(this_offset_us_lb <= this_offset_us_ub);

        // Now merge this upper and lower bound into our global
        // estimate of offset_time_us, correcting for the clock drift
        // between us and the server.

        const uint64_t dt_us = current_time_us - (recv + send) / 2;
        const uint64_t estimated_clock_drift_us =
            ctx.clock_drift_us_per_sec * dt_us / 1e6;

        offset_us_lb = std::max<int64_t>(
            offset_us_lb, this_offset_us_lb - estimated_clock_drift_us);
        offset_us_ub = std::min<int64_t>(
            offset_us_ub, this_offset_us_ub + estimated_clock_drift_us);

        observed_round_trip_time_us = std::min<uint64_t>(
            observed_round_trip_time_us, this_offset_us_ub - this_offset_us_lb);
    }

    if (offset_us_ub == INT64_MAX || offset_us_lb == INT64_MIN) {
        return std::nullopt;
    }

    if (offset_us_ub - offset_us_lb <=
        int64_t(observed_round_trip_time_us / 2)) {
        // we are not correcting enough for clock drift
        ctx.clock_drift_us_per_sec *= 2;
        // LOG(INFO) << "Increasing clock drift estimate from "
        //           << prev_clock_drift_us_per_sec << " to "
        //           << clock_drift_us_per_sec;
        return std::nullopt;
    }

    if (offset_us_ub - offset_us_lb >= int64_t(observed_round_trip_time_us)) {
        // we are correcting too much for clock drift
        ctx.clock_drift_us_per_sec *= 0.9;
        // LOG(INFO) << "Decreasing clock drift estimate";
    }

    const uint64_t min_round_trip_time_us = offset_us_ub - offset_us_lb;
    const int64_t offset_estimate_us = (offset_us_ub + offset_us_lb) / 2;

    return TimeSyncState{
        .offset_estimate_us = offset_estimate_us,
        .observed_round_trip_time_us = observed_round_trip_time_us,
        .min_round_trip_time_us = min_round_trip_time_us};
}

std::atomic<bool> received_any_ = false;

void run_receive_thread() {
    while (!should_stop_all()) {
        std::array<uint64_t, 2> recv_buff;

        if (socket_.select(kissnet::fds_read, 1000) !=
            kissnet::socket_status::valid) {
            continue;
        };

        auto [sz, stat] =
            socket_.recv((std::byte*)recv_buff.data(), sizeof(recv_buff));

        if (!stat) {
            LOG(FATAL) << "Receive unsuccessful";
        }
        if (sz == 0) {
            LOG(FATAL) << "Malformed input into receive thread";
        }

        received_any_ = true;

        const uint64_t start_time_us = recv_buff[0];
        const uint64_t server_time_us = recv_buff[1];
        const uint64_t end_time_us = get_process_time_us();
        const TimingTriplet timing_triplet = {start_time_us, server_time_us,
                                              end_time_us};

        if (!ingest_buffer_.write(timing_triplet)) {
            LOG(WARNING) << "ingest buffer full";
        }
    }
}

void run_publish_thread(const double window_duration_sec) {
    IngestContext ctx;
    ctx.window.duration_sec = window_duration_sec;

    // This online source [1] mentions that clocks usually drift
    // by about 20 PPM. That means over one second, an error of
    // +/-20 usec may accumulate between a computer clock and an
    // atomic clock who are initially synchronized. So two
    // initially synchronized computer clocks may expect to become
    // out of sync by double that, i.e. 40 usec, over one second.
    //
    // [1]
    // https://superuser.com/questions/759730/how-much-clock-drift-is-considered-normal-for-a-non-networked-windows-7-pc
    ctx.clock_drift_us_per_sec = 40;

    while (!should_stop_all()) {
        std::optional<TimeSyncState> msg =
            process_ingest_buffer(ctx, ingest_buffer_);
        if (msg) {
            pubsub::publish_simple("time_sync", 0, *msg);
        }
        sleep_ms(100);
    }
}

pubsub::SubscriberBuffer subscriber_buffer_;
void run_subscribe_thread() {
    on_stop_all([]() { subscriber_buffer_.stop(); });
    pubsub::subscribe("time_sync", &subscriber_buffer_);

    while (!should_stop_all()) {
        pubsub::Message message;

        // return if subscriber_buffer_ is shut down
        if (!subscriber_buffer_.move_read(message, /*blocking=*/true)) return;

        CHECK_EQ(message.header.message_version, 0) << "unsupported version";
        TimeSyncState state = message.get_simple<TimeSyncState>(0);

        {
            std::lock_guard<std::mutex> lock{
                process_id_to_time_sync_state_mutex_};
            process_id_to_time_sync_state_[message.header.sender_process_id] =
                state;
        }

        if (message.header.sender_process_id == get_process_id()) {
            offset_estimate_us_ = state.offset_estimate_us;
            min_round_trip_time_us_ = state.min_round_trip_time_us;
            observed_round_trip_time_us_ = state.observed_round_trip_time_us;
            got_first_sync_message_ = true;
        }
    }
}

void run_send_thread(const int blast_size = 20) {
    const auto start_time_ms = get_process_time_ms();
    while (!should_stop_all()) {
        for (int i = 0; i < blast_size; ++i) {
            // Blast out `blast_size` udp packets as fast as possible. On
            // my machine, it turns out that each subsequent packet in
            // a blast round-trips faster than the previous, up to
            // some limit.
            //
            // The round trip times for these packets go something like
            // 161, 81, 76, ..., 59 (microseconds) on my local network.
            //
            // On the other hand, if you wait even a millisecond
            // between between packets, they will all round trip at
            // approximately the speed of the first packet of the
            // aforementioned list (round trip time 161)

            const uint64_t start_time_us = get_process_time_us();
            auto [snd_sz, snd_status] = socket_.send(
                (const std::byte*)&start_time_us, sizeof(start_time_us));

            if (!snd_status) {
                LOG(WARNING) << "Send failed";
                continue;
            }
        }
        // Now back off and wait a bit before doing another send.
        const auto current_time_ms = get_process_time_ms();
        if (current_time_ms - start_time_ms > 5000) {
            sleep_ms(500);
        } else {
            sleep_ms(100);
        }
    }
}

std::jthread subscribe_thread_;
void start_without_time_server() {
    // listens to pubsub topic "time_sync" and updates
    // the internal offset variables
    subscribe_thread_ = std::jthread(run_subscribe_thread);
}

void init(const network_config::Config& config, const Options& opts) {
    const network_config::SystemConfig system_config = config.get("time_sync");

    if (!system_config.kissnet.ip.empty()) {
        LOG(INFO) << "Attempting to time sync with " << system_config.kissnet.ip
                  << ":" << system_config.kissnet.port;

        socket_ = kissnet::udp_socket(kissnet::endpoint(
            system_config.kissnet.ip, system_config.kissnet.port));
        socket_.set_non_blocking();

        // responsible for pinging the time server intermittently.
        send_thread_ =
            std::jthread([opts]() { run_send_thread(opts.blast_size); });

        // receive pongs from the time server into ingest buffer.
        // blocks until each pong is received.
        receive_thread_ = std::jthread(run_receive_thread);

        // intermittently analyze the ingest buffer, and publish time sync
        // state over pubsub topic "time_sync"
        publish_thread_ = std::jthread(
            [opts]() { run_publish_thread(opts.window_duration_sec); });
    }

    // listens to pubsub topic "time_sync" and updates
    // the internal offset variables
    subscribe_thread_ = std::jthread(run_subscribe_thread);

    if (!system_config.kissnet.ip.empty()) {    
        const uint64_t timeout_ms = 3000;
        const uint64_t start_time = get_process_time_ms();
        while ((get_process_time_ms() - start_time) < timeout_ms) {
            if (received_any_) break;
            sleep_ms(100);
        }
        if (!received_any_) {
            LOG(FATAL) << "Could not connect to the time server.";
        }
        LOG(INFO) << "Connected to time sync server";
    }
}

void cleanup() {
    stop_all();

    if (send_thread_.joinable()) {
        send_thread_.join();
    }
    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }
    if (publish_thread_.joinable()) {
        publish_thread_.join();
    }
    if (subscribe_thread_.joinable()) {
        subscribe_thread_.join();
    }
}

}  // namespace time_sync
}  // namespace axby
