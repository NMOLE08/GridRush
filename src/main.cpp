#include "Solver.h"

#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        return 1;
    }

    const std::string input_path = argv[1];
    const std::string output_path = (argc >= 3) ? argv[2] : "answer.json";

    sudoku::BatchSolver solver;
    return solver.solve_file(input_path, output_path) ? 0 : 1;
}
