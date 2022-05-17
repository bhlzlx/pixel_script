#pragma once
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <functional>
#include <vector>
#include "../name_pool.h"
#include "ast_common.h"

namespace compiler {

    using namespace ast;

    enum class ExecutionError {
        BinaryOpNotPermitted,
    };
    
    class ExecuteException : public std::exception {
    private:
        ExecutionError _error;
        union {
            Token   _op;
        };
    public:
        ExecuteException(Token op)
            : _op(op)
        {
        }
        ExecutionError error() const {
            return _error;
        }
        Token op() const {
            return _op;
        }
    };

    // this script is for 64 bit only
    class TagPointer {
    private:
        union {
            uint64_t    _value;
            void*       _pointer;
            struct {
                uint64_t   _tag : 16;
                uint64_t   _addr : 48;
            };
        };
        template<class T>
        T* getPointer() {
            return reinterpret_cast<T*>(_addr);
        };
        template<class T>
        T const* getPointer() const {
            return reinterpret_cast<T*>(_addr);
        };
        template<class T>
        operator T*() {
            return getPointer<T>();
        };
        template<class T>
        operator T const*() const{
            return getPointer<T>();
        };
        TagPointer(void* ptr = nullptr) {
            _pointer = ptr;
            _tag = 0;
        }
    };

    using Name = ksgw::Name;

    enum class PrimeVType : uint8_t {
        Nil,
        Boolean,
        Int64,
        Float64,
        String,
        Object,
        ValueRef,
        FunctionNode,
        Userdata
    };

    class Value {
    protected:
        PrimeVType           _type;    // type of the value
        SymbolLayoutType     _stype;   // for only object
        union {
            int64_t         _i64;
            double          _f64;
            Name            _str;
            Object*         _obj;
            Value*          _ref;
            Node const*     _node;
            UserdataObject* _ud;
        };
    public:
        Value() 
            : _type(PrimeVType::Nil)
            , _stype(SymbolLayoutType::None)
            , _i64(0)
        {
        }

        Value(SymbolLayout* symbolLayout);
        Value(Value const& other);
        Value(Node const* node);
        Value(Value* ref);
        Value(Value&& other);
        Value(UserdataObject* ud);

        Value& operator = (Value const& other);
        Value& operator = (Value&& other);
        Node const* node() const;
        operator bool () const;
        Object* asObject() const;
        Function* asFunc() const;

        void decRef() ;
        void incRef() ;
        Value* ref();
        void deref();

        ~Value();

        void setInt64(int64_t i64);
        void setFloat64(double f64);
        void setString(Name name);
        PrimeVType type() const;
        SymbolLayoutType stype() const;

        UserdataObject* ud() const;

        int64_t intValue() const;

        double floatValue() const;

        bool booleanValue() const;

        Name stringValue() const;

        // Value operator[](uint32_t loc);
        Value operator[](uint32_t loc) const;

        // Value operator[](Name name) ;
        Value operator[](Name name) const;

        // Value& operator = (Value const& other);

        void enumerateFunctions(std::function<void(ast::Function*)> const& func) const;
    };

    class IntegerValue: public Value {
    public:
        IntegerValue(int64_t i64) {
            setInt64(i64);
        }

        Value Op( Token op, Value const& other) const {
            Value rst;
            switch(op.type()) {
                case TokenType::Plus: {
                    if(other.type() == PrimeVType::Int64) {
                        rst.setInt64(intValue() + other.intValue());
                        return rst;
                    }
                    else if(other.type() == PrimeVType::Float64) {
                        rst.setInt64(intValue() + other.floatValue());
                        return rst;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                case TokenType::Minus: {
                    if(other.type() == PrimeVType::Int64) {
                        rst.setInt64(intValue() - other.intValue());
                        return rst;
                    }
                    else if(other.type() == PrimeVType::Float64) {
                        rst.setInt64(intValue() - other.floatValue());
                        return rst;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                case TokenType::Star: {
                    if(other.type() == PrimeVType::Int64) {
                        rst.setInt64(intValue() * other.intValue());
                        return rst;
                    }
                    else if(other.type() == PrimeVType::Float64) {
                        rst.setInt64(intValue() * other.floatValue());
                        return rst;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                case TokenType::Slash: {
                    if(other.type() == PrimeVType::Int64) {
                        rst.setInt64(intValue() / other.intValue());
                        return rst;
                    }
                    else if(other.type() == PrimeVType::Float64) {
                        rst.setInt64(intValue() / other.floatValue());
                        return rst;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                case TokenType::Assign: {
                    auto v = const_cast<IntegerValue*>(this);
                    if(other.type() == PrimeVType::Int64) {
                        v->setInt64(other.intValue());
                        return *this;
                    } else if(other.type() == PrimeVType::Float64) {
                        v->setFloat64(other.floatValue());
                        return *this;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                case TokenType::Less: {
                    if(other.type() == PrimeVType::Int64) {
                        rst.setInt64(intValue() < other.intValue());
                        return rst;
                    }
                    else if(other.type() == PrimeVType::Float64) {
                        rst.setInt64(intValue() < other.floatValue());
                        return rst;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                default: {
                    throw ExecuteException(op);
                }
            }
            return Value();
        }
    };

    class FloatValue: public Value {
    public:
        FloatValue(double f64) {
            setFloat64(f64);
        }
        Value Op( Token op, Value const& other) const {
            Value rst;
            switch(op.type()) {
                case TokenType::Plus: {
                    if(other.type() == PrimeVType::Int64) {
                        rst.setFloat64(floatValue() + other.intValue());
                        return rst;
                    }
                    else if(other.type() == PrimeVType::Float64) {
                        rst.setFloat64(floatValue() + other.floatValue());
                        return rst;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                case TokenType::Minus: {
                    if(other.type() == PrimeVType::Int64) {
                        rst.setFloat64(floatValue() - other.intValue());
                        return rst;
                    }
                    else if(other.type() == PrimeVType::Float64) {
                        rst.setFloat64(floatValue() - other.floatValue());
                        return rst;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                case TokenType::Star: {
                    if(other.type() == PrimeVType::Int64) {
                        rst.setFloat64(floatValue() * other.intValue());
                        return rst;
                    }
                    else if(other.type() == PrimeVType::Float64) {
                        rst.setFloat64(floatValue() * other.floatValue());
                        return rst;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                case TokenType::Slash: {
                    if(other.type() == PrimeVType::Int64) {
                        rst.setFloat64(floatValue() / other.intValue());
                        return rst;
                    }
                    else if(other.type() == PrimeVType::Float64) {
                        rst.setFloat64(floatValue() / other.floatValue());
                        return rst;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                case TokenType::Assign: {
                    auto v = const_cast<FloatValue*>(this);
                    if(other.type() == PrimeVType::Int64) {
                        v->setInt64(other.intValue());
                        return *this;
                    } else if(other.type() == PrimeVType::Float64) {
                        v->setFloat64(other.floatValue());
                        return *this;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                case TokenType::Less: {
                    if(other.type() == PrimeVType::Int64) {
                        rst.setFloat64(floatValue() < other.intValue());
                        return rst;
                    }
                    else if(other.type() == PrimeVType::Float64) {
                        rst.setFloat64(floatValue() < other.floatValue());
                        return rst;
                    } else {
                        throw ExecuteException(op);
                    }
                }
                default: {
                    throw ExecuteException(op);
                }
            }
        }
    };

} // namespace name
