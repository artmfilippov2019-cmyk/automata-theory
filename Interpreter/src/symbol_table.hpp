#pragma once

#include "value.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Variable {
    Value value;
    Value* ref = nullptr;
    BaseType declared_type = BaseType::UNDEF;
    std::shared_ptr<RecordTypeDef> record_type;
    bool is_array = false;
};

struct CallArg {
    Value value;
    LValue lvalue;
    bool by_ref = false;
};

struct ParamInfo {
    BaseType type = BaseType::UNDEF;
    std::string name;
    bool by_ref = false;
    std::shared_ptr<RecordTypeDef> record_type;
    std::string type_name;
};

struct Stmt;

struct Procedure {
    std::string name;
    std::vector<ParamInfo> params;
    std::vector<Stmt*> body;
};

class SymbolTable {
public:
    std::unordered_map<std::string, std::shared_ptr<RecordTypeDef>> record_types;
    std::unordered_map<std::string, Procedure> procedures;

    void declareVariable(const std::string& name, BaseType type, int array_size,
                         std::shared_ptr<RecordTypeDef> rec = nullptr);
    void declareVariableRef(const std::string& name, Value* target);
    void declareRecordType(std::shared_ptr<RecordTypeDef> def);

    Variable* findVar(const std::string& name);
    const Variable* findVar(const std::string& name) const;

    LValue resolveLValue(const std::string& name);
    LValue resolveArrayIndex(const std::string& name, long long index);
    LValue resolveField(const std::string& name, const std::string& field);

    Value getVariable(const std::string& name) const;

    void pushScope();
    void popScope();

    bool isArrayVariable(const std::string& name) const;
    Procedure* getProcedure(const std::string& name);

private:
    struct Scope {
        std::unordered_map<std::string, Variable> vars;
    };
    std::vector<Scope> scopes_;

};


