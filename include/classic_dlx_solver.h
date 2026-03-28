#pragma once

#include "constraints.h"

#include <array>
#include <string>
#include <vector>

namespace sudoku {

struct ClassicDlxResult {
    bool solved = false;
    bool uniqueness_checked = false;
    bool unique = false;
    int solution_count = -1;
    bool solution_count_complete = false;
    std::array<int, kCellCount> solved_grid{};
    std::vector<std::string> logs;
};

bool is_classic_only(const PuzzleDefinition& puzzle);
ClassicDlxResult solve_classic_with_dlx(const PuzzleDefinition& puzzle);

} // namespace sudoku
