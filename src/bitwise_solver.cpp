#include "bitwise_solver.h"
#include "log_translator.h"

#include <algorithm>
#include <bitset>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace sudoku {

namespace {

std::string rc_label(int cell) {
    std::ostringstream oss;
    oss << "R" << (row_of(cell) + 1) << "C" << (col_of(cell) + 1);
    return oss.str();
}

bool relation_allows(BitwiseSudokuSolver::BinaryRelation relation, int a, int b) {
    switch (relation) {
        case BitwiseSudokuSolver::BinaryRelation::LessThan:
            return a < b;
        case BitwiseSudokuSolver::BinaryRelation::KropkiWhite:
            return std::abs(a - b) == 1;
        case BitwiseSudokuSolver::BinaryRelation::KropkiBlack:
            return (a == 2 * b) || (b == 2 * a);
    }
    return false;
}

} // namespace

BitwiseSudokuSolver::BitwiseSudokuSolver(PuzzleDefinition definition)
    : definition_(std::move(definition)) {
    build_indices();
}

bool BitwiseSudokuSolver::solve() {
    logs_.clear();
    solved_ = false;
    uniqueness_checked_ = false;
    unique_ = false;
    solution_count_ = -1;
    solution_count_complete_ = false;

    if (!initialize_domains()) {
        logs_.push_back("Initial puzzle is contradictory during domain initialization.");
        return false;
    }

    for (int c = 0; c < kCellCount; ++c) {
        enqueue(c);
    }

    if (!propagate()) {
        logs_.push_back("Contradiction found during initial AC-3 propagation.");
        return false;
    }

    if (!dfs_with_logging(0)) {
        logs_.push_back("No solution exists for the provided constraints.");
        return false;
    }

    solved_ = true;

    if (!definition_.check_uniqueness) {
        logs_.push_back("Uniqueness check skipped by configuration.");
        return true;
    }

    uniqueness_checked_ = true;

    const auto first_solution = solved_grid();
    std::array<uint16_t, kCellCount> fresh{};
    for (int i = 0; i < kCellCount; ++i) {
        fresh[i] = kAllCandidates;
        if (definition_.givens[i] != 0) {
            fresh[i] = definition_.givens[i];
        }
    }
    // Reapply parity masks to fresh state.
    for (int cell : definition_.even_cells) {
        fresh[cell] &= static_cast<uint16_t>(digit_to_mask(2) | digit_to_mask(4) | digit_to_mask(6) | digit_to_mask(8));
    }
    for (int cell : definition_.odd_cells) {
        fresh[cell] &= static_cast<uint16_t>(digit_to_mask(1) | digit_to_mask(3) | digit_to_mask(5) | digit_to_mask(7) | digit_to_mask(9));
    }

    const int configured_limit = (definition_.max_solution_count > 0) ? definition_.max_solution_count : 5000;
    const int additional_limit = std::max(1, configured_limit - 1);
    const int found = count_solutions(fresh, first_solution, additional_limit);

    unique_ = (found == 0);
    solution_count_ = 1 + found;
    solution_count_complete_ = (found < additional_limit);

    if (unique_) {
        logs_.push_back("Uniqueness check: solution is unique.");
    } else if (solution_count_complete_) {
        logs_.push_back("Uniqueness check: puzzle is not unique; counted " + std::to_string(solution_count_) + " valid solutions.");
    } else {
        logs_.push_back("Uniqueness check: puzzle is not unique; counted at least " + std::to_string(solution_count_) + " solutions before reaching the configured cap.");
    }
    return true;
}

bool BitwiseSudokuSolver::is_solved() const {
    return solved_;
}

bool BitwiseSudokuSolver::is_uniqueness_checked() const {
    return uniqueness_checked_;
}

bool BitwiseSudokuSolver::is_unique() const {
    return unique_;
}

int BitwiseSudokuSolver::solution_count() const {
    return solution_count_;
}

bool BitwiseSudokuSolver::solution_count_complete() const {
    return solution_count_complete_;
}

std::array<int, kCellCount> BitwiseSudokuSolver::solved_grid() const {
    std::array<int, kCellCount> out{};
    for (int i = 0; i < kCellCount; ++i) {
        out[i] = is_singleton(domains_[i]) ? singleton_digit(domains_[i]) : 0;
    }
    return out;
}

