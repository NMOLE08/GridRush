#pragma once

#include "constraints.h"

#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sudoku {

class LogTranslator {
public:
    enum class ConstraintKind {
        Cell,
        RowDigit,
        ColumnDigit,
        BoxDigit
    };

    struct ConstraintRef {
        ConstraintKind kind = ConstraintKind::Cell;
        int row = -1;
        int col = -1;
        int box = -1;
        int digit = -1;
    };

    static std::string cell_label(int row, int col) {
        std::ostringstream oss;
        oss << "R" << (row + 1) << "C" << (col + 1);
        return oss.str();
    }

    static std::string cell_label(int cell) {
        return cell_label(row_of(cell), col_of(cell));
    }

    static std::string candidate_list(uint16_t domain) {
        std::ostringstream oss;
        oss << "[";
        bool first = true;
        for (int d = 1; d <= 9; ++d) {
            if ((domain & digit_to_mask(d)) == 0) continue;
            if (!first) {
                oss << ", ";
            }
            oss << d;
            first = false;
        }
        oss << "]";
        return oss.str();
    }

    static ConstraintRef decode_constraint_column(int one_based_col) {
        const int idx = one_based_col - 1;
        if (idx >= 0 && idx < 81) {
            return ConstraintRef{ConstraintKind::Cell, idx / 9, idx % 9, -1, -1};
        }
        if (idx >= 81 && idx < 162) {
            const int t = idx - 81;
            return ConstraintRef{ConstraintKind::RowDigit, t / 9, -1, -1, (t % 9) + 1};
        }
        if (idx >= 162 && idx < 243) {
            const int t = idx - 162;
            return ConstraintRef{ConstraintKind::ColumnDigit, -1, t / 9, -1, (t % 9) + 1};
        }
        const int t = idx - 243;
        return ConstraintRef{ConstraintKind::BoxDigit, -1, -1, t / 9, (t % 9) + 1};
    }

    static std::string classic_requirement_label(ConstraintKind kind) {
        switch (kind) {
            case ConstraintKind::RowDigit:
                return "Row";
            case ConstraintKind::ColumnDigit:
                return "Column";
            case ConstraintKind::BoxDigit:
                return "Box";
            case ConstraintKind::Cell:
            default:
                return "Cell";
        }
    }

    static std::string translate_choose_column(int one_based_col, int candidate_rows) {
        const ConstraintRef ref = decode_constraint_column(one_based_col);
        std::ostringstream oss;
        switch (ref.kind) {
            case ConstraintKind::Cell:
                oss << "Cell " << cell_label(ref.row, ref.col)
                    << " has " << candidate_rows << " possible placement"
                    << (candidate_rows == 1 ? "" : "s") << " left.";
                break;
            case ConstraintKind::RowDigit:
                oss << "Row " << (ref.row + 1) << " still needs digit " << ref.digit
                    << "; " << candidate_rows << " location"
                    << (candidate_rows == 1 ? " is" : "s are") << " currently possible.";
                break;
            case ConstraintKind::ColumnDigit:
                oss << "Column " << (ref.col + 1) << " still needs digit " << ref.digit
                    << "; " << candidate_rows << " location"
                    << (candidate_rows == 1 ? " is" : "s are") << " currently possible.";
                break;
            case ConstraintKind::BoxDigit:
                oss << "Box " << (ref.box + 1) << " still needs digit " << ref.digit
                    << "; " << candidate_rows << " location"
                    << (candidate_rows == 1 ? " is" : "s are") << " currently possible.";
                break;
        }
        return oss.str();
    }

    static std::string translate_try_row(int row, int col, int digit, ConstraintKind trigger) {
        std::ostringstream oss;
        oss << "Placing " << digit << " in " << cell_label(row, col)
            << " to satisfy the " << classic_requirement_label(trigger) << " requirement.";
        return oss.str();
    }

