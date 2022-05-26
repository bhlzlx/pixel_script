#include "vm_primitive_types.h"
#include <vm/vm_env.h>
#include <vm/stdlib/std_string.h>
#include <iostream>

namespace compiler {
    
    Value IntegerValue::Op(Env* env, Opcode op, Value const& other) {
        Value rst;
        switch(op) {
            case Opcode::Add: {
                if(other.type() == PrimeVType::Int64) {
                    return Value(_i64 + other.intValue());
                }
                else if(other.type() == PrimeVType::Float64) {
                    return Value(_i64 + other.floatValue());
                } else {
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted, "integer op with non-integer type");
                }
                break;
            }
            case Opcode::Sub: {
                if(other.type() == PrimeVType::Int64) {
                    return Value(_i64 - other.intValue());
                }
                else if(other.type() == PrimeVType::Float64) {
                    return Value(_i64 - other.floatValue());
                } else {
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted, "integer op with non-integer type");
                }
                break;
            }
            case Opcode::Mul: {
                if(other.type() == PrimeVType::Int64) {
                    return Value(_i64 * other.intValue());
                }
                else if(other.type() == PrimeVType::Float64) {
                    return Value(_i64 * other.floatValue());
                } else {
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted, "integer op with non-integer type");
                }
                break;
            }
            case Opcode::Div: {
                if(other.type() == PrimeVType::Int64) {
                    return Value(_i64 / other.intValue());
                }
                else if(other.type() == PrimeVType::Float64) {
                    return Value(_i64 / other.floatValue());
                } else {
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted, "integer op with non-integer type");
                }
                break;
            }
            case Opcode::Assign: {
                if(other.type() == PrimeVType::Int64) {
                    _i64 = other.intValue();
                }
                else if(other.type() == PrimeVType::Float64) {
                    _i64 = other.floatValue();
                } else {
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted, "integer op with non-integer type");
                }
                return *this;
                break;
            }
            case Opcode::Less: {
                if(other.type() == PrimeVType::Int64) {
                    return Value(_i64 < other.intValue());
                }
                else if(other.type() == PrimeVType::Float64) {
                    return Value(_i64 < other.floatValue());
                } else {
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted, "integer op with non-integer type");
                }
                break;
            }
            case Opcode::Greater: {
                if(other.type() == PrimeVType::Int64) {
                    return Value(_i64 > other.intValue());
                }
                else if(other.type() == PrimeVType::Float64) {
                    return Value(_i64 > other.floatValue());
                } else {
                    throw DumpException(env, ExecutionError::BinaryOpNotPermitted, "integer op with non-integer type");
                }
                break;
            }
            default: {
                throw DumpException(env, ExecutionError::BinaryOpNotPermitted);
            }
        }
    }

}