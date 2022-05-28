#pragma once
#include "vm_types.h"
#include "../vm_bytecode.h"

namespace compiler {

    class IntegerValue: public Value {
        IntegerValue() = delete; 
        IntegerValue(IntegerValue const&) = delete; 
    public:
        Value Op(Env* env, Opcode op, Value const& other);
    };

    class FloatValue: public Value {
        FloatValue() = delete;
        FloatValue(FloatValue const&) = delete;
    public:
        Value Op( Env* env, Opcode op, Value const& other);
    };

    class StringValue: public Value {
        StringValue() = delete;
        StringValue(StringValue const&) = delete;
    public:
        Value Op(Env* env, Opcode op, Value const& other);
    };

}