#pragma once

#include <string>
#include <vector>

enum class CellType { Empty, Wall, Exit };

struct ExitInfo {
    int x = 0, y = 0;
    std::string password;
};

struct WallPassword {
    int x = 0, y = 0;
    std::string password;
};

class Maze {
public:
    int width = 0;
    int height = 0;
    int robot_x = 0;
    int robot_y = 0;
    std::vector<ExitInfo> exits;
    std::vector<WallPassword> wall_passwords;

    static Maze loadFromFile(const std::string& path);
    CellType cellAt(int x, int y) const;
    bool isWalkable(int x, int y) const;
    const ExitInfo* exitAt(int x, int y) const;
    std::vector<std::string> visionPasswords(int x, int y) const;

private:
    std::vector<std::string> grid_;
};


