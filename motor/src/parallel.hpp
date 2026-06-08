#pragma once
#include "board.hpp"
#include "minimax.hpp"

// Root Parallelism strategy with OpenMP.
// Distributes legal moves of the root among threads.
// Each thread runs sequential Alpha-Beta on its sub-tree.
// Synchronizes only to update the global best.
SearchResult parallel_search(const Board& board, int depth, int num_threads);