    static std::string translate_linked_constraint(int one_based_col) {
        const ConstraintRef ref = decode_constraint_column(one_based_col);
        std::ostringstream oss;
        switch (ref.kind) {
            case ConstraintKind::Cell:
                oss << "Marking " << cell_label(ref.row, ref.col) << " as resolved for this branch.";
                break;
            case ConstraintKind::RowDigit:
                oss << "Removing " << ref.digit << " as a candidate from remaining cells in Row "
                    << (ref.row + 1) << ".";
                break;
            case ConstraintKind::ColumnDigit:
                oss << "Removing " << ref.digit << " as a candidate from remaining cells in Column "
                    << (ref.col + 1) << ".";
                break;
            case ConstraintKind::BoxDigit:
                oss << "Removing " << ref.digit << " as a candidate from remaining cells in Box "
                    << (ref.box + 1) << ".";
                break;
        }
        return oss.str();
    }

    static std::string translate_backtrack_row(int row, int col, int digit) {
        std::ostringstream oss;
        oss << "Placement " << digit << " at " << cell_label(row, col)
            << " leads to a contradiction later. Reverting this branch.";
        return oss.str();
    }

    static std::string translate_solution_found() {
        return "All requirements are satisfied. One complete solution is found.";
    }

    static std::string translate_dead_end() {
        return "No legal placements satisfy the current requirement. Reverting this branch.";
    }

    static std::string translate_search_start() {
        return "Classic Sudoku analysis started. Evaluating valid placements.";
    }

    static std::string translate_no_solution() {
        return "No valid Sudoku solution satisfies all constraints.";
    }

    static std::string translate_solver_scope() {
        return "This fast exact-cover path is available only for pure classic Sudoku constraints.";
    }

    static std::string translate_count_summary(int solution_count, bool hit_limit, int limit) {
        std::ostringstream oss;
        oss << "Classic analysis found " << solution_count << " solution";
        if (solution_count != 1) {
            oss << "s";
        }
        if (hit_limit) {
            oss << " before reaching the configured cap of " << limit << ".";
        } else {
            oss << " with full counting completed.";
        }
        return oss.str();
    }

    static std::string translate_uniqueness_summary(int solution_count) {
        std::ostringstream oss;
        if (solution_count == 1) {
            oss << "Exactly one valid solution exists for this puzzle.";
        } else {
            oss << "More than one valid solution exists for this puzzle.";
        }
        return oss.str();
    }

