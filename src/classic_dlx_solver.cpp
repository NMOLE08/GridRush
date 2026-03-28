#include "classic_dlx_solver.h"

#include <array>
#include <sstream>
#include <string>
#include <vector>

namespace sudoku {

namespace {

constexpr int kColumnCount = 324;

inline int cell_constraint(int r, int c) { return r * 9 + c; }
inline int row_digit_constraint(int r, int d) { return 81 + r * 9 + (d - 1); }
inline int col_digit_constraint(int c, int d) { return 162 + c * 9 + (d - 1); }
inline int box_digit_constraint(int b, int d) { return 243 + b * 9 + (d - 1); }

struct RowInfo {
    int r = 0;
    int c = 0;
    int d = 0;
};

struct Node {
    int left = 0;
    int right = 0;
    int up = 0;
    int down = 0;
    int col = 0;
    int row_id = -1;
};

struct Dlx {
    std::vector<Node> nodes;
    std::array<int, kColumnCount + 1> col_size{};
    std::vector<RowInfo> row_infos;
    std::vector<int> partial;
    std::vector<int> first_solution;
    std::vector<std::string>* trace = nullptr;
    int solution_count = 0;
    bool hit_limit = false;
    int limit = 2;

    Dlx() {
        nodes.reserve(1 + (kColumnCount + 1) + 3000);
        nodes.push_back(Node{}); // root at 0

        for (int c = 1; c <= kColumnCount; ++c) {
            Node header;
            header.left = c - 1;
            header.right = (c == kColumnCount) ? 0 : (c + 1);
            header.up = c;
            header.down = c;
            header.col = c;
            nodes.push_back(header);
        }
        nodes[0].left = kColumnCount;
        nodes[0].right = 1;
        col_size.fill(0);
    }

    void add_row(int r, int c, int d) {
        const int box = (r / 3) * 3 + (c / 3);
        std::array<int, 4> cols = {
            cell_constraint(r, c) + 1,
            row_digit_constraint(r, d) + 1,
            col_digit_constraint(c, d) + 1,
            box_digit_constraint(box, d) + 1
        };

        const int row_id = static_cast<int>(row_infos.size());
        row_infos.push_back(RowInfo{r, c, d});

        int first = -1;
        int prev = -1;
        for (int col : cols) {
            Node n;
            n.col = col;
            n.row_id = row_id;
            n.up = nodes[col].up;
            n.down = col;

            const int idx = static_cast<int>(nodes.size());
            nodes.push_back(n);

            nodes[n.up].down = idx;
            nodes[col].up = idx;
            ++col_size[col];

            if (first < 0) {
                first = idx;
                prev = idx;
                nodes[idx].left = idx;
                nodes[idx].right = idx;
            } else {
                nodes[idx].left = prev;
                nodes[idx].right = first;
                nodes[prev].right = idx;
                nodes[first].left = idx;
                prev = idx;
            }
        }
    }

    void cover(int col) {
        nodes[nodes[col].right].left = nodes[col].left;
        nodes[nodes[col].left].right = nodes[col].right;

        for (int r = nodes[col].down; r != col; r = nodes[r].down) {
            for (int j = nodes[r].right; j != r; j = nodes[j].right) {
                nodes[nodes[j].down].up = nodes[j].up;
                nodes[nodes[j].up].down = nodes[j].down;
                --col_size[nodes[j].col];
            }
        }
    }

    void uncover(int col) {
        for (int r = nodes[col].up; r != col; r = nodes[r].up) {
            for (int j = nodes[r].left; j != r; j = nodes[j].left) {
                ++col_size[nodes[j].col];
                nodes[nodes[j].down].up = j;
                nodes[nodes[j].up].down = j;
            }
        }

        nodes[nodes[col].right].left = col;
        nodes[nodes[col].left].right = col;
    }

    int choose_column() const {
        int best = nodes[0].right;
        int best_size = 1e9;
        for (int c = nodes[0].right; c != 0; c = nodes[c].right) {
            if (col_size[c] < best_size) {
                best_size = col_size[c];
                best = c;
            }
        }
        return best;
    }

    static std::string indent(int depth) {
        return std::string(static_cast<size_t>(depth * 2), ' ');
    }

    std::string describe_row(int row_id) const {
        const RowInfo& info = row_infos[row_id];
        std::ostringstream oss;
        oss << "R" << (info.r + 1) << "C" << (info.c + 1) << "=" << info.d;
        return oss.str();
    }

    void log_step(int depth, const std::string& message) {
        if (!trace) return;
        trace->push_back(indent(depth) + message);
    }

