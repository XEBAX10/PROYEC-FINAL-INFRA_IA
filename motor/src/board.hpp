#pragma once
#include <array>
#include <vector>
#include <string>

// Board layout (14 cells):
// [0-5]  = P0 pits   [6]  = P0 kalaha
// [7-12] = P1 pits   [13] = P1 kalaha
// Opposite of pit i  = 12 - i  (works for both sides)
class Board {
public:
    static constexpr int SIZE       = 14;
    static constexpr int PITS       = 6;
    static constexpr int P0_KALAHA  = 6;
    static constexpr int P1_KALAHA  = 13;
    static constexpr int INIT_SEEDS = 4;

    std::array<int, SIZE> cells{};
    int side{0}; // current player to move: 0 or 1

    Board();
    Board(const std::array<int, SIZE>& c, int s);

    std::vector<int> legal_moves() const;

    // Apply move from pit. Returns true if player earns an extra turn.
    // Side switches automatically when no extra turn.
    bool apply_move(int pit);

    bool is_terminal() const;

    // Collect remaining seeds to their owner's kalaha (call when terminal)
    void collect_remaining();

    // Heuristic: (own_kalaha - opp_kalaha) + alpha*(own_seeds - opp_seeds)
    int evaluate() const;

    // -1 = tie, 0 = P0 wins, 1 = P1 wins (only valid after collect_remaining)
    int winner() const;

    std::string to_string() const;

private:
    int kalaha_of(int s) const { return s == 0 ? P0_KALAHA : P1_KALAHA; }
    int pit_start(int s) const { return s == 0 ? 0 : 7; }
    int pit_end(int s)   const { return s == 0 ? 5 : 12; }
    bool owns_pit(int s, int pit) const { return pit >= pit_start(s) && pit <= pit_end(s); }
};