const std::vector<std::string>& BitwiseSudokuSolver::logs() const {
    return logs_;
}

void BitwiseSudokuSolver::build_indices() {
    for (auto& peers : classic_peers_) {
        peers.clear();
    }

    for (int cell = 0; cell < kCellCount; ++cell) {
        const int r = row_of(cell);
        const int c = col_of(cell);

        std::array<bool, kCellCount> seen{};

        for (int cc = 0; cc < kSize; ++cc) {
            int other = to_cell(r, cc);
            if (other != cell && !seen[other]) {
                classic_peers_[cell].push_back(other);
                seen[other] = true;
            }
        }
        for (int rr = 0; rr < kSize; ++rr) {
            int other = to_cell(rr, c);
            if (other != cell && !seen[other]) {
                classic_peers_[cell].push_back(other);
                seen[other] = true;
            }
        }

        const int br = (r / 3) * 3;
        const int bc = (c / 3) * 3;
        for (int dr = 0; dr < 3; ++dr) {
            for (int dc = 0; dc < 3; ++dc) {
                int other = to_cell(br + dr, bc + dc);
                if (other != cell && !seen[other]) {
                    classic_peers_[cell].push_back(other);
                    seen[other] = true;
                }
            }
        }
    }

    binary_edges_.clear();
    for (auto& v : cell_to_binary_edges_) {
        v.clear();
    }
    for (auto& v : cell_to_killer_cages_) {
        v.clear();
    }
    for (auto& v : cell_to_arrows_) {
        v.clear();
    }
    for (auto& v : cell_to_thermo_pos_) {
        v.clear();
    }

    for (int i = 0; i < static_cast<int>(definition_.kropki_dots.size()); ++i) {
        const auto& dot = definition_.kropki_dots[i];
        BinaryEdge e;
        e.a = dot.a;
        e.b = dot.b;
        e.relation = (dot.type == KropkiType::BlackDouble)
                         ? BinaryRelation::KropkiBlack
                         : BinaryRelation::KropkiWhite;
        e.reason = (dot.type == KropkiType::BlackDouble)
                       ? "Kropki black-dot doubling constraint"
                       : "Kropki white-dot consecutive constraint";
        const int idx = static_cast<int>(binary_edges_.size());
        binary_edges_.push_back(e);
        cell_to_binary_edges_[e.a].push_back(idx);
        cell_to_binary_edges_[e.b].push_back(idx);
    }

    for (int t = 0; t < static_cast<int>(definition_.thermos.size()); ++t) {
        const auto& thermo = definition_.thermos[t];
        for (int p = 0; p < static_cast<int>(thermo.cells.size()); ++p) {
            cell_to_thermo_pos_[thermo.cells[p]].push_back({t, p});
            if (p + 1 < static_cast<int>(thermo.cells.size())) {
                BinaryEdge e;
                e.a = thermo.cells[p];
                e.b = thermo.cells[p + 1];
                e.relation = BinaryRelation::LessThan;
                e.reason = "Thermo increasing path constraint";
                const int idx = static_cast<int>(binary_edges_.size());
                binary_edges_.push_back(e);
                cell_to_binary_edges_[e.a].push_back(idx);
                cell_to_binary_edges_[e.b].push_back(idx);
            }
        }
    }

    for (int cage_idx = 0; cage_idx < static_cast<int>(definition_.killer_cages.size()); ++cage_idx) {
        for (int cell : definition_.killer_cages[cage_idx].cells) {
            cell_to_killer_cages_[cell].push_back(cage_idx);
        }
    }

    for (int arrow_idx = 0; arrow_idx < static_cast<int>(definition_.arrows.size()); ++arrow_idx) {
        const auto& arrow = definition_.arrows[arrow_idx];
        if (arrow.circle_cell >= 0) {
            cell_to_arrows_[arrow.circle_cell].push_back(arrow_idx);
        }
        for (int cell : arrow.path_cells) {
            cell_to_arrows_[cell].push_back(arrow_idx);
        }
    }
}

