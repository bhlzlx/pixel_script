#include "vm_primitive_types.h"
#include "vm_env.h"
#include "stdlib/std_string.h"
#include <iostream>

namespace compiler {
    
    Value IntegerValue::Op(Env* env, Token op, Value const& other) const {
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
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted, "integer op with non-integer type");
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
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
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
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
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
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
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
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
                }
            }
            case TokenType::Less: {
                if(other.type() == PrimeVType::Int64) {
                    rst.setBool(intValue() < other.intValue());
                    return rst;
                }
                else if(other.type() == PrimeVType::Float64) {
                    rst.setBool(intValue() < other.floatValue());
                    return rst;
                } else {
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
                }
            }
            case TokenType::Greater: {
                if(other.type() == PrimeVType::Int64) {
                    rst.setBool(intValue() > other.intValue());
                    return rst;
                }
                else if(other.type() == PrimeVType::Float64) {
                    rst.setBool(intValue() > other.floatValue());
                    return rst;
                } else {
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
                }
            }
            default: {
                throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
            }
        }
        return Value();
    }

    Value FloatValue::Op(Env* env, Token op, Value const& other) const {
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
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
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
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
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
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
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
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
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
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
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
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
                }
            }
            default: {
                throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
            }
        }
    }


    Value StringValue::Op(Env* env, Token op, Value const& other) const {
        Value rst;
        switch(op.type()) {
            case TokenType::Plus: {
                if(other.type() == PrimeVType::String) {
                    env->stackValues().pushArgBegin();
                    env->stackValues().pushValue(*this);
                    env->stackValues().pushValue(other);
                    env->stackValues().pushArgEnd();
                    int ret = string_impl::__append(env);
                    rst = env->stackValues().popValue();
                    env->stackValues().popToArgBegin();
                    return rst;
                } else {
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted, "string + other type");
                }
            }
            case TokenType::Assign: {
                auto v = const_cast<StringValue*>(this);
                if(other.type() == PrimeVType::String) {
                    v->setString(other.stringValue());
                    return *this;
                } else {
                    throw DumpException(env, ExecutionError::AssignWasNotPermitted);
                }
            }
            default: {
                throw DumpException(env, ExecutionError::UnsupportOperator);
            }
        }
    }
}