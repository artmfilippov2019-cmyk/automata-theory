#include "interpreter.hpp"

#include <fstream>
#include <memory>
#include <sstream>

Interpreter* g_interpreter = nullptr;
std::string g_parse_error;

Interpreter::Interpreter(Maze& maze, Robot& robot)
    : maze_(maze), robot_(robot) {}

ConvFn Interpreter::procConverter() {
    return [this](const std::string& name, Value& a, Value& b) {
        callConversion(name, a, b);
    };
}

void Interpreter::callConversion(const std::string& name, Value& a, Value& b) {
    auto* proc = symbols_.getProcedure(name);
    if (!proc)
        throw std::runtime_error("Unknown conversion procedure: " + name);

    std::vector<CallArg> args(2);
    args[0].by_ref = true;
    args[0].lvalue.ptr = &a;
    args[0].lvalue.valid = true;
    args[0].lvalue.target_type = a.type;
    args[0].value = a;
    args[1].by_ref = true;
    args[1].lvalue.ptr = &b;
    args[1].lvalue.valid = true;
    args[1].lvalue.target_type = b.type;
    args[1].value = b;
    execProcBody(*proc, args);
}

long long Interpreter::toIndex(const Value& v) {
    if (v.isUndef()) return -1;
    Value num = convertValue(v, BaseType::NUMERIC, procConverter());
    if (num.isUndef()) return -1;
    return num.num;
}

LValue Interpreter::evalLValue(Expr* e) {
    if (!e) return {};
    switch (e->kind) {
        case Expr::VAR:
            return symbols_.resolveLValue(e->name);
        case Expr::INDEX: {
            Expr* idxe = e->lhs ? e->lhs : (e->elems.empty() ? nullptr : e->elems[0]);
            if (!idxe) return {};
            long long idx = toIndex(evalExpr(idxe));
            return symbols_.resolveArrayIndex(e->name, idx);
        }
        case Expr::BRACKET_ID:
            if (symbols_.isArrayVariable(e->name)) {
                long long idx = toIndex(symbols_.getVariable(e->name2));
                return symbols_.resolveArrayIndex(e->name, idx);
            }
            return symbols_.resolveField(e->name, e->name2);
        case Expr::SLICE:
            return {};
        default:
            return {};
    }
}

void Interpreter::assignLValue(LValue& lv, const Value& rhs) {
    if (!lv.valid || !lv.ptr)
        throw std::runtime_error("Invalid l-value: cannot assign to a constant or undefined location");

    if (lv.is_array && rhs.isArray()) {
        std::vector<Value> out;
        out.reserve(rhs.arr.size());
        for (const auto& el : rhs.arr) {
            if (lv.record_type)
                out.push_back(convertToRecord(el, lv.record_type, procConverter()));
            else if (lv.target_type != BaseType::UNDEF)
                out.push_back(convertValue(el, lv.target_type, procConverter()));
            else
                out.push_back(el.clone());
        }
        *lv.ptr = Value::makeArray(std::move(out), lv.target_type);
        return;
    }

    if (lv.record_type)
        *lv.ptr = convertToRecord(rhs, lv.record_type, procConverter());
    else if (lv.target_type != BaseType::UNDEF && lv.target_type != BaseType::RECORD)
        *lv.ptr = convertValue(rhs, lv.target_type, procConverter());
    else
        *lv.ptr = rhs.clone();
}

bool Interpreter::isSysProc(const std::string& name) const {
    return name == "MOVEUP" || name == "MOVEDOWN" || name == "MOVERIGHT" || name == "MOVELEFT" ||
           name == "PINGUP" || name == "PINGDOWN" || name == "PINGRIGHT" || name == "PINGLEFT" ||
           name == "VISION" || name == "VOICE";
}

CallArg Interpreter::evalArg(Expr* e, bool want_ref) {
    CallArg arg;
    if (want_ref) {
        arg.by_ref = true;
        arg.lvalue = evalLValue(e);
        if (!arg.lvalue.valid || !arg.lvalue.ptr)
            throw std::runtime_error("Cannot pass a constant or non-variable by reference");
        arg.value = *arg.lvalue.ptr;
    } else {
        arg.by_ref = false;
        arg.value = evalExpr(e);
    }
    return arg;
}

