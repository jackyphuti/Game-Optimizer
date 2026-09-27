#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <chrono>
#include <map>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace optimizer {

enum class ProcessPriority {
    Idle = 0,
    BelowNormal = 1,
    Normal = 2,
    AboveNormal = 3,
    High = 4,
    Realtime = 5
};

inline std::string priorityToString(ProcessPriority prio) {
    switch (prio) {
        case ProcessPriority::Idle: return "Idle";
        case ProcessPriority::BelowNormal: return "BelowNormal";
        case ProcessPriority::Normal: return "Normal";
        case ProcessPriority::AboveNormal: return "AboveNormal";
        case ProcessPriority::High: return "High";
        case ProcessPriority::Realtime: return "Realtime";
        default: return "Normal";
    }
}

inline ProcessPriority priorityFromString(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    if (s == "idle") return ProcessPriority::Idle;
    if (s == "belownormal" || s == "below_normal" || s == "low") return ProcessPriority::BelowNormal;
    if (s == "normal") return ProcessPriority::Normal;
    if (s == "abovenormal" || s == "above_normal") return ProcessPriority::AboveNormal;
    if (s == "high") return ProcessPriority::High;
    if (s == "realtime" || s == "real_time") return ProcessPriority::Realtime;
    return ProcessPriority::High;
}

struct RunningProcessInfo {
    uint32_t pid{0};
    std::string name;
    std::string exePath;
    ProcessPriority originalPriority{ProcessPriority::Normal};
    uint64_t originalAffinity{0};
};

struct GameProfile {
    std::string name;
    std::string exeName; // e.g. "cs2.exe" or "cs2"
    std::string fullPath;
    bool tuneCpuPriority{true};
    ProcessPriority targetPriority{ProcessPriority::High};
    bool tuneAffinity{false};
    uint64_t affinityMask{0}; // 0 = all cores
    bool tuneGpu{true};
    bool switchPowerPlan{true};
    bool lowerBackgroundProcesses{true};
    bool trimRamOnLaunch{true};
    bool dropCachesOnLaunch{false}; // Opt-in only (Linux)
    std::vector<std::string> customBackgroundBlacklist;
};

struct SystemMetrics {
    uint64_t totalRamBytes{0};
    uint64_t availableRamBytes{0};
    uint64_t usedRamBytes{0};
    double ramUsagePercent{0.0};
    double cpuUsagePercent{0.0};
    std::string activePowerPlan;
    std::string gpuVendor;
    std::string gpuName;
    bool gameModeActive{false};
};

// Optimization record for rollback
struct ProcessRollbackEntry {
    uint32_t pid{0};
    std::string name;
    ProcessPriority originalPriority{ProcessPriority::Normal};
    uint64_t originalAffinity{0};
    bool wasSuspended{false};
};

struct ActiveOptimizationSession {
    bool active{false};
    uint32_t gamePid{0};
    std::string gameName;
    std::string gameExe;
    std::string startTime;
    std::string originalPowerPlan;
    std::string originalGpuState;
    std::vector<ProcessRollbackEntry> modifiedProcesses;
    bool originalGameModeActive{false};
};

// Minimal robust JSON implementation for cross-platform portability without heavy dependencies
namespace json {

enum class Type { Null, Boolean, Number, String, Array, Object };

struct Value {
    Type type{Type::Null};
    bool boolVal{false};
    double numVal{0.0};
    std::string strVal;
    std::vector<Value> arrVal;
    std::map<std::string, Value> objVal;

    Value() = default;
    Value(bool b) : type(Type::Boolean), boolVal(b) {}
    Value(int n) : type(Type::Number), numVal(n) {}
    Value(uint32_t n) : type(Type::Number), numVal(n) {}
    Value(uint64_t n) : type(Type::Number), numVal(static_cast<double>(n)) {}
    Value(double n) : type(Type::Number), numVal(n) {}
    Value(const char* s) : type(Type::String), strVal(s ? s : "") {}
    Value(std::string s) : type(Type::String), strVal(std::move(s)) {}
    Value(Type t) : type(t) {}

    bool isNull() const { return type == Type::Null; }
    bool isBool() const { return type == Type::Boolean; }
    bool isNumber() const { return type == Type::Number; }
    bool isString() const { return type == Type::String; }
    bool isArray() const { return type == Type::Array; }
    bool isObject() const { return type == Type::Object; }

    bool asBool(bool def = false) const { return isBool() ? boolVal : def; }
    int asInt(int def = 0) const { return isNumber() ? static_cast<int>(numVal) : def; }
    uint32_t asUInt(uint32_t def = 0) const { return isNumber() ? static_cast<uint32_t>(numVal) : def; }
    uint64_t asUInt64(uint64_t def = 0) const { return isNumber() ? static_cast<uint64_t>(numVal) : def; }
    double asDouble(double def = 0.0) const { return isNumber() ? numVal : def; }
    std::string asString(const std::string& def = "") const { return isString() ? strVal : def; }

    bool contains(const std::string& key) const {
        if (!isObject()) return false;
        return objVal.find(key) != objVal.end();
    }

    Value& operator[](const std::string& key) {
        if (type != Type::Object) {
            type = Type::Object;
            objVal.clear();
        }
        return objVal[key];
    }

    const Value& operator[](const std::string& key) const {
        static const Value nullVal;
        if (!isObject()) return nullVal;
        auto it = objVal.find(key);
        return (it != objVal.end()) ? it->second : nullVal;
    }

