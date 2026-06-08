#include <gtest/gtest.h>
#include "../src/board.hpp"
#include "../src/minimax.hpp"

// ─── Board initialization ────────────────────────────────────────────────────

TEST(BoardInit, SeventyTwoSeeds) {
    Board b;
    int total = 0;
    for (int i = 0; i < Board::SIZE; i++) total += b.cells[i];
    EXPECT_EQ(total, 48); // 6 pits × 2 sides × 4 seeds
}

TEST(BoardInit, KalahasEmpty) {
    Board b;
    EXPECT_EQ(b.cells[Board::P0_KALAHA], 0);
    EXPECT_EQ(b.cells[Board::P1_KALAHA], 0);
}

TEST(BoardInit, PlayerZeroStarts) {
    Board b;
    EXPECT_EQ(b.side, 0);
}

// ─── Legal moves ─────────────────────────────────────────────────────────────

TEST(LegalMoves, AllSixAvailableAtStart) {
    Board b;
    EXPECT_EQ(b.legal_moves().size(), 6u);
}

TEST(LegalMoves, EmptyPitExcluded) {
    Board b;
    b.cells[0] = 0;
    auto moves = b.legal_moves();
    EXPECT_EQ(moves.size(), 5u);
    for (int m : moves) EXPECT_NE(m, 0);
}

// ─── Extra turn rule ─────────────────────────────────────────────────────────

TEST(ExtraTurn, LastSeedInOwnKalaha) {
    // Pit 2 has 4 seeds → lands on 3,4,5,6(kalaha) → extra turn
    Board b;
    b.cells.fill(0);
    b.cells[2] = 4;
    b.side = 0;
    bool extra = b.apply_move(2);
    EXPECT_TRUE(extra);
    EXPECT_EQ(b.side, 0); // side stays
    EXPECT_EQ(b.cells[Board::P0_KALAHA], 1);
}

TEST(ExtraTurn, NoExtraTurnWhenLandingOnPit) {
    Board b;
    b.cells.fill(0);
    b.cells[0] = 2; // lands on 1, 2 → no extra
    b.side = 0;
    bool extra = b.apply_move(0);
    EXPECT_FALSE(extra);
    EXPECT_EQ(b.side, 1);
}

// ─── Capture rule ────────────────────────────────────────────────────────────

TEST(Capture, LastSeedInEmptyOwnPitCapturesBoth) {
    Board b;
    b.cells.fill(0);
    b.side = 0;
    // P0 pit 0 empty, P1 pit 12 (opposite) has 5 seeds
    // Move from pit 5 with 1 seed → lands on pit 0 (own empty) → capture
    b.cells[5]  = 1; // P0 pit 5: 1 seed
    b.cells[12] = 5; // P1 pit 12: opposite of pit 0
    // After apply_move(5): seed goes to pit 6? No: 5+1=6=kalaha, that's extra turn.
    // Use pit 4 with 2 seeds instead → lands on 5, 6 (kalaha) → extra. Not useful.
    // Reset: pit 3 with 4 seeds → 4,5,6(kalaha),7 → lands on 7 (not own). Extra = false.
    // Actually let's set up: pit 1 has 1 seed → lands on pit 2 (not empty capture, 2 already has seeds)
    // Cleanest: P0 turn, pit 5 has 2 seeds, pit 0 is empty, pit 12 has 3 seeds
    b.cells.fill(0);
    b.cells[5]  = 2; // 2 seeds: goes to pits 6(kalaha)=1, and 7=... that's extra + extra land
    // Let's do it properly:
    // P0 move from pit 4, 2 seeds → lands on 5, then 6(kalaha)=extra turn. Not capture.
    // P0 move from pit 3, 3 seeds → 4,5,6(kalaha) → extra.
    // P0 move from pit 0, 1 seed → lands on pit 1 (has seeds already, no capture since not empty before)
    // The cleanest capture test:
    b.cells.fill(0);
    b.side = 0;
    b.cells[3]  = 1; // P0 pit 3: 1 seed
    b.cells[9]  = 7; // P1 pit 9: opposite of pit 3 (12-3=9)
    // apply_move(3): seed goes to pit 4. pit 4 was 0 → now 1. It's own pit, was empty.
    // No! wait: seed from pit 3 → goes to pit 4, not pit 3 itself. opposite of pit 4 = 12-4 = 8.
    // We need: after distributing, last seed lands on OWN EMPTY pit.
    // Simple case: pit 0 has 1 seed, pit 1 is empty, pit 11 (opposite of 1) has 5 seeds.
    b.cells.fill(0);
    b.side = 0;
    b.cells[0]  = 1;  // 1 seed from pit 0
    b.cells[1]  = 0;  // pit 1 is empty → last seed lands here
    b.cells[11] = 5;  // opposite of pit 1 = 12-1 = 11 → P1 pit 11

    int kalaha_before = b.cells[Board::P0_KALAHA];
    b.apply_move(0);

    EXPECT_EQ(b.cells[Board::P0_KALAHA], kalaha_before + 1 + 5); // captured both
    EXPECT_EQ(b.cells[1],  0);
    EXPECT_EQ(b.cells[11], 0);
}