bool BitwiseSudokuSolver::initialize_domains() {
    domains_.fill(kAllCandidates);
    in_queue_.fill(false);
    queue_.clear();

    for (int cell = 0; cell < kCellCount; ++cell) {
        if (definition_.givens[cell] != 0) {
            domains_[cell] = definition_.givens[cell];
        }
    }

    const uint16_t even_mask = static_cast<uint16_t>(digit_to_mask(2) | digit_to_mask(4) | digit_to_mask(6) | digit_to_mask(8));
    const uint16_t odd_mask = static_cast<uint16_t>(digit_to_mask(1) | digit_to_mask(3) | digit_to_mask(5) | digit_to_mask(7) | digit_to_mask(9));

    for (int cell : definition_.even_cells) {
        if (!remove_candidates(cell, static_cast<uint16_t>(domains_[cell] & static_cast<uint16_t>(~even_mask)), "Even/Odd marker (even-only)") ) {
            return false;
        }
    }
    for (int cell : definition_.odd_cells) {
        if (!remove_candidates(cell, static_cast<uint16_t>(domains_[cell] & static_cast<uint16_t>(~odd_mask)), "Even/Odd marker (odd-only)") ) {
            return false;
        }
    }

    for (uint16_t d : domains_) {
        if (d == 0) return false;
    }
    return true;
}

void BitwiseSudokuSolver::enqueue(int cell) {
    if (!in_queue_[cell]) {
        queue_.push_back(cell);
        in_queue_[cell] = true;
    }
}

bool BitwiseSudokuSolver::propagate() {
    while (!queue_.empty()) {
        const int cell = queue_.front();
        queue_.pop_front();
        in_queue_[cell] = false;

        if (domains_[cell] == 0) {
            return false;
        }

        if (!apply_classic_peer_rule(cell)) return false;

        for (int edge_idx : cell_to_binary_edges_[cell]) {
            if (!apply_binary_edge(edge_idx)) return false;
        }

        for (int cage_idx : cell_to_killer_cages_[cell]) {
            if (!apply_killer_cage(cage_idx)) return false;
        }

        for (int arrow_idx : cell_to_arrows_[cell]) {
            if (!apply_arrow(arrow_idx)) return false;
        }

        for (const auto& [thermo_idx, pos] : cell_to_thermo_pos_[cell]) {
            if (!apply_thermo_bounds(thermo_idx, pos)) return false;
        }
    }

    for (uint16_t d : domains_) {
        if (d == 0) return false;
    }
    return true;
}

bool BitwiseSudokuSolver::apply_classic_peer_rule(int source_cell) {
    if (!is_singleton(domains_[source_cell])) {
        return true;
    }

    const uint16_t bit = domains_[source_cell];
    const int val = singleton_digit(bit);

    for (int peer : classic_peers_[source_cell]) {
        if ((domains_[peer] & bit) != 0) {
            std::string reason = "Classic Sudoku row/column/box distinctness from " + rc_label(source_cell) + "=" + std::to_string(val);
            if (!remove_candidates(peer, bit, reason)) {
                return false;
            }
        }
    }
    return true;
}

bool BitwiseSudokuSolver::apply_binary_edge(int edge_idx) {
    const auto& e = binary_edges_[edge_idx];
    uint16_t da = domains_[e.a];
    uint16_t db = domains_[e.b];

    uint16_t remove_a = 0;
    uint16_t remove_b = 0;

    for (int va = 1; va <= 9; ++va) {
        const uint16_t ba = digit_to_mask(va);
        if ((da & ba) == 0) continue;
        bool supported = false;
        for (int vb = 1; vb <= 9; ++vb) {
            const uint16_t bb = digit_to_mask(vb);
            if ((db & bb) == 0) continue;
            if (relation_allows(e.relation, va, vb)) {
                supported = true;
                break;
            }
        }
        if (!supported) remove_a |= ba;
    }

    for (int vb = 1; vb <= 9; ++vb) {
        const uint16_t bb = digit_to_mask(vb);
        if ((db & bb) == 0) continue;
        bool supported = false;
        for (int va = 1; va <= 9; ++va) {
            const uint16_t ba = digit_to_mask(va);
            if ((da & ba) == 0) continue;
            if (relation_allows(e.relation, va, vb)) {
                supported = true;
                break;
            }
        }
        if (!supported) remove_b |= bb;
    }

    auto relation_reason_for = [&](int other_cell) {
        if (e.relation == BinaryRelation::LessThan) {
            std::ostringstream oss;
            oss << "Thermo increasing path constraint with " << rc_label(other_cell);
            if (is_singleton(domains_[other_cell])) {
                oss << "=" << singleton_digit(domains_[other_cell]);
            }
            return oss.str();
        }
        return e.reason + " with " + rc_label(other_cell);
    };

    if (remove_a != 0) {
        if (!remove_candidates(e.a, remove_a, relation_reason_for(e.b))) return false;
    }
    if (remove_b != 0) {
        if (!remove_candidates(e.b, remove_b, relation_reason_for(e.a))) return false;
    }

    return domains_[e.a] != 0 && domains_[e.b] != 0;
}

