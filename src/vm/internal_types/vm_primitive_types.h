#pragma once
#include "vm_types.h"
#include "../vm_bytecode.h"

namespace compiler {

    class IntegerValue: public Value {
        IntegerValue() = delete; 
        IntegerValue(IntegerValue const&) = delete; 
    public:
        void Op(Env* env, Opcode op, Value const& other);
    };

    // class FloatValue: public Value {
    // public:
    //     FloatValue(double f64) {
    //         setFloat64(f64);
    //     }
    //     void Op( Env* env, Opcode op, Value const& other) const;
    // };

    // class StringValue: public Value {
    // public:
    //     StringValue() {
    //     }
    //     void Op(Env* env, Opcode op, Value const& other) const;
    //     // void callMethod(Name name, Env* env);
    // };

}