#pragma once
#include "vm_types.h"

namespace compiler {

    class IntegerValue: public Value {
    public:
        IntegerValue(int64_t i64) {
            setInt64(i64);
        }
        Value Op(Env* env, Token op, Value const& other) const ;
    };

    class FloatValue: public Value {
    public:
        FloatValue(double f64) {
            setFloat64(f64);
        }
        Value Op( Env* env, Token op, Value const& other) const;
    };

}