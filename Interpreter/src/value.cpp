#include "value.hpp"

#include <cmath>

Value Value::makeUndef() {
    return {};
}

Value Value::makeLogic(bool v) {
    Value r;
    r.type = BaseType::LOGIC;
    r.logic = v;
    return r;
}

Value Value::makeNumeric(long long v) {
    Value r;
    r.type = BaseType::NUMERIC;
    r.num = v;
    return r;
}

Value Value::makeString(std::string s) {
    Value r;
    r.type = BaseType::STRING;
    r.str = std::move(s);
    return r;
}

Value Value::makeArray(std::vector<Value> elems, BaseType elem_type) {
    Value r;
    r.type = BaseType::UNDEF;
    r.arr = std::move(elems);
    r.array_element_type = elem_type;
    return r;
}

Value Value::makeRecord(std::shared_ptr<RecordTypeDef> def) {
    Value r;
    r.type = BaseType::RECORD;
    r.record_def = std::move(def);
    r.fields = std::make_shared<std::unordered_map<std::string, Value>>();
    for (const auto& f : r.record_def->fields) {
        if (f.type == BaseType::RECORD && f.record_type) {
            (*r.fields)[f.name] = makeRecord(f.record_type);
        } else {
            (*r.fields)[f.name] = Value::makeUndef();
        }
    }
    return r;
}

Value Value::clone() const {
    Value r = *this;
    if (!arr.empty()) {
        r.arr.clear();
        for (const auto& e : arr) r.arr.push_back(e.clone());
    }
    if (type == BaseType::RECORD && fields) {
        auto nf = std::make_shared<std::unordered_map<std::string, Value>>();
        for (const auto& [k, v] : *fields) (*nf)[k] = v.clone();
        r.fields = std::move(nf);
    }
    return r;
}

Logic3 toLogic3(const Value& v) {
    if (v.isUndef()) return Logic3::Unknown;
    if (v.type == BaseType::LOGIC) return v.logic ? Logic3::True : Logic3::False;
    if (v.type == BaseType::NUMERIC) {
        if (v.num == 0) return Logic3::False;
        if (v.num == 1) return Logic3::True;
        return Logic3::Unknown;
    }
    return Logic3::Unknown;
}

Value fromLogic3(Logic3 v) {
    if (v == Logic3::Unknown) return Value::makeUndef();
    return Value::makeLogic(v == Logic3::True);
}

static Logic3 logicAnd(Logic3 a, Logic3 b) {
    if (a == Logic3::False || b == Logic3::False) return Logic3::False;
    if (a == Logic3::Unknown || b == Logic3::Unknown) return Logic3::Unknown;
    return Logic3::True;
}

static Logic3 logicOr(Logic3 a, Logic3 b) {
    if (a == Logic3::True || b == Logic3::True) return Logic3::True;
    if (a == Logic3::Unknown || b == Logic3::Unknown) return Logic3::Unknown;
    return Logic3::False;
}

static Logic3 logicXor(Logic3 a, Logic3 b) {
    if (a == Logic3::Unknown || b == Logic3::Unknown) return Logic3::Unknown;
    return (a != b) ? Logic3::True : Logic3::False;
}

static Logic3 logicNand(Logic3 a, Logic3 b) {
    auto r = logicAnd(a, b);
    if (r == Logic3::Unknown) return Logic3::Unknown;
    return r == Logic3::True ? Logic3::False : Logic3::True;
}

static Logic3 logicNor(Logic3 a, Logic3 b) {
    auto r = logicOr(a, b);
    if (r == Logic3::Unknown) return Logic3::Unknown;
    return r == Logic3::True ? Logic3::False : Logic3::True;
}

static Logic3 logicNot(Logic3 a) {
    if (a == Logic3::Unknown) return Logic3::Unknown;
    return a == Logic3::True ? Logic3::False : Logic3::True;
}

Value convertValue(const Value& src, BaseType target, const ConvFn& callConv) {
    if (src.isArray()) {
        std::vector<Value> out;
        out.reserve(src.arr.size());
        for (const auto& e : src.arr)
            out.push_back(convertValue(e, target, callConv));
        return Value::makeArray(std::move(out), target);
    }
    if (src.isUndef()) return Value::makeUndef();
    if (src.type == target) return src;

    if (src.type == BaseType::RECORD && src.record_def) {
        for (const auto& c : src.record_def->to_conversions) {
            if (c.target == target) {
                Value first = src.clone();
                Value second = Value::makeUndef();
                if (callConv) callConv(c.proc_name, first, second);
                return second;
            }
        }
        return Value::makeUndef();
    }

    if (target == BaseType::RECORD) return Value::makeUndef();

    switch (src.type) {
        case BaseType::LOGIC:
            if (target == BaseType::NUMERIC) return Value::makeNumeric(src.logic ? 1 : 0);
            if (target == BaseType::STRING) return Value::makeString(src.logic ? "TRUE" : "FALSE");
            break;
        case BaseType::NUMERIC:
            if (target == BaseType::LOGIC) {
                if (src.num == 0) return Value::makeLogic(false);
                if (src.num == 1) return Value::makeLogic(true);
                return Value::makeUndef();
            }
            if (target == BaseType::STRING) return Value::makeString(std::to_string(src.num));
            break;
        case BaseType::STRING:
            if (target == BaseType::NUMERIC) {
                try {
                    size_t pos = 0;
                    long long n = std::stoll(src.str, &pos);
                    if (pos == src.str.size()) return Value::makeNumeric(n);
                } catch (...) {}
                return Value::makeNumeric(0);
            }
            if (target == BaseType::LOGIC) {
                if (src.str == "TRUE" || src.str == "true" || src.str == "1")
                    return Value::makeLogic(true);
                return Value::makeLogic(false);
            }
            break;
        default:
            break;
    }
    return Value::makeUndef();
}

