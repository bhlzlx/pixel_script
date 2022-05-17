#include "string_impl.h"

namespace compiler {

    namespace string_impl {

        std::string toString(Value const& value) {
            switch(value.type()) {
                case PrimeVType::Nil:
                    return "{nil}";
                case PrimeVType::Boolean:
                    return value.booleanValue() ? "true" : "false";
                case PrimeVType::Int64:
                    return std::to_string(value.intValue());
                case PrimeVType::Float64:
                    return std::to_string(value.floatValue());
                case PrimeVType::String: {
                    Name* name = (Name*)value.ud()->ptr();
                    std::string str = name->text();
                    return str;
                }
                case PrimeVType::Object: {
                    Object* obj = value.asObject();
                    return "{object}";
                }
                case PrimeVType::Userdata: {
                    UserdataObject* ud = value.ud();
                    // ud->callMemberMethod();
                    return "{userdata}";
                }
            }
        }

        Value string_append(std::vector<Value> const& args) {
            assert(args.size());
            auto& self = args.back();
            auto& other = args.front();
        }

    }

}