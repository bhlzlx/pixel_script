#include "../vm_types.h"
#include "../vm_object.h"

namespace compiler {

    Value prepareForPackage(Env* env, char const* path);

    std::string valueToString( Env* env, Value const& value );

}