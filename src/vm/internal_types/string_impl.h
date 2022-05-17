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

        using Name = ksgw::Name;

        std::string toString(Value const& value) ;

        Value string_append(Value const* args, uint32_t argc);
        Value string_length(std::vector<Value> const& args);


        Value console_log(std::vector<Value> const& args) {
        }


        Value createString(Env* env, char const* str);        

    }



}