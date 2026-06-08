#pragma once
#include "board.hpp"
#include <climits>

struct SearchStats {
    long long nodes{0};
    long long prunes{0};
};

struct SearchResult {
    int  score{0};
    int  move{-1};
    SearchStats stats{};
};

// Sequential Minimax with Alpha-Beta pruning.
// maximizing=true means it is the root player's turn.
int alphabeta(Board board, int depth, int alpha, int beta,
              bool maximizing, SearchStats& stats);

// Entry point: returns best move + evaluation for 'board.side'.
SearchResult search(const Board& board, int depth);