bool BitwiseSudokuSolver::apply_killer_cage(int cage_idx) {
    const auto& cage = definition_.killer_cages[cage_idx];

    // No repeat in cage.
    for (int cell : cage.cells) {
        if (!is_singleton(domains_[cell])) continue;
        const uint16_t bit = domains_[cell];
        const int val = singleton_digit(bit);
        for (int other : cage.cells) {
            if (other == cell) continue;
            if ((domains_[other] & bit) != 0) {
                if (!remove_candidates(other, bit,
                                       "Killer Cage no-repeat in sum " + std::to_string(cage.sum) +
                                           " due to " + rc_label(cell) + "=" + std::to_string(val))) {
                    return false;
                }
            }
        }
    }

    // Subset-sum support pruning: keep only values that can participate in a valid
    // no-repeat cage assignment reaching the target sum.
    for (int idx = 0; idx < static_cast<int>(cage.cells.size()); ++idx) {
        const int cell = cage.cells[idx];
        uint16_t dom = domains_[cell];
        uint16_t remove = 0;

        for (int d = 1; d <= 9; ++d) {
            const uint16_t bit = digit_to_mask(d);
            if ((dom & bit) == 0) continue;

            if (!killer_support_exists(cage, idx, d)) {
                remove |= bit;
            }
        }

        if (remove != 0) {
            if (!remove_candidates(cell, remove,
                                   "Killer Cage sum constraint target " + std::to_string(cage.sum))) {
                return false;
            }
        }
    }

    return true;
}

bool BitwiseSudokuSolver::killer_support_exists(const KillerCage& cage, int fixed_index, int fixed_digit) const {
    const int target_remaining = cage.sum - fixed_digit;
    if (target_remaining < 0) {
        return false;
    }

    std::vector<int> others;
    others.reserve(cage.cells.size());
    for (int i = 0; i < static_cast<int>(cage.cells.size()); ++i) {
        if (i != fixed_index) {
            others.push_back(cage.cells[i]);
        }
    }

    std::sort(others.begin(), others.end(), [&](int a, int b) {
        return popcount9(domains_[a]) < popcount9(domains_[b]);
    });

    std::function<bool(int, int, uint16_t)> dfs = [&](int pos, int rem, uint16_t used_mask) -> bool {
        if (rem < 0) return false;
        if (pos == static_cast<int>(others.size())) {
            return rem == 0;
        }

        const uint16_t dom = domains_[others[pos]];
        for (int d = 1; d <= 9; ++d) {
            const uint16_t bit = digit_to_mask(d);
            if ((dom & bit) == 0) continue;
            if ((used_mask & bit) != 0) continue;
            if (d > rem) continue;
            if (dfs(pos + 1, rem - d, static_cast<uint16_t>(used_mask | bit))) {
                return true;
            }
        }
        return false;
    };

    return dfs(0, target_remaining, digit_to_mask(fixed_digit));
}

