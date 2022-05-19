#include "vm_primitive_types.h"
#include "vm_env.h"
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
                    auto message = env->backtrace("IntegerValue::Op(+) not permitted");
                    std::cout<<message<<std::endl;
                    throw ExecuteException(op, std::move(message));
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
                    rst.setBool(intValue() < other.intValue());
                    return rst;
                }
                else if(other.type() == PrimeVType::Float64) {
                    rst.setBool(intValue() < other.floatValue());
                    return rst;
                } else {
                    throw ExecuteException(op);
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
                    throw ExecuteException(op);
                }
            }
            default: {
                throw ExecuteException(op);
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
}