    void search(int depth) {
        if (solution_count >= limit) {
            hit_limit = true;
            return;
        }

        if (nodes[0].right == 0) {
            log_step(depth, "Exact cover complete: one solution found.");
            ++solution_count;
            if (first_solution.empty()) {
                first_solution = partial;
            }
            if (solution_count >= limit) {
                hit_limit = true;
            }
            return;
        }

        const int col = choose_column();
        if (col_size[col] == 0) {
            log_step(depth, "Dead end: selected constraint column has zero candidates. Backtrack.");
            return;
        }

        {
            std::ostringstream oss;
            oss << "Choose constraint column C" << col << " (candidate rows=" << col_size[col] << ").";
            log_step(depth, oss.str());
        }

        cover(col);
        log_step(depth, "Cover selected column and iterate candidate rows.");
        for (int r = nodes[col].down; r != col; r = nodes[r].down) {
            const int row_id = nodes[r].row_id;
            log_step(depth, "Try row " + describe_row(row_id) + ".");
            partial.push_back(nodes[r].row_id);
            for (int j = nodes[r].right; j != r; j = nodes[j].right) {
                {
                    std::ostringstream oss;
                    oss << "Cover linked constraint column C" << nodes[j].col << ".";
                    log_step(depth + 1, oss.str());
                }
                cover(nodes[j].col);
            }

            search(depth + 1);

            for (int j = nodes[r].left; j != r; j = nodes[j].left) {
                {
                    std::ostringstream oss;
                    oss << "Uncover linked constraint column C" << nodes[j].col << ".";
                    log_step(depth + 1, oss.str());
                }
                uncover(nodes[j].col);
            }
            partial.pop_back();

            log_step(depth, "Backtrack from row " + describe_row(row_id) + ".");

            if (solution_count >= limit) {
                log_step(depth, "Stop search: solution limit reached.");
                break;
            }
        }
        uncover(col);
        log_step(depth, "Uncover selected constraint column and return.");
    }
};

bool given_digit(const PuzzleDefinition& puzzle, int r, int c, int* out_digit) {
    const uint16_t mask = puzzle.givens[to_cell(r, c)];
    if (mask == 0) return false;
    for (int d = 1; d <= 9; ++d) {
        if (mask == digit_to_mask(d)) {
            *out_digit = d;
            return true;
        }
    }
    return false;
}

} // namespace

bool is_classic_only(const PuzzleDefinition& puzzle) {
    return puzzle.even_cells.empty()
        && puzzle.odd_cells.empty()
        && puzzle.killer_cages.empty()
        && puzzle.thermos.empty()
        && puzzle.arrows.empty()
        && puzzle.kropki_dots.empty();
}

ClassicDlxResult solve_classic_with_dlx(const PuzzleDefinition& puzzle) {
    ClassicDlxResult result;
    result.solved_grid.fill(0);
    result.logs.push_back("Algorithm: Classic Sudoku solved with Algorithm X (Dancing Links).");

    if (!is_classic_only(puzzle)) {
        result.logs.push_back("DLX solver only handles classic constraints.");
        return result;
    }

    Dlx dlx;
    dlx.trace = &result.logs;
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            int fixed = 0;
            if (given_digit(puzzle, r, c, &fixed)) {
                dlx.add_row(r, c, fixed);
            } else {
                for (int d = 1; d <= 9; ++d) {
                    dlx.add_row(r, c, d);
                }
            }
        }
    }

    const bool wants_all_solution_count = puzzle.count_all_solutions;
    const bool wants_uniqueness = puzzle.check_uniqueness || wants_all_solution_count;
    const int configured_limit = (puzzle.max_solution_count > 0) ? puzzle.max_solution_count : 5000;
    dlx.limit = wants_all_solution_count ? configured_limit : (wants_uniqueness ? 2 : 1);
    result.logs.push_back("DLX initialization complete. Start Algorithm X search.");
    dlx.search(0);

    if (dlx.solution_count == 0 || dlx.first_solution.empty()) {
        result.logs.push_back("DLX search found no exact cover solution.");
        return result;
    }

    for (int row_id : dlx.first_solution) {
        const RowInfo info = dlx.row_infos[row_id];
        result.solved_grid[to_cell(info.r, info.c)] = info.d;
    }

    result.solved = true;
    result.uniqueness_checked = wants_uniqueness;
    result.unique = wants_uniqueness && (dlx.solution_count == 1);
    if (wants_all_solution_count) {
        result.solution_count = dlx.solution_count;
        result.solution_count_complete = !dlx.hit_limit;
    }

    if (wants_all_solution_count) {
        std::ostringstream oss;
        oss << "DLX counted " << dlx.solution_count << " solution(s)";
        if (dlx.hit_limit) {
            oss << " and hit the configured cap of " << dlx.limit << ".";
        } else {
            oss << " with full count completion.";
        }
        result.logs.push_back(oss.str());
    } else if (wants_uniqueness) {
        std::ostringstream oss;
        oss << "DLX explored exact cover and found " << dlx.solution_count
            << (dlx.solution_count == 1 ? " solution (unique within limit)." : " solutions (not unique).");
        result.logs.push_back(oss.str());
    } else {
        result.logs.push_back("Uniqueness check skipped by configuration.");
    }
    return result;
}

} // namespace sudoku
