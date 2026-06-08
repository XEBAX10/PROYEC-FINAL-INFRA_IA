#include "parallel.hpp"
#include <omp.h>
#include <climits>
#include <vector>

SearchResult parallel_search(const Board& board, int depth, int num_threads) {
    auto moves = board.legal_moves();
    if (moves.empty()) return {};

    int n = static_cast<int>(moves.size());

    // Per-thread result storage to avoid false sharing
    std::vector<int>       scores(n, INT_MIN);
    std::vector<SearchStats> thread_stats(n);

    // Root Parallelism: each move gets its own independent Alpha-Beta tree.
    // No shared alpha/beta across threads → may explore more nodes than sequential,
    // but is correct and embarrassingly parallel.
    #pragma omp parallel for num_threads(num_threads) schedule(dynamic, 1)
    for (int i = 0; i < n; i++) {
        Board child = board;
        bool extra = child.apply_move(moves[i]);
        SearchStats st;
        scores[i] = alphabeta(child, depth - 1, INT_MIN, INT_MAX,
                               extra ? true : false, st);
        thread_stats[i] = st;
    }

    SearchResult result;
    result.score = INT_MIN;

    for (int i = 0; i < n; i++) {
        result.stats.nodes  += thread_stats[i].nodes;
        result.stats.prunes += thread_stats[i].prunes;
        if (scores[i] > result.score) {
            result.score = scores[i];
            result.move  = moves[i];
        }
    }
    return result;
}