Value convertToRecord(const Value& src, std::shared_ptr<RecordTypeDef> target, const ConvFn& callConv) {
    if (src.isUndef()) return Value::makeUndef();
    if (src.type == BaseType::RECORD && src.record_def == target) return src;
    for (const auto& c : target->from_conversions) {
        if (src.type == c.target || c.target == BaseType::RECORD) {
            Value first = Value::makeRecord(target);
            Value second = src.clone();
            if (callConv) callConv(c.proc_name, first, second);
            return first;
        }
    }
    return Value::makeUndef();
}

static Value elemWise(const Value& a, const Value& b,
                      const std::function<Value(const Value&, const Value&)>& op) {
    if (!a.arr.empty() || !b.arr.empty()) {
        size_t n = std::max(a.arr.size(), b.arr.size());
        std::vector<Value> res;
        for (size_t i = 0; i < n; ++i) {
            Value va = i < a.arr.size() ? a.arr[i] : Value::makeUndef();
            Value vb = i < b.arr.size() ? b.arr[i] : Value::makeUndef();
            res.push_back(op(va, vb));
        }
        BaseType et = a.array_element_type != BaseType::UNDEF ? a.array_element_type : b.array_element_type;
        return Value::makeArray(std::move(res), et);
    }
    return op(a, b);
}

Value applyUnaryMinus(const Value& v) {
    if (!v.arr.empty()) {
        std::vector<Value> res;
        for (const auto& e : v.arr) res.push_back(applyUnaryMinus(e));
        return Value::makeArray(std::move(res), v.array_element_type);
    }
    if (v.isUndef()) return Value::makeUndef();
    if (v.type == BaseType::NUMERIC) return Value::makeNumeric(-v.num);
    if (v.type == BaseType::LOGIC) return fromLogic3(logicNot(toLogic3(v)));
    return Value::makeUndef();
}

static Value binScalar(const Value& left, const Value& right, char op, bool left_dot, bool right_dot,
                       const ConvFn& callConv) {
    if (left.isUndef() || right.isUndef()) {
        if (left.type == BaseType::LOGIC || right.type == BaseType::LOGIC ||
            left.type == BaseType::UNDEF || right.type == BaseType::UNDEF) {
            Logic3 la = toLogic3(left);
            Logic3 ra = toLogic3(right);
            switch (op) {
                case '+': return fromLogic3(logicOr(la, ra));
                case '-': return fromLogic3(logicXor(la, ra));
                case '*': return fromLogic3(logicAnd(la, ra));
                case '/': return fromLogic3(logicNand(la, ra));
                case '^': return fromLogic3(logicNor(la, ra));
                default: break;
            }
        }
        return Value::makeUndef();
    }

    Value l = left, r = right;
    BaseType common = l.type;
    if (left_dot) {
        r = convertValue(r, l.type, callConv);
    } else if (right_dot) {
        l = convertValue(l, r.type, callConv);
        common = r.type;
    } else if (l.type != r.type) {
        return Value::makeUndef();
    }

    if (common == BaseType::NUMERIC) {
        switch (op) {
            case '+': return Value::makeNumeric(l.num + r.num);
            case '-': return Value::makeNumeric(l.num - r.num);
            case '*': return Value::makeNumeric(l.num * r.num);
            case '/':
                if (r.num == 0) return Value::makeUndef();
                return Value::makeNumeric(l.num / r.num);
            case '^': return Value::makeNumeric(static_cast<long long>(std::pow(l.num, r.num)));
            default: break;
        }
    }
    if (common == BaseType::LOGIC) {
        Logic3 la = toLogic3(l), ra = toLogic3(r);
        switch (op) {
            case '+': return fromLogic3(logicOr(la, ra));
            case '-': return fromLogic3(logicXor(la, ra));
            case '*': return fromLogic3(logicAnd(la, ra));
            case '/': return fromLogic3(logicNand(la, ra));
            case '^': return fromLogic3(logicNor(la, ra));
            default: break;
        }
    }
    return Value::makeUndef();
}

Value applyBinary(const Value& left, const Value& right, char op, bool left_dot, bool right_dot,
                  const ConvFn& callConv) {
    return elemWise(left, right, [&](const Value& a, const Value& b) {
        return binScalar(a, b, op, left_dot, right_dot, callConv);
    });
}

Value applyCompare(const Value& left, const Value& right, char op, bool left_dot, bool right_dot,
                   const ConvFn& callConv) {
    auto cmp = [&](const Value& a, const Value& b) -> Value {
        if (a.isUndef() || b.isUndef()) return Value::makeUndef();
        Value l = a, r = b;
        if (left_dot) r = convertValue(r, l.type, callConv);
        else if (right_dot) l = convertValue(l, r.type, callConv);
        else if (l.type != r.type) return Value::makeUndef();

        bool eq = false;
        if (l.type == BaseType::NUMERIC) eq = (l.num == r.num);
        else if (l.type == BaseType::LOGIC) eq = (l.logic == r.logic);
        else if (l.type == BaseType::STRING) eq = (l.str == r.str);
        else return Value::makeUndef();

        if (op == '?') return Value::makeLogic(eq);
        if (op == '!') return Value::makeLogic(!eq);
        if (l.type != BaseType::NUMERIC) return Value::makeUndef();
        if (op == '<') return Value::makeLogic(l.num < r.num);
        if (op == '>') return Value::makeLogic(l.num > r.num);
        return Value::makeUndef();
    };
    return elemWise(left, right, cmp);
}