bool BitwiseSudokuSolver::killer_support_exists_state(const KillerCage& cage,
                                                      const std::array<uint16_t, kCellCount>& state,
                                                      int fixed_index,
                                                      int fixed_digit) {
    const int target_remaining = cage.sum - fixed_digit;
    if (target_remaining < 0) {
        return false;
    }

    auto popcount_domain = [](uint16_t domain) {
        int count = 0;
        for (int d = 1; d <= 9; ++d) {
            if ((domain & digit_to_mask(d)) != 0) {
                ++count;
            }
        }
        return count;
    };

    std::vector<int> others;
    others.reserve(cage.cells.size());
    for (int i = 0; i < static_cast<int>(cage.cells.size()); ++i) {
        if (i != fixed_index) {
            others.push_back(cage.cells[i]);
        }
    }

    std::sort(others.begin(), others.end(), [&](int a, int b) {
        return popcount_domain(state[a]) < popcount_domain(state[b]);
    });

    std::function<bool(int, int, uint16_t)> dfs = [&](int pos, int rem, uint16_t used_mask) -> bool {
        if (rem < 0) return false;
        if (pos == static_cast<int>(others.size())) {
            return rem == 0;
        }

        const uint16_t dom = state[others[pos]];
        for (int d = 1; d <= 9; ++d) {
            const uint16_t bit = digit_to_mask(d);
            if ((dom & bit) == 0) continue;
            if ((used_mask & bit) != 0) continue;
            if (d > rem) continue;
            if (dfs(pos + 1, rem - d, static_cast<uint16_t>(used_mask | bit))) {
                return true;
            }
        }
        return false;
    };

    return dfs(0, target_remaining, digit_to_mask(fixed_digit));
}

bool BitwiseSudokuSolver::apply_arrow(int arrow_idx) {
    const auto& arrow = definition_.arrows[arrow_idx];
    if (arrow.circle_cell < 0 || arrow.path_cells.empty()) return true;

    std::bitset<46> sums;
    sums.set(0);
    for (int cell : arrow.path_cells) {
        std::bitset<46> next;
        for (int s = 0; s <= 45; ++s) {
            if (!sums.test(s)) continue;
            for (int d = 1; d <= 9; ++d) {
                if ((domains_[cell] & digit_to_mask(d)) == 0) continue;
                if (s + d <= 45) next.set(s + d);
            }
        }
        sums = next;
    }

    uint16_t remove_circle = 0;
    for (int d = 1; d <= 9; ++d) {
        const uint16_t bit = digit_to_mask(d);
        if ((domains_[arrow.circle_cell] & bit) == 0) continue;
        if (!sums.test(d)) remove_circle |= bit;
    }
    if (remove_circle != 0) {
        if (!remove_candidates(arrow.circle_cell,
                               remove_circle,
                               "Arrow circle equals path sum constraint (max circle " + std::to_string(max_digit(domains_[arrow.circle_cell])) + ")")) {
            return false;
        }
    }

    // Per path cell support check.
    for (int i = 0; i < static_cast<int>(arrow.path_cells.size()); ++i) {
        const int cell = arrow.path_cells[i];
        std::bitset<46> other_sums;
        other_sums.set(0);
        for (int j = 0; j < static_cast<int>(arrow.path_cells.size()); ++j) {
            if (j == i) continue;
            const int other = arrow.path_cells[j];
            std::bitset<46> next;
            for (int s = 0; s <= 45; ++s) {
                if (!other_sums.test(s)) continue;
                for (int d = 1; d <= 9; ++d) {
                    if ((domains_[other] & digit_to_mask(d)) == 0) continue;
                    if (s + d <= 45) next.set(s + d);
                }
            }
            other_sums = next;
        }

        uint16_t remove = 0;
        for (int v = 1; v <= 9; ++v) {
            const uint16_t bit = digit_to_mask(v);
            if ((domains_[cell] & bit) == 0) continue;

            bool supported = false;
            for (int cv = 1; cv <= 9; ++cv) {
                if ((domains_[arrow.circle_cell] & digit_to_mask(cv)) == 0) continue;
                const int need = cv - v;
                if (need >= 0 && need <= 45 && other_sums.test(need)) {
                    supported = true;
                    break;
                }
            }
            if (!supported) remove |= bit;
        }

        if (remove != 0) {
            if (!remove_candidates(cell,
                                   remove,
                                   "Arrow path-to-circle sum constraint (max circle " + std::to_string(max_digit(domains_[arrow.circle_cell])) + ")")) {
                return false;
            }
        }
    }

    return true;
}

