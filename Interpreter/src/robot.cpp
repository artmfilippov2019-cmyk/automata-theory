#include "robot.hpp"
#include "visualizer.hpp"

Robot::Robot(Maze& maze) : maze_(maze) {}

long long Robot::move(int dx, int dy, long long steps) {
    long long failed = 0;
    for (long long i = 0; i < steps; ++i) {
        int nx = maze_.robot_x + dx;
        int ny = maze_.robot_y + dy;
        if (!maze_.isWalkable(nx, ny)) {
            failed = steps - i;
            break;
        }
        maze_.robot_x = nx;
        maze_.robot_y = ny;

        if (g_visualizer) {
            const char* dir = "?";
            if (dx == 0 && dy == -1) dir = "UP";
            else if (dx == 0 && dy == 1) dir = "DOWN";
            else if (dx == 1 && dy == 0) dir = "RIGHT";
            else if (dx == -1 && dy == 0) dir = "LEFT";
            g_visualizer->render(maze_, *this, std::string("MOVE ") + dir);
        }
    }
    return failed;
}

long long Robot::moveUp(long long steps) { return move(0, -1, steps); }
long long Robot::moveDown(long long steps) { return move(0, 1, steps); }
long long Robot::moveRight(long long steps) { return move(1, 0, steps); }
long long Robot::moveLeft(long long steps) { return move(-1, 0, steps); }

Value Robot::ping(int dx, int dy, const Value& mode) const {
    int search_mode = -1;
    if (mode.isUndef()) search_mode = 2;
    else if (mode.type == BaseType::NUMERIC) search_mode = static_cast<int>(mode.num);
    else return Value::makeUndef();

    int cx = maze_.robot_x;
    int cy = maze_.robot_y;
    long long dist = 0;

    for (int guard = 0; guard < maze_.width + maze_.height + 2; ++guard) {
        cx += dx;
        cy += dy;
        if (cx < 0 || cy < 0 || cx >= maze_.width || cy >= maze_.height) {
            if (search_mode == 1) return Value::makeNumeric(dist + 1);
            if (search_mode == 2) return Value::makeNumeric(dist + 1);
            return Value::makeUndef();
        }
        dist++;
        bool hit = false;
        if (search_mode == 0) hit = maze_.cellAt(cx, cy) == CellType::Exit;
        else if (search_mode == 1) hit = maze_.cellAt(cx, cy) == CellType::Wall;
        else hit = maze_.cellAt(cx, cy) != CellType::Empty;

        if (hit) return Value::makeNumeric(dist);
    }
    return Value::makeUndef();
}

Value Robot::pingUp(const Value& mode) { return ping(0, -1, mode); }
Value Robot::pingDown(const Value& mode) { return ping(0, 1, mode); }
Value Robot::pingRight(const Value& mode) { return ping(1, 0, mode); }
Value Robot::pingLeft(const Value& mode) { return ping(-1, 0, mode); }

Value Robot::vision() {
    auto pwds = maze_.visionPasswords(maze_.robot_x, maze_.robot_y);
    std::vector<Value> arr(8, Value::makeString(""));
    for (size_t i = 0; i < pwds.size() && i < arr.size(); ++i)
        arr[i] = Value::makeString(pwds[i]);
    return Value::makeArray(std::move(arr), BaseType::STRING);
}

void Robot::voice(const std::string& password) {
    const auto* exit = maze_.exitAt(maze_.robot_x, maze_.robot_y);
    if (!exit) return;
    if (exit->password.empty() || exit->password == password)
        escaped_ = true;

    if (g_visualizer) {
        std::string cap = "VOICE: \"" + password + "\"";
        if (escaped_) cap += " -> exit opened!";
        g_visualizer->render(maze_, *this, cap);
    }
}
