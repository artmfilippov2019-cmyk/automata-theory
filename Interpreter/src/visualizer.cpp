#include "visualizer.hpp"

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

Visualizer* g_visualizer = nullptr;

void Visualizer::render(const Maze& maze, const Robot& robot, const std::string& caption) {
    std::ostringstream out;
    out << "\033[2J\033[H";
    out << "=== Robot simulator (frame " << ++frame_ << ") ===\n";
    if (!caption.empty()) out << caption << '\n';
    out << "Position: (" << robot.x() << ", " << robot.y() << ")";
    if (robot.escaped()) out << "  [ESCAPED]";
    out << "\n\n";

    for (int y = 0; y < maze.height; ++y) {
        for (int x = 0; x < maze.width; ++x) {
            char ch;
            if (x == robot.x() && y == robot.y())
                ch = robot.escaped() ? 'X' : 'R';
            else {
                switch (maze.cellAt(x, y)) {
                    case CellType::Wall: ch = '#'; break;
                    case CellType::Exit: ch = 'E'; break;
                    default: ch = '.'; break;
                }
            }
            out << ch;
        }
        out << '\n';
    }

    out << "\n# wall   . path   E exit   R robot   X escaped\n";
    std::cout << out.str() << std::flush;

    if (delay_ms_ > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_));
}