Value Interpreter::callSysProc(const std::string& name, std::vector<CallArg>& args) {
    if (name == "MOVEUP" || name == "MOVEDOWN" || name == "MOVERIGHT" || name == "MOVELEFT") {
        long long steps = 1;
        if (!args.empty()) {
            Value n = args[0].by_ref && args[0].lvalue.valid
                          ? *args[0].lvalue.ptr
                          : convertValue(args[0].value, BaseType::NUMERIC, procConverter());
            if (!n.isUndef()) steps = n.num;
        }
        long long failed = 0;
        if (name == "MOVEUP") failed = robot_.moveUp(steps);
        else if (name == "MOVEDOWN") failed = robot_.moveDown(steps);
        else if (name == "MOVERIGHT") failed = robot_.moveRight(steps);
        else failed = robot_.moveLeft(steps);

        Value result = Value::makeNumeric(failed);
        if (!args.empty() && args[0].by_ref && args[0].lvalue.valid)
            assignLValue(args[0].lvalue, result);
        return result;
    }

    if (name == "PINGUP" || name == "PINGDOWN" || name == "PINGRIGHT" || name == "PINGLEFT") {
        Value mode = Value::makeUndef();
        if (!args.empty()) {
            mode = args[0].by_ref && args[0].lvalue.valid ? *args[0].lvalue.ptr : args[0].value;
        }
        Value dist;
        if (name == "PINGUP") dist = robot_.pingUp(mode);
        else if (name == "PINGDOWN") dist = robot_.pingDown(mode);
        else if (name == "PINGRIGHT") dist = robot_.pingRight(mode);
        else dist = robot_.pingLeft(mode);

        if (!args.empty() && args[0].by_ref && args[0].lvalue.valid)
            assignLValue(args[0].lvalue, dist);
        return dist;
    }

    if (name == "VISION") {
        Value result = robot_.vision();
        if (!args.empty() && args[0].by_ref && args[0].lvalue.valid)
            assignLValue(args[0].lvalue, result);
        return result;
    }

    if (name == "VOICE") {
        std::string pwd;
        if (!args.empty()) {
            Value s = args[0].by_ref && args[0].lvalue.valid
                          ? convertValue(*args[0].lvalue.ptr, BaseType::STRING, procConverter())
                          : convertValue(args[0].value, BaseType::STRING, procConverter());
            pwd = s.str;
        }
        robot_.voice(pwd);
        return Value::makeUndef();
    }

    return Value::makeUndef();
}

Value Interpreter::evalCall(Expr* e) {
    std::string name = e->name;
    if (isSysProc(name)) {
        bool first_ref = (name != "VOICE");
        std::vector<CallArg> args;
        for (size_t i = 0; i < e->elems.size(); ++i)
            args.push_back(evalArg(e->elems[i], first_ref && i == 0));
        return callSysProc(name, args);
    }

    auto* proc = symbols_.getProcedure(name);
    if (!proc)
        throw std::runtime_error("Unknown procedure: " + name);

    std::vector<CallArg> args;
    for (size_t i = 0; i < e->elems.size(); ++i) {
        bool by_ref = i < proc->params.size() && proc->params[i].by_ref;
        args.push_back(evalArg(e->elems[i], by_ref));
    }
    execProcBody(*proc, args);
    return Value::makeUndef();
}

void Interpreter::execProcBody(const Procedure& proc, const std::vector<CallArg>& args) {
    symbols_.pushScope();
    for (size_t i = 0; i < proc.params.size(); ++i) {
        const auto& p = proc.params[i];
        if (i < args.size() && p.by_ref && args[i].lvalue.valid && args[i].lvalue.ptr) {
            symbols_.declareVariableRef(p.name, args[i].lvalue.ptr);
            continue;
        }
        symbols_.declareVariable(p.name, p.type, -1, p.record_type);
        if (i < args.size()) {
            LValue lv = symbols_.resolveLValue(p.name);
            assignLValue(lv, args[i].by_ref && args[i].lvalue.valid ? *args[i].lvalue.ptr : args[i].value);
        }
    }
    for (auto* st : proc.body) {
        execStmt(st);
        if (robot_.escaped()) break;
    }
    symbols_.popScope();
}

void Interpreter::registerProc(Stmt* s) {
    for (auto& p : s->params) {
        if (p.type == BaseType::RECORD && !p.record_type) {
            auto it = symbols_.record_types.find(p.type_name);
            if (it == symbols_.record_types.end())
                throw std::runtime_error("Unknown type in PROC: " + p.type_name);
            p.record_type = it->second;
        }
    }
    Procedure proc;
    proc.name = s->name;
    proc.params = s->params;
    proc.body = s->body;
    symbols_.procedures[proc.name] = std::move(proc);
}

