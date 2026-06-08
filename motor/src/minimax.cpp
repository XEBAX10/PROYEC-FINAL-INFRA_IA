#include "minimax.hpp"

int alphabeta(Board board, int depth, int alpha, int beta,
              bool maximizing, SearchStats& stats) {
    stats.nodes++;

    if (depth == 0 || board.is_terminal()) {
        if (board.is_terminal()) {
            Board tmp = board;
            tmp.collect_remaining();
            // Return score from the perspective of the player who just moved
            // (side has already been flipped by apply_move, so evaluate from current side)
            return tmp.evaluate();
        }
        return board.evaluate();
    }

    auto moves = board.legal_moves();
    if (moves.empty()) {
        Board tmp = board;
        tmp.collect_remaining();
        return tmp.evaluate();
    }

    if (maximizing) {
        int best = INT_MIN;
        for (int mv : moves) {
            Board child = board;
            bool extra = child.apply_move(mv);
            // Extra turn means same player moves again → still maximizing
            int score = alphabeta(child, depth - 1, alpha, beta, extra ? true : false, stats);
            if (score > best) best = score;
            if (score > alpha) alpha = score;
            if (alpha >= beta) { stats.prunes++; break; }
        }
        return best;
    } else {
        int best = INT_MAX;
        for (int mv : moves) {
            Board child = board;
            bool extra = child.apply_move(mv);
            int score = alphabeta(child, depth - 1, alpha, beta, extra ? false : true, stats);
            if (score < best) best = score;
            if (score < beta) beta = score;
            if (alpha >= beta) { stats.prunes++; break; }
        }
        return best;
    }
}

SearchResult search(const Board& board, int depth) {
    SearchResult result;
    auto moves = board.legal_moves();
    if (moves.empty()) return result;

    int alpha = INT_MIN, beta = INT_MAX;
    int best_score = INT_MIN;

    for (int mv : moves) {
        Board child = board;
        bool extra = child.apply_move(mv);
        SearchStats stats;
        // After applying, if extra turn → child is still maximizing
        int score = alphabeta(child, depth - 1, alpha, beta, extra ? true : false, stats);
        result.stats.nodes  += stats.nodes;
        result.stats.prunes += stats.prunes;

        if (score > best_score) {
            best_score   = score;
            result.move  = mv;
            result.score = score;
        }
        if (score > alpha) alpha = score;
    }
    return result;
}