bool BitwiseSudokuSolver::apply_thermo_bounds(int thermo_idx, int pos) {
    const auto& thermo = definition_.thermos[thermo_idx];
    const int cell = thermo.cells[pos];
    const int len = static_cast<int>(thermo.cells.size());

    const int min_allowed = pos + 1;
    const int max_allowed = 9 - (len - 1 - pos);

    uint16_t remove = 0;
    for (int d = 1; d <= 9; ++d) {
        if (d < min_allowed || d > max_allowed) {
            if ((domains_[cell] & digit_to_mask(d)) != 0) {
                remove |= digit_to_mask(d);
            }
        }
    }

    if (remove != 0) {
        if (!remove_candidates(cell, remove, "Thermo positional bound constraint")) {
            return false;
        }
    }

    return true;
}

bool BitwiseSudokuSolver::remove_candidates(int cell, uint16_t remove_mask, const std::string& reason) {
    const uint16_t actual_remove = static_cast<uint16_t>(domains_[cell] & remove_mask);
    if (actual_remove == 0) return true;

    const uint16_t resulting_domain = static_cast<uint16_t>(domains_[cell] & static_cast<uint16_t>(~actual_remove));

    for (int d = 1; d <= 9; ++d) {
        const uint16_t bit = digit_to_mask(d);
        if ((actual_remove & bit) != 0) {
            log_candidate_removal(cell, d, reason, resulting_domain);
        }
    }

    domains_[cell] = resulting_domain;
    if (domains_[cell] == 0) return false;

    enqueue(cell);
    return true;
}

void BitwiseSudokuSolver::log_candidate_removal(int cell,
                                                int digit,
                                                const std::string& reason,
                                                uint16_t resulting_domain) {
    logs_.push_back(LogTranslator::translate_candidate_removal(definition_, cell, digit, reason, resulting_domain));
}

bool BitwiseSudokuSolver::is_singleton(uint16_t domain) {
    return domain != 0 && (domain & static_cast<uint16_t>(domain - 1)) == 0;
}

int BitwiseSudokuSolver::singleton_digit(uint16_t domain) {
    for (int d = 1; d <= 9; ++d) {
        if (domain == digit_to_mask(d)) return d;
    }
    return 0;
}

int BitwiseSudokuSolver::popcount9(uint16_t domain) {
    int c = 0;
    for (int d = 1; d <= 9; ++d) {
        if ((domain & digit_to_mask(d)) != 0) ++c;
    }
    return c;
}

int BitwiseSudokuSolver::min_digit(uint16_t domain) {
    for (int d = 1; d <= 9; ++d) {
        if ((domain & digit_to_mask(d)) != 0) return d;
    }
    return 10;
}

int BitwiseSudokuSolver::max_digit(uint16_t domain) {
    for (int d = 9; d >= 1; --d) {
        if ((domain & digit_to_mask(d)) != 0) return d;
    }
    return 0;
}

bool BitwiseSudokuSolver::dfs_with_logging(int depth) {
    bool complete = true;
    int best_cell = -1;
    int best_entropy = 10;

    for (int cell = 0; cell < kCellCount; ++cell) {
        const int pc = popcount9(domains_[cell]);
        if (pc == 0) return false;
        if (pc > 1) {
            complete = false;
            if (pc < best_entropy) {
                best_entropy = pc;
                best_cell = cell;
            }
        }
    }

    if (complete) {
        return true;
    }

    const uint16_t dom = domains_[best_cell];
    for (int d = 1; d <= 9; ++d) {
        const uint16_t bit = digit_to_mask(d);
        if ((dom & bit) == 0) continue;

        BitwiseSudokuSolver branch = *this;
        {
            std::ostringstream oss;
            oss << "Testing assumption at depth " << depth << ": place " << d
                << " in " << rc_label(best_cell) << ".";
            branch.logs_.push_back(oss.str());
        }

        branch.domains_[best_cell] = bit;
        branch.enqueue(best_cell);

        if (!branch.propagate()) {
            std::ostringstream oss;
            oss << "Assumption " << rc_label(best_cell) << "=" << d
                << " creates a contradiction. Reverting.";
            logs_.push_back(oss.str());
            continue;
        }

        if (branch.dfs_with_logging(depth + 1)) {
            *this = std::move(branch);
            return true;
        }

        std::ostringstream oss;
        oss << "Assumption " << rc_label(best_cell) << "=" << d
            << " does not lead to a full solution. Reverting.";
        logs_.push_back(oss.str());
    }

    return false;
}

