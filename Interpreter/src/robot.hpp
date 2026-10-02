#pragma once

#include "maze.hpp"
#include "value.hpp"

class Robot {
public:
    explicit Robot(Maze& maze);

    long long moveUp(long long steps);
    long long moveDown(long long steps);
    long long moveRight(long long steps);
    long long moveLeft(long long steps);

    Value pingUp(const Value& mode);
    Value pingDown(const Value& mode);
    Value pingRight(const Value& mode);
    Value pingLeft(const Value& mode);

    Value vision();
    void voice(const std::string& password);

    int x() const { return maze_.robot_x; }
    int y() const { return maze_.robot_y; }
    bool escaped() const { return escaped_; }

private:
    Maze& maze_;
    bool escaped_ = false;

    long long move(int dx, int dy, long long steps);
    Value ping(int dx, int dy, const Value& mode) const;
};