Value Interpreter::evalExpr(Expr* e) {
    if (!e) return Value::makeUndef();
    switch (e->kind) {
        case Expr::LIT:
            return e->lit.clone();
        case Expr::VAR:
            return symbols_.getVariable(e->name);
        case Expr::INDEX: {
            auto lv = evalLValue(e);
            return (lv.valid && lv.ptr) ? lv.ptr->clone() : Value::makeUndef();
        }
        case Expr::BRACKET_ID: {
            auto lv = evalLValue(e);
            return (lv.valid && lv.ptr) ? lv.ptr->clone() : Value::makeUndef();
        }
        case Expr::SLICE: {
            std::vector<long long> idxs;
            if (e->elems.size() == 1) {
                Value v = evalExpr(e->elems[0]);
                if (v.isArray()) {
                    for (const auto& el : v.arr) idxs.push_back(toIndex(el));
                } else {
                    idxs.push_back(toIndex(v));
                }
            } else {
                for (auto* x : e->elems) idxs.push_back(toIndex(evalExpr(x)));
            }
            auto* var = symbols_.findVar(e->name);
            if (!var) return Value::makeUndef();
            Value& container = var->ref ? *var->ref : var->value;
            std::vector<Value> slice;
            for (auto idx : idxs) {
                if (idx < 0 || static_cast<size_t>(idx) >= container.arr.size())
                    slice.push_back(Value::makeUndef());
                else
                    slice.push_back(container.arr[static_cast<size_t>(idx)].clone());
            }
            return Value::makeArray(std::move(slice), container.array_element_type);
        }
        case Expr::UNARY:
            return applyUnaryMinus(evalExpr(e->rhs));
        case Expr::BINARY:
            return applyBinary(evalExpr(e->lhs), evalExpr(e->rhs), e->op, e->left_dot, e->right_dot,
                               procConverter());
        case Expr::COMPARE:
            return applyCompare(evalExpr(e->lhs), evalExpr(e->rhs), e->op, e->left_dot, e->right_dot,
                                procConverter());
        case Expr::ASSIGN: {
            LValue lv = evalLValue(e->lhs);
            Value rhs = evalExpr(e->rhs);
            assignLValue(lv, rhs);
            return lv.valid && lv.ptr ? lv.ptr->clone() : Value::makeUndef();
        }
        case Expr::CALL:
            return evalCall(e);
    }
    return Value::makeUndef();
}

void Interpreter::execStmt(Stmt* s) {
    if (!s || robot_.escaped()) return;
    switch (s->kind) {
        case Stmt::DECL: {
            std::shared_ptr<RecordTypeDef> rec;
            if (s->decl_type == BaseType::RECORD) {
                auto it = symbols_.record_types.find(s->type_name);
                if (it == symbols_.record_types.end())
                    throw std::runtime_error("Unknown record type: " + s->type_name);
                rec = it->second;
            }
            symbols_.declareVariable(s->name, s->decl_type, s->array_size, rec);
            break;
        }
        case Stmt::RECORD_DECL: {
            for (auto& f : s->record_def->fields) {
                if (f.type == BaseType::RECORD && !f.record_type && !f.type_name.empty()) {
                    auto it = symbols_.record_types.find(f.type_name);
                    if (it == symbols_.record_types.end())
                        throw std::runtime_error("Unknown record field type: " + f.type_name);
                    f.record_type = it->second;
                }
            }
            symbols_.declareRecordType(s->record_def);
            break;
        }
        case Stmt::ASSIGN:
        case Stmt::CALL:
            evalExpr(s->expr);
            break;
        case Stmt::BLOCK:
            for (auto* c : s->body) {
                execStmt(c);
                if (robot_.escaped()) break;
            }
            break;
        case Stmt::LOOP: {
            int guard = 0;
            while (true) {
                if (++guard > 100000)
                    throw std::runtime_error("Loop iteration limit exceeded");
                Value c = convertValue(evalExpr(s->cond), BaseType::LOGIC, procConverter());
                if (c.isUndef() || !c.logic) break;
                for (auto* b : s->body) {
                    execStmt(b);
                    if (robot_.escaped()) return;
                }
            }
            break;
        }
        case Stmt::PROC_DEF:
            registerProc(s);
            break;
    }
}

void Interpreter::runSource(const std::string& source) {
    g_parse_error.clear();
    g_program = nullptr;
    lexer_set_input(source);
    if (yyparse() != 0 || !g_parse_error.empty())
        throw std::runtime_error(g_parse_error.empty() ? "Parse error" : g_parse_error);
    if (!g_program) return;
    std::unique_ptr<Program> hold(g_program);
    g_program = nullptr;
    for (auto* st : hold->stmts) {
        execStmt(st);
        if (robot_.escaped()) break;
    }
}

void Interpreter::run(const std::string& source_path) {
    std::ifstream in(source_path);
    if (!in) throw std::runtime_error("Cannot open program: " + source_path);
    std::ostringstream oss;
    oss << in.rdbuf();
    runSource(oss.str());
}


