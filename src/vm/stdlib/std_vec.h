#pragma once
#include "../../compiler_common.h"

namespace compiler {

    namespace std_vec_impl {

        UserdataObject* create(Env* env);

        void __privateAdd(UserdataObject* vec, Value const& val);

        void init(Env* env);

    }

}