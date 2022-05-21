#pragma once
#include <compiler_common.h>

namespace compiler {

    namespace std_map_impl {

        UserdataObject* create(Env* env);

        void __privateAdd(UserdataObject* vec, Value const& key, Value const& val);

        void init(Env* env);

    }


}