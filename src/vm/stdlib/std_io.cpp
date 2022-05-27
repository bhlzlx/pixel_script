#include "std_io.h"
#include "../vm_env.h"
#include "../utils/vm_package_util.h"

namespace compiler {

    namespace io { 

        int print(Env* env) {
            auto& stack = env->stackFrames();
            auto argCount = stack.argCount();
            for(uint32_t i = 0; i < argCount; ++i) {
                auto value = stack.local(i);
                auto str = valueToString(env, value);
                printf("%s", str.c_str());
            }
            return 0;
        }

        BridgeFuncPair regItems[] = {
            {print, "print"}
        };

        bool init(Env* env) {
            auto root = env->root();
            Value io = prepareForPackage(env, "std.io");
            auto obj = io.asObject();
            for(auto& item: regItems) {
                auto rst = obj->addSymbol(env->createName(item.name), SymbolType::Function, Value(print), Name());
                if(!rst.item) {
                    return false;
                }
            }
            return true;
        }

    }

}