    static std::string translate_candidate_removal(const PuzzleDefinition& puzzle,
                                                   int cell,
                                                   int digit,
                                                   const std::string& reason,
                                                   uint16_t resulting_domain) {
        static const std::unordered_map<std::string_view, std::string_view> kReasonPrefix = {
            {"Classic Sudoku row/column/box distinctness", "classic"},
            {"Even/Odd marker (even-only)", "evenodd_even"},
            {"Even/Odd marker (odd-only)", "evenodd_odd"},
            {"Killer Cage no-repeat", "killer_norepeat"},
            {"Killer Cage sum constraint target ", "killer_sum"},
            {"Thermo increasing path constraint", "thermo_link"},
            {"Thermo positional bound constraint", "thermo_bound"},
            {"Arrow circle equals path sum constraint", "arrow_circle"},
            {"Arrow path-to-circle sum constraint", "arrow_path"},
            {"Kropki black-dot doubling constraint", "kropki_black"},
            {"Kropki white-dot consecutive constraint", "kropki_white"}
        };

        std::string_view tag = "";
        for (const auto& entry : kReasonPrefix) {
            if (reason.rfind(std::string(entry.first), 0) == 0) {
                tag = entry.second;
                break;
            }
        }

        const std::string cell_name = cell_label(cell);

        if (tag == "classic") {
            const std::string source = parse_after_token(reason, "from ");
            const int placed_digit = parse_trailing_integer(source, "=");
            std::string source_cell = source;
            const std::size_t eq = source_cell.find('=');
            if (eq != std::string::npos) {
                source_cell = source_cell.substr(0, eq);
            }
            std::ostringstream oss;
            oss << "Placing " << (placed_digit > 0 ? placed_digit : digit)
                << " in " << (source_cell.empty() ? cell_name : source_cell)
                << " to satisfy the Row/Column/Box requirement.";
            return oss.str();
        }

        if (tag == "evenodd_even") {
            std::ostringstream oss;
            oss << cell_name
                << " is a Square, so it must be an Even number. Restricting options to "
                << candidate_list(resulting_domain) << ".";
            return oss.str();
        }

        if (tag == "evenodd_odd") {
            std::ostringstream oss;
            oss << cell_name
                << " is a Circle, so it must be an Odd number. Restricting options to "
                << candidate_list(resulting_domain) << ".";
            return oss.str();
        }

        if (tag == "killer_norepeat" || tag == "killer_sum") {
            const int target = parse_trailing_integer(reason, "target ");
            const int fallback_target = parse_trailing_integer(reason, "sum ");
            std::ostringstream oss;
            oss << "The cage at " << cell_name;
            if (target > 0) {
                oss << " requires a sum of " << target;
            } else if (fallback_target > 0) {
                oss << " requires a sum of " << fallback_target;
            } else {
                oss << " has a fixed target sum";
            }
            oss << ". Candidate " << digit << " would exceed this sum. Removing " << digit << ".";
            return oss.str();
        }

        if (tag == "thermo_link" || tag == "thermo_bound") {
            const int prev_digit = parse_trailing_integer(reason, "=");
            std::ostringstream oss;
            oss << cell_name << " is further up the thermometer than the previous cell. ";
            if (prev_digit > 0) {
                oss << "It must be strictly greater than " << prev_digit << ". ";
            } else {
                oss << "It must be strictly increasing. ";
            }
            oss << "Removing lower candidates.";
            return oss.str();
        }

        if (tag == "arrow_circle" || tag == "arrow_path") {
            const int target = parse_trailing_integer(reason, "max circle ");
            std::ostringstream oss;
            oss << "Candidates in arrow path exceed the maximum possible circle value of "
                << (target > 0 ? target : 9) << ". Removing high-value candidates.";
            return oss.str();
        }

        if (tag == "kropki_black" || tag == "kropki_white") {
            const std::string peer = parse_after_token(reason, "with ");
            std::ostringstream oss;
            oss << "There is a " << (tag == "kropki_black" ? "Black" : "White")
                << " dot between " << cell_name;
            if (!peer.empty()) {
                oss << " and " << peer;
            }
            oss << ". Valid options are restricted to "
                << (tag == "kropki_black" ? "Ratio" : "Consecutive") << " values.";
            return oss.str();
        }

        if (!puzzle.category.empty()) {
            std::ostringstream oss;
            oss << cell_name << " candidate " << digit
                << " removed after applying " << puzzle.category << " constraints.";
            return oss.str();
        }

        std::ostringstream fallback;
        fallback << cell_name << " candidate " << digit << " removed by constraint propagation.";
        return fallback.str();
    }

private:
    static int parse_trailing_integer(const std::string& text, const std::string& token) {
        const std::size_t pos = text.rfind(token);
        if (pos == std::string::npos) return -1;
        std::size_t start = pos + token.size();
        while (start < text.size() && text[start] == ' ') {
            ++start;
        }
        int value = 0;
        bool has_digits = false;
        for (std::size_t i = start; i < text.size(); ++i) {
            if (text[i] < '0' || text[i] > '9') {
                break;
            }
            has_digits = true;
            value = (value * 10) + (text[i] - '0');
        }
        return has_digits ? value : -1;
    }

    static std::string parse_after_token(const std::string& text, const std::string& token) {
        const std::size_t pos = text.rfind(token);
        if (pos == std::string::npos) {
            return "";
        }
        std::string out = text.substr(pos + token.size());
        while (!out.empty() && (out.back() == '.' || out.back() == ' ')) {
            out.pop_back();
        }
        return out;
    }
};

} // namespace sudoku
