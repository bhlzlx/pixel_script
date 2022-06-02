#include "vm_package_util.h"
#include <vm/vm_env.h>
#include <vm/internal_types/vm_userdata.h>

namespace compiler {

   Value prepareForPackage(Env* env, char const* path) {
        auto pack = env->root();
        std::string packName;
        char const* pos = path;
        while(pos) {
            pos = strchr(path, '.');
            if(pos) {
                packName = std::string(path, pos);
                path = pos + 1;
            } else {
                packName = path;
            }
            auto name = env->createName(packName.c_str());
            auto rst = pack[name];
            if(rst && rst.stype() == SymbolLayoutType::Package) {
                pack = rst;
            } else {
                Object* obj = pack.asObject();
                auto symbolLayout = env->newSymbolLayout(SymbolLayoutType::Package);
                Value subPack(symbolLayout);
                obj->addSymbol(name, SymbolType::Package, subPack, Name());
                pack = subPack;
            }
        }
        return pack;
    }


    std::string valueToString( Env* env, Value const& value ) {
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
                return value.stringValue().text();
            }
            case PrimeVType::Object: {
                auto field = value[lib_keywords::___tostring];
                if(field) {
                    auto& stackFrames = env->stackFrames();
                    // 没有参数，所以只压一个self
                    stackFrames.push(value);
                    stackFrames.push(field);
                    env->call(0);
                    auto rst = stackFrames.retVal();
                    stackFrames.pop();
                    return valueToString(env, rst);
                } else {
                    // Object* obj = value.asObject();
                    char buf[32] = {};
                    sprintf(buf, "{ object: %p }", value.asObject());
                    return buf;
                }
            }
            case PrimeVType::Userdata: {
                auto field = value[lib_keywords::___tostring];
                if(field) {
                    auto& stackFrames = env->stackFrames();
                    // 没有参数，所以只压一个self
                    stackFrames.push(value);
                    stackFrames.push(field);
                    env->call(0);
                    auto rst = stackFrames.retVal();
                    stackFrames.pop();
                    return valueToString(env, rst);
                } else {
                    // UserdataObject* ud = value.ud();
                    char buf[32] = {};
                    sprintf(buf, "{ userdata: %p }", value.ud());
                    return buf;
                }
            }
            default: {
                break;
            }
        }
        return "";
    }

}