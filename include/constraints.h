#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace sudoku {

constexpr int kSize = 9;
constexpr int kCellCount = 81;
constexpr uint16_t kAllCandidates = 0x01FF; // bits 0..8 => digits 1..9

inline int row_of(int cell) { return cell / kSize; }
inline int col_of(int cell) { return cell % kSize; }
inline int box_of(int cell) { return (row_of(cell) / 3) * 3 + (col_of(cell) / 3); }
inline int to_cell(int row, int col) { return row * kSize + col; }

inline uint16_t digit_to_mask(int digit) { return static_cast<uint16_t>(1u << (digit - 1)); }
inline bool has_digit(uint16_t domain, int digit) { return (domain & digit_to_mask(digit)) != 0; }

struct KillerCage {
    int sum = 0;
    std::vector<int> cells; // flattened indices 0..80
};

struct Thermo {
    std::vector<int> cells; // bulb first, then path
};

struct Arrow {
    int circle_cell = -1;
    std::vector<int> path_cells; // excludes circle; digits may repeat
};

enum class KropkiType {
    WhiteConsecutive,
    BlackDouble
};

struct KropkiDot {
    int a = -1;
    int b = -1;
    KropkiType type = KropkiType::WhiteConsecutive;
};

struct PuzzleDefinition {
    std::array<uint16_t, kCellCount> givens{}; // zero means unspecified, otherwise single-bit domain
    std::vector<int> even_cells;
    std::vector<int> odd_cells;
    std::vector<KillerCage> killer_cages;
    std::vector<Thermo> thermos;
    std::vector<Arrow> arrows;
    std::vector<KropkiDot> kropki_dots;
};

} // namespace sudoku
