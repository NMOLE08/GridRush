#pragma once

#include "constraints.h"

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace sudoku {

class BitwiseSudokuSolver {
public:
    enum class BinaryRelation {
        LessThan,
        KropkiWhite,
        KropkiBlack
    };

    explicit BitwiseSudokuSolver(PuzzleDefinition definition);

    // Solves one solution and checks uniqueness by searching for a second one.
    bool solve();

    bool is_solved() const;
    bool is_unique() const;

    std::array<int, kCellCount> solved_grid() const;
    const std::vector<std::string>& logs() const;

private:
    struct BinaryEdge {
        int a = -1;
        int b = -1;
        BinaryRelation relation = BinaryRelation::LessThan;
        std::string reason;
    };

    PuzzleDefinition definition_;
    std::array<uint16_t, kCellCount> domains_{};
    std::array<std::vector<int>, kCellCount> classic_peers_{};
    std::vector<BinaryEdge> binary_edges_;
    std::array<std::vector<int>, kCellCount> cell_to_binary_edges_{};
    std::array<std::vector<int>, kCellCount> cell_to_killer_cages_{};
    std::array<std::vector<int>, kCellCount> cell_to_arrows_{};
    std::array<std::vector<std::pair<int, int>>, kCellCount> cell_to_thermo_pos_{}; // {thermo idx, position}

    std::deque<int> queue_;
    std::array<bool, kCellCount> in_queue_{};

    bool solved_ = false;
    bool unique_ = false;
    std::vector<std::string> logs_;

    void build_indices();
    bool initialize_domains();

    void enqueue(int cell);
    bool propagate();

    bool apply_classic_peer_rule(int source_cell);
    bool apply_binary_edge(int edge_idx);
    bool apply_killer_cage(int cage_idx);
    bool apply_arrow(int arrow_idx);
    bool apply_thermo_bounds(int thermo_idx, int pos);

    bool remove_candidates(int cell, uint16_t remove_mask, const std::string& reason);
    void log_candidate_removal(int cell, int digit, const std::string& reason);

    static bool is_singleton(uint16_t domain);
    static int singleton_digit(uint16_t domain);
    static int popcount9(uint16_t domain);
    static int min_digit(uint16_t domain);
    static int max_digit(uint16_t domain);

    bool dfs_with_logging(int depth);

    bool matches_solution(const std::array<uint16_t, kCellCount>& state,
                          const std::array<int, kCellCount>& solved_grid) const;
    int count_solutions(std::array<uint16_t, kCellCount> state,
                        const std::optional<std::array<int, kCellCount>>& forbidden,
                        int limit) const;
    bool propagate_state(std::array<uint16_t, kCellCount>& state) const;
};

} // namespace sudoku
