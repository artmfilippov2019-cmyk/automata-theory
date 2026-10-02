#include "interpreter.hpp"
#include "maze.hpp"
#include "robot.hpp"
#include "visualizer.hpp"

#include <iostream>
#include <string>

static constexpr const char* kDefaultMaze = "mazes/maze2.txt";
static constexpr const char* kDefaultProgram = "programs/find_exit.txt";
static constexpr int kDelayMs = 80;

static std::string normalizeMazePath(const std::string& input) {
    if (input.empty()) return kDefaultMaze;

    std::string path = input;
    if (path.find('/') == std::string::npos && path.find('\\') == std::string::npos) {
        if (path.find('.') == std::string::npos) path += ".txt";
        path = "mazes/" + path;
    }
    return path;
}

static std::string askMazeFile() {
    std::cout << "Maze file (e.g. maze2.txt, Enter = maze2.txt): ";
    std::cout.flush();

    std::string line;
    if (!std::getline(std::cin, line))
        return kDefaultMaze;

    return normalizeMazePath(line);
}

int main() {
    try {
        const std::string maze_file = askMazeFile();

        Maze maze = Maze::loadFromFile(maze_file);
        Robot robot(maze);
        Interpreter interpreter(maze, robot);
        g_interpreter = &interpreter;

        Visualizer visualizer;
        visualizer.setDelayMs(kDelayMs);
        g_visualizer = &visualizer;

        std::cout << "Maze " << maze.width << "x" << maze.height
                  << ", robot at (" << maze.robot_x << ", " << maze.robot_y << ")\n";
        std::cout << "Running program: " << kDefaultProgram
                  << " [visual mode, delay " << kDelayMs << " ms]\n";
        std::cout.flush();

        visualizer.render(maze, robot, "Program start");
        interpreter.run(kDefaultProgram);
        visualizer.render(maze, robot, robot.escaped() ? "Program finished: escaped"
                                                       : "Program finished");

        if (robot.escaped()) {
            std::cout << "Robot escaped the maze at (" << robot.x() << ", " << robot.y() << ")!\n";
            return 0;
        }

        std::cout << "Program finished. Robot at (" << robot.x() << ", " << robot.y() << ").\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
