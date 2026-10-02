#pragma once

#include "symbol_table.hpp"

#include <string>
#include <vector>

enum class CastDirection { NONE, LEFT, RIGHT };

struct Expr {
    enum Kind {
        LIT,
        VAR,
        INDEX,
        SLICE,
        BRACKET_ID,
        UNARY,
        BINARY,
        COMPARE,
        ASSIGN,
        CALL
    } kind = LIT;

    Value lit;
    std::string name;
    std::string name2;
    char op = 0;
    bool left_dot = false;
    bool right_dot = false;
    Expr* lhs = nullptr;
    Expr* rhs = nullptr;
    std::vector<Expr*> elems;
    bool voice_parens = false;

    explicit Expr(Kind k) : kind(k) {}
    Expr() = default;
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;

    ~Expr() {
        delete lhs;
        delete rhs;
        for (auto* e : elems) delete e;
    }
};

struct Stmt {
    enum Kind {
        DECL,
        RECORD_DECL,
        ASSIGN,
        CALL,
        BLOCK,
        LOOP,
        PROC_DEF
    } kind = ASSIGN;

    Expr* expr = nullptr;
    Expr* cond = nullptr;
    std::vector<Stmt*> body;
    std::string name;
    std::string type_name;
    BaseType decl_type = BaseType::UNDEF;
    int array_size = -1;
    std::vector<ParamInfo> params;
    std::shared_ptr<RecordTypeDef> record_def;

    explicit Stmt(Kind k) : kind(k) {}
    Stmt() = default;
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;

    ~Stmt() {
        delete expr;
        delete cond;
        for (auto* s : body) delete s;
    }
};

struct Program {
    std::vector<Stmt*> stmts;
    Program() = default;
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;
    ~Program() {
        for (auto* s : stmts) delete s;
    }
};

extern Program* g_program;