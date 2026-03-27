#include "bitwise_solver.h"

#include <array>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

using sudoku::BitwiseSudokuSolver;
using sudoku::PuzzleDefinition;
using sudoku::digit_to_mask;
using sudoku::kCellCount;

struct Metrics {
    long long total = 0;
    long long solved = 0;
    long long unsolved = 0;
    long long correct = 0;
    long long solved_but_wrong = 0;
    long long malformed = 0;
};

bool parse_puzzle_string(const std::string& s, PuzzleDefinition& out) {
    if (s.size() != static_cast<size_t>(kCellCount)) {
        return false;
    }

    out.givens.fill(0);
    out.even_cells.clear();
    out.odd_cells.clear();
    out.killer_cages.clear();
    out.thermos.clear();
    out.arrows.clear();
    out.kropki_dots.clear();

    for (int i = 0; i < kCellCount; ++i) {
        const char ch = s[static_cast<size_t>(i)];
        if (ch == '.' || ch == '0') {
            continue;
        }
        if (ch < '1' || ch > '9') {
            return false;
        }
        out.givens[static_cast<size_t>(i)] = digit_to_mask(ch - '0');
    }
    return true;
}

bool parse_solution_string(const std::string& s, std::array<int, kCellCount>& out) {
    if (s.size() != static_cast<size_t>(kCellCount)) {
        return false;
    }

    for (int i = 0; i < kCellCount; ++i) {
        const char ch = s[static_cast<size_t>(i)];
        if (ch < '1' || ch > '9') {
            return false;
        }
        out[static_cast<size_t>(i)] = ch - '0';
    }
    return true;
}

// Minimal CSV split that supports quoted fields.
bool split_csv_line(const std::string& line, std::array<std::string, 5>& cols) {
    cols = {};
    int idx = 0;
    std::string cur;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                cur.push_back('"');
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
            continue;
        }

        if (ch == ',' && !in_quotes) {
            if (idx >= 5) {
                return false;
            }
            cols[static_cast<size_t>(idx)] = cur;
            cur.clear();
            ++idx;
            continue;
        }

        cur.push_back(ch);
    }

    if (idx != 4) {
        return false;
    }
    cols[4] = cur;
    return true;
}

std::string grid_to_string(const std::array<int, kCellCount>& grid) {
    std::string out;
    out.reserve(kCellCount);
    for (int d : grid) {
        if (d < 1 || d > 9) {
            out.push_back('0');
        } else {
            out.push_back(static_cast<char>('0' + d));
        }
    }
    return out;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: benchmark_classic <dataset.csv> [max_rows]\n";
        return 1;
    }

    std::ifstream in(argv[1]);
    if (!in) {
        std::cerr << "Failed to open dataset file.\n";
        return 1;
    }

    long long max_rows = -1;
    if (argc >= 3) {
        try {
            max_rows = std::stoll(argv[2]);
        } catch (...) {
            std::cerr << "Invalid max_rows value.\n";
            return 1;
        }
    }

    std::string header;
    if (!std::getline(in, header)) {
        std::cerr << "Dataset is empty.\n";
        return 1;
    }

    Metrics m;
    std::string line;
    while (std::getline(in, line)) {
        if (max_rows >= 0 && m.total >= max_rows) {
            break;
        }

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::array<std::string, 5> cols;
        if (!split_csv_line(line, cols)) {
            ++m.malformed;
            ++m.total;
            continue;
        }

        PuzzleDefinition puzzle;
        std::array<int, kCellCount> expected{};

        if (!parse_puzzle_string(cols[1], puzzle) || !parse_solution_string(cols[2], expected)) {
            ++m.malformed;
            ++m.total;
            continue;
        }

        BitwiseSudokuSolver solver(std::move(puzzle));
        const bool ok = solver.solve();

        ++m.total;
        if (!ok) {
            ++m.unsolved;
        } else {
            ++m.solved;
            const auto got = solver.solved_grid();
            if (got == expected) {
                ++m.correct;
            } else {
                ++m.solved_but_wrong;
            }
        }

        if (m.total % 100000 == 0) {
            std::cout << "Processed=" << m.total
                      << " Solved=" << m.solved
                      << " Correct=" << m.correct
                      << " Unsolved=" << m.unsolved
                      << " Malformed=" << m.malformed
                      << std::endl;
        }
    }

    const double solve_rate = (m.total > 0) ? (100.0 * static_cast<double>(m.solved) / static_cast<double>(m.total)) : 0.0;
    const double accuracy_total = (m.total > 0) ? (100.0 * static_cast<double>(m.correct) / static_cast<double>(m.total)) : 0.0;
    const double accuracy_solved = (m.solved > 0) ? (100.0 * static_cast<double>(m.correct) / static_cast<double>(m.solved)) : 0.0;

    std::cout << "=== Benchmark Summary ===\n";
    std::cout << "Total: " << m.total << "\n";
    std::cout << "Solved: " << m.solved << "\n";
    std::cout << "Unsolved: " << m.unsolved << "\n";
    std::cout << "Correct (matches dataset solution): " << m.correct << "\n";
    std::cout << "Solved but wrong: " << m.solved_but_wrong << "\n";
    std::cout << "Malformed rows: " << m.malformed << "\n";
    std::cout << "Solve rate (% of total): " << solve_rate << "\n";
    std::cout << "Accuracy on total (%): " << accuracy_total << "\n";
    std::cout << "Accuracy on solved (%): " << accuracy_solved << "\n";

    return 0;
}
