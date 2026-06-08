#include "board.hpp"
#include <sstream>
#include <stdexcept>

Board::Board() {
    cells.fill(INIT_SEEDS);
    cells[P0_KALAHA] = 0;
    cells[P1_KALAHA] = 0;
    side = 0;
}

Board::Board(const std::array<int, SIZE>& c, int s) : cells(c), side(s) {}

std::vector<int> Board::legal_moves() const {
    std::vector<int> moves;
    moves.reserve(PITS);
    int start = pit_start(side);
    int end   = pit_end(side);
    for (int i = start; i <= end; i++) {
        if (cells[i] > 0) moves.push_back(i);
    }
    return moves;
}

bool Board::apply_move(int pit) {
    if (cells[pit] <= 0)
        throw std::invalid_argument("Empty pit selected");

    int seeds = cells[pit];
    cells[pit] = 0;

    int opp_kalaha = kalaha_of(1 - side);
    int pos = pit;

    while (seeds > 0) {
        pos = (pos + 1) % SIZE;
        if (pos == opp_kalaha) continue; // skip opponent's kalaha
        cells[pos]++;
        seeds--;
    }

    // Extra turn: last seed in own kalaha
    if (pos == kalaha_of(side)) {
        return true; // side stays the same
    }

    // Capture: last seed in own empty pit (had 1 after placing), opposite non-empty
    if (owns_pit(side, pos) && cells[pos] == 1) {
        int opp_pit = 12 - pos;
        if (cells[opp_pit] > 0) {
            cells[kalaha_of(side)] += cells[pos] + cells[opp_pit];
            cells[pos]    = 0;
            cells[opp_pit] = 0;
        }
    }

    side = 1 - side;
    return false;
}

bool Board::is_terminal() const {
    int sum0 = 0, sum1 = 0;
    for (int i = 0; i < PITS;  i++) sum0 += cells[i];
    for (int i = 7; i <= 12;   i++) sum1 += cells[i];
    return sum0 == 0 || sum1 == 0;
}

void Board::collect_remaining() {
    int sum0 = 0, sum1 = 0;
    for (int i = 0;  i < PITS; i++) { sum0 += cells[i]; cells[i] = 0; }
    for (int i = 7; i <= 12;   i++) { sum1 += cells[i]; cells[i] = 0; }
    cells[P0_KALAHA] += sum0;
    cells[P1_KALAHA] += sum1;
}

int Board::evaluate() const {
    constexpr double ALPHA = 0.1;
    int own_k = cells[kalaha_of(side)];
    int opp_k = cells[kalaha_of(1 - side)];
    int own_seeds = 0, opp_seeds = 0;
    for (int i = pit_start(side); i <= pit_end(side); i++) own_seeds += cells[i];
    for (int i = pit_start(1-side); i <= pit_end(1-side); i++) opp_seeds += cells[i];
    return (own_k - opp_k) + static_cast<int>(ALPHA * (own_seeds - opp_seeds));
}

int Board::winner() const {
    int k0 = cells[P0_KALAHA];
    int k1 = cells[P1_KALAHA];
    if (k0 > k1) return 0;
    if (k1 > k0) return 1;
    return -1;
}

std::string Board::to_string() const {
    std::ostringstream ss;
    ss << "P1: [";
    for (int i = 12; i >= 7; i--) ss << cells[i] << (i > 7 ? "," : "");
    ss << "] K1=" << cells[P1_KALAHA] << "\n";
    ss << "P0: [";
    for (int i = 0;  i < 6;  i++) ss << cells[i] << (i < 5 ? "," : "");
    ss << "] K0=" << cells[P0_KALAHA] << "\n";
    ss << "Turn: P" << side;
    return ss.str();
}
