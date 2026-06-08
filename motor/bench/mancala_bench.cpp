#include "../src/board.hpp"
#include "../src/minimax.hpp"
#include "../src/parallel.hpp"
#include <omp.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <string>
#include <cstdlib>

// Parse one line: 14 ints then side
static Board parse_position(const std::string& line) {
    std::istringstream ss(line);
    std::array<int, Board::SIZE> cells{};
    for (int i = 0; i < Board::SIZE; i++) ss >> cells[i];
    int side = 0;
    ss >> side;
    return Board(cells, side);
}

int main(int argc, char* argv[]) {
    int depth   = 8;
    std::string positions_file;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--depth" && i + 1 < argc)       depth = std::stoi(argv[++i]);
        else if (arg == "--positions" && i + 1 < argc) positions_file = argv[++i];
    }

    int threads = 1;
    const char* env = std::getenv("OMP_NUM_THREADS");
    if (env) threads = std::stoi(env);

    std::vector<Board> positions;
    if (!positions_file.empty()) {
        std::ifstream f(positions_file);
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty() && line[0] != '#') positions.push_back(parse_position(line));
        }
    }

    // Default: use the initial board position
    if (positions.empty()) positions.emplace_back();

    std::cout << "depth,threads,nodes,prunes,wall_ms\n";

    for (auto& board : positions) {
        auto t0 = std::chrono::steady_clock::now();
        SearchResult r = (threads > 1)
            ? parallel_search(board, depth, threads)
            : search(board, depth);
        auto t1 = std::chrono::steady_clock::now();
        long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        std::cout << depth << ","
                  << threads << ","
                  << r.stats.nodes << ","
                  << r.stats.prunes << ","
                  << ms << "\n";
    }
    return 0;
}
