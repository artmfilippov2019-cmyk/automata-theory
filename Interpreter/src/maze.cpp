#include "maze.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

static void rtrim(std::string& s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ' || s.back() == '\t'))
        s.pop_back();
}

Maze Maze::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open maze file: " + path);

    Maze m;
    std::string line;

    if (!std::getline(in, line)) throw std::runtime_error("Empty maze file");
    rtrim(line);
    {
        std::istringstream iss(line);
        iss >> m.width >> m.height;
    }

    if (!std::getline(in, line)) throw std::runtime_error("Missing robot position");
    rtrim(line);
    {
        std::istringstream iss(line);
        iss >> m.robot_x >> m.robot_y;
    }

    int exit_count = 0;
    if (std::getline(in, line)) {
        rtrim(line);
        std::istringstream iss(line);
        iss >> exit_count;
    }

    for (int i = 0; i < exit_count && std::getline(in, line); ++i) {
        rtrim(line);
        ExitInfo e;
        std::istringstream iss(line);
        iss >> e.x >> e.y;
        std::getline(iss, e.password);
        if (!e.password.empty() && e.password[0] == ' ') e.password.erase(0, 1);
        rtrim(e.password);
        m.exits.push_back(std::move(e));
    }

    int pwd_count = 0;
    if (std::getline(in, line)) {
        rtrim(line);
        std::istringstream iss(line);
        iss >> pwd_count;
    }

    for (int i = 0; i < pwd_count && std::getline(in, line); ++i) {
        rtrim(line);
        WallPassword wp;
        std::istringstream iss(line);
        iss >> wp.x >> wp.y;
        std::getline(iss, wp.password);
        if (!wp.password.empty() && wp.password[0] == ' ') wp.password.erase(0, 1);
        rtrim(wp.password);
        m.wall_passwords.push_back(std::move(wp));
    }

    while (std::getline(in, line)) {
        rtrim(line);
        if (!line.empty()) m.grid_.push_back(line);
    }

    if (static_cast<int>(m.grid_.size()) != m.height)
        throw std::runtime_error("Maze height mismatch");

    return m;
}

CellType Maze::cellAt(int x, int y) const {
    if (x < 0 || y < 0 || x >= width || y >= height) return CellType::Wall;
    if (y >= static_cast<int>(grid_.size())) return CellType::Wall;
    char c = grid_[y][x];
    if (c == '#') return CellType::Wall;
    if (c == 'E' || c == 'e') return CellType::Exit;
    return CellType::Empty;
}

bool Maze::isWalkable(int x, int y) const {
    auto t = cellAt(x, y);
    return t == CellType::Empty || t == CellType::Exit;
}

const ExitInfo* Maze::exitAt(int x, int y) const {
    for (const auto& e : exits) {
        if (e.x == x && e.y == y) return &e;
    }
    if (cellAt(x, y) == CellType::Exit) {
        static ExitInfo default_exit;
        default_exit.x = x;
        default_exit.y = y;
        default_exit.password = "";
        return &default_exit;
    }
    return nullptr;
}

std::vector<std::string> Maze::visionPasswords(int x, int y) const {
    std::vector<std::string> result;
    for (const auto& wp : wall_passwords) {
        if (wp.password.empty()) continue;
        const bool neighbor =
            (wp.x == x && (wp.y == y - 1 || wp.y == y + 1)) ||
            (wp.y == y && (wp.x == x - 1 || wp.x == x + 1));
        if (neighbor)
            result.push_back(wp.password);
    }
    return result;
}