bool BitwiseSudokuSolver::matches_solution(const std::array<uint16_t, kCellCount>& state,
                                           const std::array<int, kCellCount>& solved_grid) const {
    for (int i = 0; i < kCellCount; ++i) {
        if (!is_singleton(state[i])) return false;
        if (singleton_digit(state[i]) != solved_grid[i]) return false;
    }
    return true;
}

int BitwiseSudokuSolver::count_solutions(std::array<uint16_t, kCellCount> state,
                                         const std::optional<std::array<int, kCellCount>>& forbidden,
                                         int limit) const {
    if (!propagate_state(state)) {
        return 0;
    }

    bool complete = true;
    int best_cell = -1;
    int best_entropy = 10;
    for (int i = 0; i < kCellCount; ++i) {
        const int pc = popcount9(state[i]);
        if (pc == 0) return 0;
        if (pc > 1) {
            complete = false;
            if (pc < best_entropy) {
                best_entropy = pc;
                best_cell = i;
            }
        }
    }

    if (complete) {
        if (forbidden && matches_solution(state, *forbidden)) {
            return 0;
        }
        return 1;
    }

    int total = 0;
    const uint16_t dom = state[best_cell];
    for (int d = 1; d <= 9; ++d) {
        const uint16_t bit = digit_to_mask(d);
        if ((dom & bit) == 0) continue;
        auto next = state;
        next[best_cell] = bit;
        total += count_solutions(next, forbidden, limit - total);
        if (total >= limit) return total;
    }
    return total;
}

