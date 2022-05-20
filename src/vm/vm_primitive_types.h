#pragma once
#include "vm_types.h"

namespace compiler {

    class IntegerValue: public Value {
        IntegerValue() = delete; 
        IntegerValue(IntegerValue const&) = delete; 
    public:
        Value Op(Env* env, Token op, Value const& other) const ;
    };

    class FloatValue: public Value {
    public:
        FloatValue(double f64) {
            setFloat64(f64);
        }
        Value Op( Env* env, Token op, Value const& other) const;
    };

    class StringValue: public Value {
    public:
        StringValue() {
        }
        Value Op(Env* env, Token op, Value const& other) const;
        Value callMethod(Name name, Env* env);
    };

}