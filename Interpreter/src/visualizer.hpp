#pragma once

#include "maze.hpp"
#include "robot.hpp"

#include <string>

class Visualizer {
public:
    void setDelayMs(int ms) { delay_ms_ = ms; }
    int delayMs() const { return delay_ms_; }

    void render(const Maze& maze, const Robot& robot, const std::string& caption = "");

private:
    int delay_ms_ = 120;
    unsigned long long frame_ = 0;
};

extern Visualizer* g_visualizer;
