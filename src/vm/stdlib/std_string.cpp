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

        int __append(Env* env) {
            StackFrames& stack = env->stackFrames();
            size_t paramsCount = stack.topFrameSize();
            if(paramsCount < 2) {
                return 0;
            }
            Value first = stack.localValue(0);
            Value second = stack.localValue(1);
            if(second.type() != PrimeVType::String) {
                throw DumpException(env, ExecutionError::InvalidArgument, "append: second argument must be string");
                return 0;
            }
            Name a = first.stringValue();
            Name b = second.stringValue();
            std::string str = a.text();
            str.append(b.text());
            Name n = env->createName(str.c_str());
            Value rst = Value(n);
            stack.pushValue(rst);
            return 1;
        }

        int __len(Env* env) {
            StackFrames& stack = env->stackFrames();
            size_t paramsCount = stack.topFrameSize();
            if(paramsCount < 1) {
                return 0;
            }
            Value first = stack.localValue(0);
            size_t name = (size_t)first.ud()->ptr();
            Name* a = (Name*)&name;
            std::string str = a->text();
            Value rst;
            rst.setInt64(str.size());
            stack.pushValue(rst);
            return 1;
        }

        BridgeFuncPair stringRegItems[] = {
            {__append, "append"},
            {__len, "length"}
        };

        void init(Env* env) {
            if(stringLayout) {
                return;
            }
            stringLayout = new UserdataLayout();
            for(auto const& item: stringRegItems) {
                stringLayout->addBridgeFunc(env->createName(item.name), item.func); 
            }
        }

    }

}