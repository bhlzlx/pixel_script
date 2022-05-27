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
                Object* obj = value.asObject();
                auto field = value[lib_keywords::___tostring];
                if(field) {
                    Value rst;
                    auto& stackFrames = env->stackFrames();
                    stackFrames.pushFrame();
                    // 没有参数，所以只压一个self
                    stackFrames.push(value);
                    auto func = value[field];
                    auto bridgeFunc = func.asBytecodeFunc();
                    // auto ret = byteFunc(env, 0);
                    // if(ret) {
                    //     rst = stackFrames.topLocal(0);
                    // } else {
                    //     rst = Value();
                    // }
                    stackFrames.popFrame();
                    return valueToString(env, rst);
                } else {
                    char buf[32] = {};
                    sprintf(buf, "{ object: %p }", value.asObject());
                    return buf;
                }
            }
            case PrimeVType::Userdata: {
                UserdataObject* ud = value.ud();
                Value rst;
                // env->stackFrames().pushArgBegin(); {
                //     env->stackFrames().pushValue(value); // 传self
                //     env->stackFrames().pushArgEnd();
                //     auto ret = ud->callMemberMethod(env,lib_keywords::___tostring);
                //     if(ret) {
                //         rst = env->stackFrames().popValue();
                //     }
                // }
                // env->stackFrames().popToArgBegin();
                if(rst) {
                    return valueToString(env, rst);
                }
                char buf[32] = {};
                sprintf(buf, "{ userdata: %p }", value.ud());
                return buf;
            }
            default: {
                break;
            }
        }
        return "";
    }

}