    Value& operator[](size_t index) {
        if (type != Type::Array) {
            type = Type::Array;
            arrVal.clear();
        }
        if (index >= arrVal.size()) {
            arrVal.resize(index + 1);
        }
        return arrVal[index];
    }

    const Value& operator[](size_t index) const {
        static const Value nullVal;
        if (!isArray() || index >= arrVal.size()) return nullVal;
        return arrVal[index];
    }

    void push_back(Value v) {
        if (type != Type::Array) {
            type = Type::Array;
            arrVal.clear();
        }
        arrVal.push_back(std::move(v));
    }

    std::string serialize(int indent = 2, int currentIndent = 0) const {
        std::string ind(currentIndent, ' ');
        std::string nextInd(currentIndent + indent, ' ');
        std::ostringstream ss;

        switch (type) {
            case Type::Null: ss << "null"; break;
            case Type::Boolean: ss << (boolVal ? "true" : "false"); break;
            case Type::Number: {
                if (numVal == static_cast<int64_t>(numVal)) {
                    ss << static_cast<int64_t>(numVal);
                } else {
                    ss << std::fixed << std::setprecision(4) << numVal;
                }
                break;
            }
            case Type::String: {
                ss << "\"";
                for (char c : strVal) {
                    if (c == '"') ss << "\\\"";
                    else if (c == '\\') ss << "\\\\";
                    else if (c == '\n') ss << "\\n";
                    else if (c == '\r') ss << "\\r";
                    else if (c == '\t') ss << "\\t";
                    else ss << c;
                }
                ss << "\"";
                break;
            }
            case Type::Array: {
                if (arrVal.empty()) { ss << "[]"; break; }
                ss << "[\n";
                for (size_t i = 0; i < arrVal.size(); ++i) {
                    ss << nextInd << arrVal[i].serialize(indent, currentIndent + indent);
                    if (i + 1 < arrVal.size()) ss << ",";
                    ss << "\n";
                }
                ss << ind << "]";
                break;
            }
            case Type::Object: {
                if (objVal.empty()) { ss << "{}"; break; }
                ss << "{\n";
                size_t i = 0;
                for (const auto& [k, v] : objVal) {
                    ss << nextInd << "\"" << k << "\": " << v.serialize(indent, currentIndent + indent);
                    if (++i < objVal.size()) ss << ",";
                    ss << "\n";
                }
                ss << ind << "}";
                break;
            }
        }
        return ss.str();
    }
};

class Parser {
    const std::string& src;
    size_t pos{0};

    void skipWhitespace() {
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\n' || src[pos] == '\r')) {
            pos++;
        }
    }

    char peek() {
        skipWhitespace();
        return (pos < src.size()) ? src[pos] : '\0';
    }

    char get() {
        skipWhitespace();
        return (pos < src.size()) ? src[pos++] : '\0';
    }

    std::string parseString() {
        if (get() != '"') return "";
        std::string res;
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') return res;
            if (c == '\\' && pos < src.size()) {
                char esc = src[pos++];
                if (esc == '"') res += '"';
                else if (esc == '\\') res += '\\';
                else if (esc == 'n') res += '\n';
                else if (esc == 'r') res += '\r';
                else if (esc == 't') res += '\t';
                else res += esc;
            } else {
                res += c;
            }
        }
        return res;
    }

    Value parseNumber() {
        size_t start = pos;
        if (pos < src.size() && (src[pos] == '-' || src[pos] == '+')) pos++;
        while (pos < src.size() && (std::isdigit(static_cast<unsigned char>(src[pos])) || src[pos] == '.' || src[pos] == 'e' || src[pos] == 'E' || src[pos] == '+' || src[pos] == '-')) {
            pos++;
        }
        std::string numStr = src.substr(start, pos - start);
        try {
            double d = std::stod(numStr);
            return Value(d);
        } catch (...) {
            return Value(0.0);
        }
    }

public:
    explicit Parser(const std::string& input) : src(input) {}

    Value parse() {
        skipWhitespace();
        char c = peek();
        if (c == '{') {
            get(); // eat '{'
            Value obj(Type::Object);
            while (peek() != '}' && peek() != '\0') {
                std::string key = parseString();
                if (get() != ':') break;
                obj[key] = parse();
                if (peek() == ',') get();
            }
            if (peek() == '}') get();
            return obj;
        } else if (c == '[') {
            get(); // eat '['
            Value arr(Type::Array);
            while (peek() != ']' && peek() != '\0') {
                arr.push_back(parse());
                if (peek() == ',') get();
            }
            if (peek() == ']') get();
            return arr;
        } else if (c == '"') {
            return Value(parseString());
        } else if (c == 't' || c == 'f') {
            std::string token;
            while (std::isalpha(static_cast<unsigned char>(peek()))) token += get();
            return Value(token == "true");
        } else if (c == 'n') {
            std::string token;
            while (std::isalpha(static_cast<unsigned char>(peek()))) token += get();
            return Value(Type::Null);
        } else if (std::isdigit(static_cast<unsigned char>(c)) || c == '-') {
            return parseNumber();
        }
        return Value(Type::Null);
    }
};

inline Value parse(const std::string& input) {
    Parser p(input);
    return p.parse();
}

} // namespace json

} // namespace optimizer
