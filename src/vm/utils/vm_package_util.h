#pragma once
#include <compiler_common.h>

namespace compiler {

    Value prepareForPackage(Env* env, char const* path);

    std::string valueToString( Env* env, Value const& value );

}