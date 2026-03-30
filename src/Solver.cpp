#include "Solver.h"

#include "bitwise_solver.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace sudoku {

namespace {

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

int parse_cell(const json& cell_json) {
    if (!cell_json.is_array() || cell_json.size() != 2) {
        throw std::runtime_error("Cell coordinate must be [row, col].");
    }

    const int r = cell_json[0].get<int>();
    const int c = cell_json[1].get<int>();
    if (r < 1 || r > 9 || c < 1 || c > 9) {
        throw std::runtime_error("Cell coordinate out of range (1..9).");
    }

    return to_cell(r - 1, c - 1);
}

std::vector<int> parse_cells(const json& arr) {
    std::vector<int> out;
    out.reserve(arr.size());
    for (const auto& cell : arr) {
        out.push_back(parse_cell(cell));
    }
    return out;
}

PuzzleDefinition parse_puzzle(const json& j) {
    PuzzleDefinition p;
    p.category.clear();
    p.givens.fill(0);

    if (j.contains("category")) {
        p.category = j.at("category").get<std::string>();
    }

    const json* grid_ptr = nullptr;
    if (j.contains("givens_grid")) {
        grid_ptr = &j.at("givens_grid");
    } else if (j.contains("grid") && j.at("grid").is_object() && j.at("grid").contains("rows")) {
        grid_ptr = &j.at("grid").at("rows");
    }

    if (grid_ptr != nullptr) {
        const auto& grid = *grid_ptr;
        if (!grid.is_array() || grid.size() != 9) {
            throw std::runtime_error("givens_grid must be a 9x9 array.");
        }

        for (int r = 0; r < 9; ++r) {
            if (!grid[r].is_array() || grid[r].size() != 9) {
                throw std::runtime_error("Each givens_grid row must have 9 items.");
            }
            for (int c = 0; c < 9; ++c) {
                const int v = grid[r][c].get<int>();
                if (v < 0 || v > 9) {
                    throw std::runtime_error("Given value out of range 0..9.");
                }
                if (v != 0) {
                    p.givens[to_cell(r, c)] = digit_to_mask(v);
                }
            }
        }
    }

    if (j.contains("givens")) {
        for (const auto& g : j.at("givens")) {
            const int cell = parse_cell(g.at("cell"));
            const int digit = g.at("value").get<int>();
            if (digit < 1 || digit > 9) {
                throw std::runtime_error("Given digit out of range 1..9.");
            }
            p.givens[cell] = digit_to_mask(digit);
        }
    }

    if (j.contains("even_cells")) {
        p.even_cells = parse_cells(j.at("even_cells"));
    }
    if (j.contains("odd_cells")) {
        p.odd_cells = parse_cells(j.at("odd_cells"));
    }

    if (j.contains("killer_cages")) {
        for (const auto& kc : j.at("killer_cages")) {
            KillerCage cage;
            cage.sum = kc.at("sum").get<int>();
            cage.cells = parse_cells(kc.at("cells"));
            p.killer_cages.push_back(std::move(cage));
        }
    }

    if (j.contains("thermos")) {
        for (const auto& th : j.at("thermos")) {
            Thermo thermo;
            thermo.cells = parse_cells(th.at("cells"));
            p.thermos.push_back(std::move(thermo));
        }
    }

    if (j.contains("arrows")) {
        for (const auto& ar : j.at("arrows")) {
            Arrow arrow;
            arrow.circle_cell = parse_cell(ar.at("circle"));
            arrow.path_cells = parse_cells(ar.at("path"));
            p.arrows.push_back(std::move(arrow));
        }
    }

    if (j.contains("kropki")) {
        for (const auto& kd : j.at("kropki")) {
            KropkiDot dot;
            dot.a = parse_cell(kd.at("a"));
            dot.b = parse_cell(kd.at("b"));
            const std::string type = kd.at("type").get<std::string>();
            if (type == "white") {
                dot.type = KropkiType::WhiteConsecutive;
            } else if (type == "black") {
                dot.type = KropkiType::BlackDouble;
            } else {
                throw std::runtime_error("kropki.type must be 'white' or 'black'.");
            }
            p.kropki_dots.push_back(dot);
        }
    }

    // Round 2 speed mode: always stop at first valid solution.
    p.check_uniqueness = false;
    p.count_all_solutions = false;
    p.max_solution_count = 1;

    return p;
}

std::array<int, kCellCount> original_grid_from_json(const json& puzzle_json) {
    std::array<int, kCellCount> grid{};
    grid.fill(0);

    if (!puzzle_json.is_object()) {
        return grid;
    }

    const json* src_ptr = nullptr;
    if (puzzle_json.contains("givens_grid")) {
        src_ptr = &puzzle_json.at("givens_grid");
    } else if (puzzle_json.contains("grid") && puzzle_json.at("grid").is_object() && puzzle_json.at("grid").contains("rows")) {
        src_ptr = &puzzle_json.at("grid").at("rows");
    }
    if (src_ptr == nullptr) {
        return grid;
    }

    const auto& src = *src_ptr;
    if (!src.is_array() || src.size() != 9) {
        return grid;
    }

    for (int r = 0; r < 9; ++r) {
        if (!src[r].is_array() || src[r].size() != 9) {
            return grid;
        }
        for (int c = 0; c < 9; ++c) {
            if (!src[r][c].is_number_integer()) {
                return grid;
            }
            const int v = src[r][c].get<int>();
            if (v < 0 || v > 9) {
                return grid;
            }
            grid[to_cell(r, c)] = v;
        }
    }

    return grid;
}

json grid_to_json(const std::array<int, kCellCount>& grid) {
    json out = json::array();
    for (int r = 0; r < 9; ++r) {
        json row = json::array();
        for (int c = 0; c < 9; ++c) {
            row.push_back(grid[to_cell(r, c)]);
        }
        out.push_back(std::move(row));
    }
    return out;
}

std::string puzzle_id_from_json(const json& puzzle) {
    if (puzzle.is_object() && puzzle.contains("id") && puzzle.at("id").is_string()) {
        return puzzle.at("id").get<std::string>();
    }
    return "";
}

json normalize_puzzle_list(const json& root) {
    if (root.is_object() && root.contains("puzzles") && root.at("puzzles").is_array()) {
        return root.at("puzzles");
    }
    if (root.is_object() && root.contains("questions") && root.at("questions").is_array()) {
        return root.at("questions");
    }
    if (root.is_array()) {
        return root;
    }

    json questions = json::array();
    questions.push_back(root);
    return questions;
}

void write_answer_json(std::ostream& out, const ordered_json& answers) {
    out << "{\n";
    out << "  \"answers\": [\n";

    for (size_t i = 0; i < answers.size(); ++i) {
        const auto& ans = answers[i];
        const std::string id = ans.value("id", std::string{});
        const bool solved = ans.value("solved", false);
        const auto& rows = ans.at("grid").at("rows");

        out << "    {\n";
        out << "      \"id\": " << json(id).dump() << ",\n";
        out << "      \"solved\": " << (solved ? "true" : "false") << ",\n";
        out << "      \"grid\": {\n";
        out << "        \"rows\": [\n";

        for (size_t r = 0; r < rows.size(); ++r) {
            out << "          [";
            const auto& row = rows[r];
            for (size_t c = 0; c < row.size(); ++c) {
                if (c > 0) {
                    out << ", ";
                }
                out << row[c].get<int>();
            }
            out << "]";
            if (r + 1 < rows.size()) {
                out << ",";
            }
            out << "\n";
        }

        out << "        ]\n";
        out << "      }\n";
        out << "    }";
        if (i + 1 < answers.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";
}

} // namespace

BatchSolver::SolveOutcome BatchSolver::solve_one(const nlohmann::json& puzzle_json) {
    SolveOutcome out;
    out.solved = false;
    out.grid = original_grid_from_json(puzzle_json);

    try {
        PuzzleDefinition puzzle = parse_puzzle(puzzle_json);
        BitwiseSudokuSolver solver(std::move(puzzle));
        if (solver.solve()) {
            out.grid = solver.solved_grid();
            out.solved = true;
        }
    } catch (...) {
        // Invalid puzzles are returned unsolved by design for competition stability.
    }

    return out;
}

bool BatchSolver::solve_file(const std::string& input_path, const std::string& output_path) const {
    try {
        std::ifstream in(input_path);
        if (!in) {
            return false;
        }

        json root;
        in >> root;

        const json puzzles = normalize_puzzle_list(root);

        ordered_json answers = ordered_json::array();

        for (const auto& puzzle : puzzles) {
            const SolveOutcome solved = solve_one(puzzle);
            ordered_json ans = ordered_json::object();
            ans["id"] = puzzle_id_from_json(puzzle);
            ans["solved"] = solved.solved;
            ordered_json grid_obj = ordered_json::object();
            grid_obj["rows"] = grid_to_json(solved.grid);
            ans["grid"] = std::move(grid_obj);
            answers.push_back(std::move(ans));
        }

        std::ofstream out(output_path, std::ios::trunc);
        if (!out) {
            return false;
        }
        write_answer_json(out, answers);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace sudoku
