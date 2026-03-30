#pragma once

#include "constraints.h"

#include <nlohmann/json_fwd.hpp>

#include <array>
#include <string>

namespace sudoku {

class BatchSolver {
public:
    bool solve_file(const std::string& input_path, const std::string& output_path) const;

private:
    struct SolveOutcome {
        bool solved = false;
        std::array<int, kCellCount> grid{};
    };

    static SolveOutcome solve_one(const nlohmann::json& puzzle_json);
};

} // namespace sudoku
