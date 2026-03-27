#include "bitwise_solver.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

namespace {

using json = nlohmann::json;
using sudoku::Arrow;
using sudoku::KillerCage;
using sudoku::KropkiDot;
using sudoku::KropkiType;
using sudoku::PuzzleDefinition;
using sudoku::Thermo;
using sudoku::digit_to_mask;
using sudoku::kCellCount;
using sudoku::to_cell;

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
    for (const auto& c : arr) {
        out.push_back(parse_cell(c));
    }
    return out;
}

PuzzleDefinition parse_puzzle(const json& j) {
    PuzzleDefinition p;
    p.givens.fill(0);

    if (j.contains("givens_grid")) {
        const auto& grid = j.at("givens_grid");
        if (!grid.is_array() || grid.size() != 9) {
            throw std::runtime_error("givens_grid must be a 9x9 array.");
        }
        for (int r = 0; r < 9; ++r) {
            if (!grid[r].is_array() || grid[r].size() != 9) {
                throw std::runtime_error("Each givens_grid row must have 9 items.");
            }
            for (int c = 0; c < 9; ++c) {
                const int v = grid[r][c].get<int>();
                if (v < 0 || v > 9) throw std::runtime_error("Given value out of range 0..9.");
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
            if (digit < 1 || digit > 9) throw std::runtime_error("Given digit out of range 1..9.");
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

    return p;
}

void print_grid(const std::array<int, kCellCount>& grid) {
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            std::cout << grid[to_cell(r, c)] << (c == 8 ? '\n' : ' ');
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: sudoku_solver <input.json>\n";
        return 1;
    }

    try {
        std::ifstream in(argv[1]);
        if (!in) {
            throw std::runtime_error("Failed to open input JSON file.");
        }

        json j;
        in >> j;

        PuzzleDefinition puzzle = parse_puzzle(j);
        sudoku::BitwiseSudokuSolver solver(std::move(puzzle));

        if (!solver.solve()) {
            std::cout << "No valid solution found.\n";
            for (const auto& line : solver.logs()) {
                std::cout << line << '\n';
            }
            return 2;
        }

        std::cout << "Solved grid:\n";
        print_grid(solver.solved_grid());
        std::cout << "\nUnique: " << (solver.is_unique() ? "true" : "false") << "\n\n";

        std::cout << "Glassbox solving log:\n";
        for (const auto& line : solver.logs()) {
            std::cout << line << '\n';
        }

    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
