#include "symbol_table.hpp"

void SymbolTable::declareVariable(const std::string& name, BaseType type, int array_size,
                                  std::shared_ptr<RecordTypeDef> rec) {
    if (scopes_.empty()) pushScope();
    if (scopes_.back().vars.count(name))
        throw std::runtime_error("Variable already declared: " + name);

    Variable var;
    var.declared_type = type;
    var.record_type = std::move(rec);
    var.is_array = array_size >= 0;

    if (var.is_array) {
        std::vector<Value> elems(static_cast<size_t>(array_size), Value::makeUndef());
        var.value = Value::makeArray(std::move(elems), type);
    } else {
        var.value = Value::makeUndef();
    }
    scopes_.back().vars[name] = std::move(var);
}

void SymbolTable::declareVariableRef(const std::string& name, Value* target) {
    if (scopes_.empty()) pushScope();
    if (scopes_.back().vars.count(name))
        throw std::runtime_error("Variable already declared: " + name);
    if (!target)
        throw std::runtime_error("Invalid reference parameter: " + name);

    Variable var;
    var.ref = target;
    scopes_.back().vars[name] = std::move(var);
}

void SymbolTable::declareRecordType(std::shared_ptr<RecordTypeDef> def) {
    record_types[def->name] = std::move(def);
}

Variable* SymbolTable::findVar(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto f = it->vars.find(name);
        if (f != it->vars.end()) return &f->second;
    }
    return nullptr;
}

const Variable* SymbolTable::findVar(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto f = it->vars.find(name);
        if (f != it->vars.end()) return &f->second;
    }
    return nullptr;
}

LValue SymbolTable::resolveLValue(const std::string& name) {
    auto* v = findVar(name);
    if (!v) return {};
    LValue lv;
    lv.valid = true;
    lv.ptr = v->ref ? v->ref : &v->value;
    lv.target_type = v->declared_type;
    lv.record_type = v->record_type;
    lv.is_array = v->is_array;
    return lv;
}

LValue SymbolTable::resolveArrayIndex(const std::string& name, long long index) {
    auto* v = findVar(name);
    if (!v) return {};
    Value& container = v->ref ? *v->ref : v->value;
    if (index < 0 || static_cast<size_t>(index) >= container.arr.size()) return {};
    LValue lv;
    lv.valid = true;
    lv.ptr = &container.arr[static_cast<size_t>(index)];
    lv.target_type = container.array_element_type != BaseType::UNDEF
                         ? container.array_element_type
                         : v->declared_type;
    lv.record_type = v->record_type;
    lv.is_array = false;
    return lv;
}

LValue SymbolTable::resolveField(const std::string& name, const std::string& field) {
    auto* v = findVar(name);
    if (!v) return {};
    Value& container = v->ref ? *v->ref : v->value;
    if (container.type != BaseType::RECORD || !container.fields) return {};
    auto f = container.fields->find(field);
    if (f == container.fields->end()) return {};
    LValue lv;
    lv.valid = true;
    lv.ptr = &f->second;
    lv.is_array = false;
    if (container.record_def) {
        for (const auto& fd : container.record_def->fields) {
            if (fd.name == field) {
                lv.target_type = fd.type;
                lv.record_type = fd.record_type;
                break;
            }
        }
    }
    return lv;
}

Value SymbolTable::getVariable(const std::string& name) const {
    const auto* v = findVar(name);
    if (!v) throw std::runtime_error("Unknown variable: " + name);
    if (v->ref) return v->ref->clone();
    return v->value;
}

void SymbolTable::pushScope() { scopes_.push_back({}); }
void SymbolTable::popScope() {
    if (!scopes_.empty()) scopes_.pop_back();
}

bool SymbolTable::isArrayVariable(const std::string& name) const {
    const auto* v = findVar(name);
    return v && v->is_array;
}

Procedure* SymbolTable::getProcedure(const std::string& name) {
    auto it = procedures.find(name);
    return it != procedures.end() ? &it->second : nullptr;
}