bool BitwiseSudokuSolver::propagate_state(std::array<uint16_t, kCellCount>& state) const {
    std::deque<int> q;
    std::array<bool, kCellCount> inq{};
    for (int i = 0; i < kCellCount; ++i) {
        q.push_back(i);
        inq[i] = true;
    }

    auto remove_state = [&](int cell, uint16_t mask) -> bool {
        uint16_t actual = static_cast<uint16_t>(state[cell] & mask);
        if (actual == 0) return true;
        state[cell] = static_cast<uint16_t>(state[cell] & static_cast<uint16_t>(~actual));
        if (state[cell] == 0) return false;
        if (!inq[cell]) {
            q.push_back(cell);
            inq[cell] = true;
        }
        return true;
    };

    while (!q.empty()) {
        const int cell = q.front();
        q.pop_front();
        inq[cell] = false;

        if (state[cell] == 0) return false;

        if (is_singleton(state[cell])) {
            const uint16_t bit = state[cell];
            for (int peer : classic_peers_[cell]) {
                if (!remove_state(peer, bit)) return false;
            }
        }

        for (int edge_idx : cell_to_binary_edges_[cell]) {
            const auto& e = binary_edges_[edge_idx];
            uint16_t da = state[e.a];
            uint16_t db = state[e.b];
            uint16_t remove_a = 0, remove_b = 0;

            for (int va = 1; va <= 9; ++va) {
                const uint16_t ba = digit_to_mask(va);
                if ((da & ba) == 0) continue;
                bool supported = false;
                for (int vb = 1; vb <= 9; ++vb) {
                    const uint16_t bb = digit_to_mask(vb);
                    if ((db & bb) == 0) continue;
                    if (relation_allows(e.relation, va, vb)) {
                        supported = true;
                        break;
                    }
                }
                if (!supported) remove_a |= ba;
            }
            for (int vb = 1; vb <= 9; ++vb) {
                const uint16_t bb = digit_to_mask(vb);
                if ((db & bb) == 0) continue;
                bool supported = false;
                for (int va = 1; va <= 9; ++va) {
                    const uint16_t ba = digit_to_mask(va);
                    if ((da & ba) == 0) continue;
                    if (relation_allows(e.relation, va, vb)) {
                        supported = true;
                        break;
                    }
                }
                if (!supported) remove_b |= bb;
            }

            if (!remove_state(e.a, remove_a)) return false;
            if (!remove_state(e.b, remove_b)) return false;
        }

        // Killer cages.
        for (int cage_idx : cell_to_killer_cages_[cell]) {
            const auto& cage = definition_.killer_cages[cage_idx];

            for (int c1 : cage.cells) {
                if (!is_singleton(state[c1])) continue;
                uint16_t bit = state[c1];
                for (int c2 : cage.cells) {
                    if (c1 == c2) continue;
                    if (!remove_state(c2, bit)) return false;
                }
            }

            for (int idx = 0; idx < static_cast<int>(cage.cells.size()); ++idx) {
                int c = cage.cells[idx];
                uint16_t dom = state[c];
                uint16_t remove = 0;
                for (int d = 1; d <= 9; ++d) {
                    uint16_t bit = digit_to_mask(d);
                    if ((dom & bit) == 0) continue;
                    if (!killer_support_exists_state(cage, state, idx, d)) {
                        remove |= bit;
                    }
                }
                if (!remove_state(c, remove)) return false;
            }
        }

        for (int arrow_idx : cell_to_arrows_[cell]) {
            const auto& arrow = definition_.arrows[arrow_idx];
            if (arrow.circle_cell < 0 || arrow.path_cells.empty()) continue;

            std::bitset<46> sums;
            sums.set(0);
            for (int ac : arrow.path_cells) {
                std::bitset<46> next;
                for (int s = 0; s <= 45; ++s) {
                    if (!sums.test(s)) continue;
                    for (int d = 1; d <= 9; ++d) {
                        if ((state[ac] & digit_to_mask(d)) == 0) continue;
                        if (s + d <= 45) next.set(s + d);
                    }
                }
                sums = next;
            }

            uint16_t remove_circle = 0;
            for (int d = 1; d <= 9; ++d) {
                uint16_t bit = digit_to_mask(d);
                if ((state[arrow.circle_cell] & bit) == 0) continue;
                if (!sums.test(d)) remove_circle |= bit;
            }
            if (!remove_state(arrow.circle_cell, remove_circle)) return false;

            for (int i = 0; i < static_cast<int>(arrow.path_cells.size()); ++i) {
                int ac = arrow.path_cells[i];
                std::bitset<46> other;
                other.set(0);
                for (int j = 0; j < static_cast<int>(arrow.path_cells.size()); ++j) {
                    if (j == i) continue;
                    int oc = arrow.path_cells[j];
                    std::bitset<46> next;
                    for (int s = 0; s <= 45; ++s) {
                        if (!other.test(s)) continue;
                        for (int d = 1; d <= 9; ++d) {
                            if ((state[oc] & digit_to_mask(d)) == 0) continue;
                            if (s + d <= 45) next.set(s + d);
                        }
                    }
                    other = next;
                }

                uint16_t remove = 0;
                for (int v = 1; v <= 9; ++v) {
                    uint16_t bit = digit_to_mask(v);
                    if ((state[ac] & bit) == 0) continue;
                    bool supported = false;
                    for (int cv = 1; cv <= 9; ++cv) {
                        if ((state[arrow.circle_cell] & digit_to_mask(cv)) == 0) continue;
                        int need = cv - v;
                        if (need >= 0 && need <= 45 && other.test(need)) {
                            supported = true;
                            break;
                        }
                    }
                    if (!supported) remove |= bit;
                }
                if (!remove_state(ac, remove)) return false;
            }
        }

        for (const auto& [thermo_idx, pos] : cell_to_thermo_pos_[cell]) {
            const auto& thermo = definition_.thermos[thermo_idx];
            const int c = thermo.cells[pos];
            const int len = static_cast<int>(thermo.cells.size());
            const int lo = pos + 1;
            const int hi = 9 - (len - 1 - pos);
            uint16_t remove = 0;
            for (int d = 1; d <= 9; ++d) {
                if ((d < lo || d > hi) && (state[c] & digit_to_mask(d))) {
                    remove |= digit_to_mask(d);
                }
            }
            if (!remove_state(c, remove)) return false;
        }
    }

    for (uint16_t d : state) {
        if (d == 0) return false;
    }
    return true;
}

} // namespace sudoku
