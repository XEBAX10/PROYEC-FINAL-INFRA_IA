#include "board.hpp"
#include "minimax.hpp"
#include "parallel.hpp"
#include "httplib.h"
#include "nlohmann/json.hpp"
#include <omp.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <cstdlib>

using json = nlohmann::json;

// Global counters for /metrics
static std::atomic<long long> g_total_nodes{0};
static std::atomic<long long> g_total_prunes{0};
static std::atomic<long long> g_total_requests{0};
static std::atomic<long long> g_total_ms{0};

static Board board_from_json(const json& j) {
    std::array<int, Board::SIZE> cells{};
    auto arr = j.at("board").get<std::vector<int>>();
    if (arr.size() != Board::SIZE)
        throw std::invalid_argument("board must have 14 elements");
    for (int i = 0; i < Board::SIZE; i++) cells[i] = arr[i];
    int side = j.at("side").get<int>();
    if (side != 0 && side != 1)
        throw std::invalid_argument("side must be 0 or 1");
    return Board(cells, side);
}

int main() {
    const char* port_env = std::getenv("MOTOR_PORT");
    int port = port_env ? std::stoi(port_env) : 8001;

    httplib::Server svr;

    // POST /move
    svr.Post("/move", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body  = json::parse(req.body);
            Board board = board_from_json(body);
            int depth   = body.value("depth",   8);
            int threads = body.value("threads", 1);

            if (depth < 1 || depth > 20)
                throw std::invalid_argument("depth must be 1-20");
            if (threads < 1 || threads > 64)
                throw std::invalid_argument("threads must be 1-64");

            auto t0 = std::chrono::steady_clock::now();
            SearchResult result = (threads > 1)
                ? parallel_search(board, depth, threads)
                : search(board, depth);
            auto t1 = std::chrono::steady_clock::now();

            long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

            g_total_nodes   += result.stats.nodes;
            g_total_prunes  += result.stats.prunes;
            g_total_requests++;
            g_total_ms      += ms;

            json resp;
            resp["move"]         = result.move;
            resp["evaluation"]   = result.score;
            resp["elapsed_ms"]   = ms;
            resp["threads_used"] = threads;
            resp["stats"] = {
                {"nodes",  result.stats.nodes},
                {"prunes", result.stats.prunes}
            };
            res.set_content(resp.dump(), "application/json");
        } catch (const json::exception& e) {
            res.status = 422;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } catch (const std::invalid_argument& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // GET /healthz
    svr.Get("/healthz", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // GET /metrics  (Prometheus text format)
    svr.Get("/metrics", [](const httplib::Request&, httplib::Response& res) {
        std::ostringstream ss;
        ss << "# HELP mancala_requests_total Total POST /move requests\n";
        ss << "# TYPE mancala_requests_total counter\n";
        ss << "mancala_requests_total " << g_total_requests.load() << "\n";
        ss << "# HELP mancala_nodes_total Total nodes explored\n";
        ss << "# TYPE mancala_nodes_total counter\n";
        ss << "mancala_nodes_total " << g_total_nodes.load() << "\n";
        ss << "# HELP mancala_prunes_total Total alpha-beta prunes\n";
        ss << "# TYPE mancala_prunes_total counter\n";
        ss << "mancala_prunes_total " << g_total_prunes.load() << "\n";
        ss << "# HELP mancala_elapsed_ms_total Total elapsed ms\n";
        ss << "# TYPE mancala_elapsed_ms_total counter\n";
        ss << "mancala_elapsed_ms_total " << g_total_ms.load() << "\n";
        res.set_content(ss.str(), "text/plain");
    });

    std::cout << "Motor listening on port " << port << std::endl;
    svr.listen("0.0.0.0", port);
    return 0;
}
