#pragma once
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <functional>
#include <vector>
#include <name_pool.h>
#include "compiler_common.h"

namespace compiler {

    using namespace ast;

    enum class ExecutionError {
        BinaryOpNotPermitted,
        AssignWasNotPermitted,
        UnsupportOperator,
        InvalidArgument,
        ArgumentCountMismatch,
        ArgumentTypeMismatch,
        IndexOutOfRange,
        KeyNotFound,
        IndexANoneObject,
        InvalidClassObject,
    };
    
    class ExecuteException : public std::exception {
    private:
        ExecutionError  _error;
        std::string     _backtrace;
    public:
        ExecuteException(std::string&& backtrace)
            : _backtrace(std::move(backtrace))
        {
        }
        ExecuteException(Token op)
            : _backtrace("")
        {
        }
        ExecutionError error() const {
            return _error;
        }
        std::string const& backtrace() const {
            return _backtrace;
        }
    };

    class DumpException : public std::exception {
    private:
        ExecutionError      _error;
        std::string         _message;
    public:
        DumpException(Env const* env, ExecutionError error, char const* brifError = nullptr);
        std::string const& dumpMessage() const {
            return _message;
        }
    };

    enum class PrimeVType : uint8_t {
        Nil,
        Boolean,
        Int64,
        Float64,
        String,
        Object,
        ValueRef,
        FunctionNode,
        BytecodeFunction,
        Userdata,
        BridgeFunc,
        // Vector,
        // Map,
    };

    class Value {
    protected:
        PrimeVType           _type;    // type of the value
        union {
        SymbolLayoutType     _stype;   // for only object
        };
        union {
            int64_t         _i64;
            double          _f64;
            bool            _bool;
            Name            _str;
            Object*         _obj;
            Value*          _ref;
            Node const*     _node;
            UserdataObject* _ud;
            BridgeFunc      _bridgeFunc;
            BytecodeFunction* _bytecodeFunc;
        };
    public:
        Value() 
            : _type(PrimeVType::Nil)
            , _stype(SymbolLayoutType::None)
            , _i64(0)
        {}

        Value(SymbolLayout* symbolLayout);
        Value(Value const& other);
        Value(Node const* node);
        Value(Value* ref);
        Value(Value&& other);
        Value(UserdataObject* ud);
        Value(BridgeFunc func);
        Value(Name name);
        Value(int64_t val);
        Value(double val);
        Value(bool val);
        Value(BytecodeFunction* func);

        Value& operator = (Value const& other);
        Value& operator = (Value&& other);
        Node const* node() const;
        operator bool () const;
        Object* asObject() const;
        // Function* asFunc() const;
        BridgeFunc asBridgeFunc() const;
        // BytecodeFunction const* asBytecodeFunc() const;
        BytecodeFunction* asBytecodeFunc();

        void decRef();
        void incRef();
        Value* ref();
        void deref();

        void nilIt();

        ~Value();

        void setInt64(int64_t i64);
        void setFloat64(double f64);
        void setString(Name name);
        void setBool(bool val);
        PrimeVType type() const;
        SymbolLayoutType stype() const;

        UserdataObject* ud() const;

        int64_t intValue() const;

        double floatValue() const;

        bool booleanValue() const;

        Name stringValue() const;

        Value operator[](uint32_t loc) const;

        Value indexAccess(Env* env) const;

        Value operator[](Name name) const;

        bool operator < (Value const& other) const;

        bool operator == (Value const& other) const;
    
    };

    class BytecodeFunction {
    private:
        Name                    _name;
        Value                   _package;
        Module*                 _module;
        SymbolLayout*           _symbolLayout;
        Instruction const*      _entry;
        Instruction const*      _end;
        uint32_t                _ip;
        uint32_t                _argc;
        uint32_t                _localSize;
        // 需要再加
    public:
        BytecodeFunction(Name name, Value package, Module* module)
            : _name(name)
            , _package(package)
            , _module(module)
            , _symbolLayout(nullptr)
            , _entry(nullptr)
            , _end(nullptr)
            , _ip(0)
            , _localSize(0)
        {}

        void setInstructionRange(Instruction const* entry, Instruction const* end) {
            _entry = entry;
            _end = end;
        }

        void setArgc(uint32_t argc) {
            _argc = argc;
        }

        void setSymbolLayout(SymbolLayout* symbolLayout);

        uint32_t localSize() const {
            return _localSize;
        }

        SymbolLayout const* symbolLayout() {
            return _symbolLayout;
        }

        Value package() const {
            return _package;
        }

        Module* module() const {
            return _module;
        }

        Instruction const* instruction() const {
            return _entry;
        }

        void setInstruction(Instruction const* instr) {
            _entry = instr;
            _ip = 0;
        }

        size_t ip() const {
            return _ip;
        }
        
    };


} // namespace name
