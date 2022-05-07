#include "vm_object.h"
#include "vm_types.h"
#include <exception>

namespace compiler {

    Value* Value::operator[](Name name)  {
        if(this->_type == ValueType::Object) {
            Object* obj = (Object*)_ud;
            return obj->operator[](name);
        }
        else {
            return nullptr;
        }
    }

    Value const* Value::operator[](Name name) const {
        if(this->_type == ValueType::Object) {
            Object const* obj = (Object*)_ud;
            return &obj->operator[](name);
        }
        else {
            return nullptr;
        }
    }
}