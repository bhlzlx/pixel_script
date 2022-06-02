#pragma once
#include <name_pool.h>
#include <vm/internal_types/vm_types.h>
#include <initializer_list>
#include <functional>
#include <vector>
#include <map>

namespace compiler {

    namespace string_impl {

        extern UserdataLayout* stringLayout;
        extern int __append(Env* env);
        void init(Env* env);

    }



}