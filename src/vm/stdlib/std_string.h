#pragma once
#include "../../name_pool.h"
#include "../vm_object.h"
#include "../vm_types.h"
#include "../vm_userdata.h"
#include <initializer_list>
#include <functional>
#include <vector>
#include <map>

namespace compiler {

    namespace string_impl {

        std::string toString(Value const& value) ;

        int string_append(Env* env);
        int string_length(Env* env);
        int console_log(Env* env);

        extern UserdataLayout* stringLayout;

        Value createString(Env* env, char const* str);        

        void initString(Env* env);

    }



}