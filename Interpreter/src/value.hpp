#pragma once

#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

enum class BaseType { UNDEF, LOGIC, NUMERIC, STRING, RECORD };

struct RecordTypeDef;

struct Value {
    BaseType type = BaseType::UNDEF;
    bool logic = false;
    long long num = 0;
    std::string str;
    std::vector<Value> arr;
    std::shared_ptr<RecordTypeDef> record_def;
    std::shared_ptr<std::unordered_map<std::string, Value>> fields;

    static Value makeUndef();
    static Value makeLogic(bool v);
    static Value makeNumeric(long long v);
    static Value makeString(std::string s);
    static Value makeArray(std::vector<Value> elems, BaseType elem_type);
    static Value makeRecord(std::shared_ptr<RecordTypeDef> def);

    bool isArray() const { return !arr.empty() || array_element_type != BaseType::UNDEF; }
    bool isUndef() const {
        if (isArray()) return false;
        if (type == BaseType::RECORD && fields) return false;
        return type == BaseType::UNDEF;
    }
    BaseType array_element_type = BaseType::UNDEF;

    Value clone() const;
};

struct RecordField {
    BaseType type = BaseType::UNDEF;
    std::string name;
    std::shared_ptr<RecordTypeDef> record_type;
    std::string type_name;
};

struct ConversionDef {
    BaseType target;
    std::string proc_name;
};

struct RecordTypeDef {
    std::string name;
    std::vector<RecordField> fields;
    std::vector<ConversionDef> to_conversions;
    std::vector<ConversionDef> from_conversions;
};

struct LValue {
    Value* ptr = nullptr;
    bool valid = false;
    BaseType target_type = BaseType::UNDEF;
    std::shared_ptr<RecordTypeDef> record_type;
    bool is_array = false;
};

enum class Logic3 { False = 0, True = 1, Unknown = 2 };

Logic3 toLogic3(const Value& v);
Value fromLogic3(Logic3 v);

using ConvFn = std::function<void(const std::string&, Value&, Value&)>;

Value convertValue(const Value& src, BaseType target, const ConvFn& callConv);
Value convertToRecord(const Value& src, std::shared_ptr<RecordTypeDef> target, const ConvFn& callConv);

Value applyUnaryMinus(const Value& v);
Value applyBinary(const Value& left, const Value& right, char op, bool left_dot, bool right_dot,
                  const ConvFn& callConv);
Value applyCompare(const Value& left, const Value& right, char op, bool left_dot, bool right_dot,
                   const ConvFn& callConv);