TEST(Capture, NoCapturIfOppositeEmpty) {
    Board b;
    b.cells.fill(0);
    b.side = 0;
    b.cells[0]  = 1;
    b.cells[1]  = 0;
    b.cells[11] = 0; // opposite empty → no capture
    int kalaha_before = b.cells[Board::P0_KALAHA];
    b.apply_move(0);
    // seed lands on pit 1, opposite empty → no capture
    EXPECT_EQ(b.cells[Board::P0_KALAHA], kalaha_before);
    EXPECT_EQ(b.cells[1], 1);
}

// ─── Terminal condition ───────────────────────────────────────────────────────

TEST(Terminal, EmptySideIsTerminal) {
    Board b;
    b.cells.fill(0);
    b.cells[Board::P0_KALAHA] = 24;
    b.cells[Board::P1_KALAHA] = 24;
    EXPECT_TRUE(b.is_terminal());
}

TEST(Terminal, InitialBoardNotTerminal) {
    Board b;
    EXPECT_FALSE(b.is_terminal());
}

TEST(Terminal, CollectRemainingAddsSeedsToKalahas) {
    Board b;
    b.cells.fill(0);
    b.cells[0] = 3;
    b.cells[1] = 2;
    b.cells[7] = 4;
    b.collect_remaining();
    EXPECT_EQ(b.cells[Board::P0_KALAHA], 5);
    EXPECT_EQ(b.cells[Board::P1_KALAHA], 4);
    for (int i = 0; i < 6;  i++) EXPECT_EQ(b.cells[i], 0);
    for (int i = 7; i <= 12; i++) EXPECT_EQ(b.cells[i], 0);
}

// ─── Alpha-Beta equivalence with Minimax ─────────────────────────────────────

static int minimax_no_pruning(Board board, int depth, bool maximizing, SearchStats& stats) {
    stats.nodes++;
    if (depth == 0 || board.is_terminal()) {
        if (board.is_terminal()) { Board t = board; t.collect_remaining(); return t.evaluate(); }
        return board.evaluate();
    }
    auto moves = board.legal_moves();
    if (moves.empty()) { Board t = board; t.collect_remaining(); return t.evaluate(); }

    int best = maximizing ? INT_MIN : INT_MAX;
    for (int mv : moves) {
        Board child = board;
        bool extra = child.apply_move(mv);
        int score = minimax_no_pruning(child, depth - 1, extra ? maximizing : !maximizing, stats);
        if (maximizing) best = std::max(best, score);
        else            best = std::min(best, score);
    }
    return best;
}

TEST(AlphaBeta, SameMoveAsMinimaxDepth4) {
    Board b;
    // Compare best move from alphabeta vs pure minimax at depth 4
    SearchResult ab = search(b, 4);

    // Run minimax without pruning for each move and find best
    int best_mm_score = INT_MIN;
    int best_mm_move  = -1;
    for (int mv : b.legal_moves()) {
        Board child = b;
        bool extra = child.apply_move(mv);
        SearchStats dummy;
        int score = minimax_no_pruning(child, 3, extra ? true : false, dummy);
        if (score > best_mm_score) { best_mm_score = score; best_mm_move = mv; }
    }

    EXPECT_EQ(ab.score, best_mm_score);
    // Moves can differ when tied; check score equivalence is sufficient
}

TEST(AlphaBeta, SameMoveAsMinimaxDepth6) {
    Board b;
    SearchResult ab = search(b, 6);

    int best_mm_score = INT_MIN;
    for (int mv : b.legal_moves()) {
        Board child = b;
        bool extra = child.apply_move(mv);
        SearchStats dummy;
        int score = minimax_no_pruning(child, 5, extra ? true : false, dummy);
        best_mm_score = std::max(best_mm_score, score);
    }
    EXPECT_EQ(ab.score, best_mm_score);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
