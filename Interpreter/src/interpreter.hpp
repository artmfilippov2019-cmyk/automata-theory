#pragma once

#include "ast.hpp"
#include "maze.hpp"
#include "robot.hpp"
#include "symbol_table.hpp"
#include "value.hpp"

#include <string>
#include <vector>

class Interpreter {
public:
    Interpreter(Maze& maze, Robot& robot);

    void run(const std::string& source_path);
    void runSource(const std::string& source);

    SymbolTable& symbols() { return symbols_; }
    Robot& robot() { return robot_; }

    Value evalExpr(Expr* e);
    void execStmt(Stmt* s);

    void assignLValue(LValue& lv, const Value& rhs);
    long long toIndex(const Value& v);
    LValue evalLValue(Expr* e);

    ConvFn procConverter();
    void callConversion(const std::string& name, Value& a, Value& b);

private:
    Maze& maze_;
    Robot& robot_;
    SymbolTable symbols_;

    void execProcBody(const Procedure& proc, const std::vector<CallArg>& args);
    bool isSysProc(const std::string& name) const;
    Value callSysProc(const std::string& name, std::vector<CallArg>& args);
    Value evalCall(Expr* e);
    CallArg evalArg(Expr* e, bool want_ref);
    void registerProc(Stmt* s);
};

extern Interpreter* g_interpreter;
extern std::string g_parse_error;

int yyparse();
void lexer_set_input(const std::string& src);