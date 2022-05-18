#include "std_string.h"
#include "../vm_env.h"

namespace compiler {

    namespace string_impl {

        UserdataLayout* stringLayout = nullptr;

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
                default: {
                    break;
                }
            }
            return "";
        }

        int string_append(Env* env) {
            StackValues& stack = env->stackValues();
            size_t paramsCount = stack.topFrameSize();
            if(paramsCount < 2) {
                return 0;
            }
            Value first = stack.localValue(0);
            Value second = stack.localValue(1);
            size_t name = (size_t)first.ud()->ptr();
            Name* a = (Name*)&name;
            name = (size_t)second.ud()->ptr();
            Name* b = (Name*)&name;
            std::string str = a->text();
            str.append(b->text());
            Name n = env->createName(str.c_str());
            UserdataObject* ud = new UserdataObject((void*)(size_t)n, stringLayout);
            Value rst = Value(ud);
            stack.pushValue(rst);
            return 1;
        }

        void initString(Env* env) {
            if(stringLayout) {
                return;
            }
            stringLayout = new UserdataLayout();
            stringLayout->addBridgeFunc(env->createName("append"), string_append);
            stringLayout->addBridgeFunc(env->createName("length"), string_length);
        }

    }

}