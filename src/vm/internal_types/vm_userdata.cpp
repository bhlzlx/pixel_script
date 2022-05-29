#include "vm_userdata.h"
#include "../vm_env.h"

namespace compiler {

    void UserdataCallFunction(Env* env, Value ud, Name methodName, int argc) {
        auto userdata = ud.ud();
        auto method = userdata->getFunction(methodName);
        if (method == nullptr) {
            throw DumpException(env, ExecutionError::MethodNotFound, "method not found!");
        }
        auto &stackFrames = env->stackFrames();
        stackFrames.push(ud); // push self
        stackFrames.precall(method, argc);
        method(env);
        Value ret = stackFrames.topLocalRef(0);
        stackFrames.popFrame();
        stackFrames.push(ret);
    }

    void UserdataGetIndexed(Env* env, Value ud, uint32_t index) {
        UserdataObject* obj = ud.ud();
        auto method = obj->getFunction(lib_keywords::___index);
        if(!method) {
            throw DumpException(env, ExecutionError::MethodNotFound, "__index method not found!");
        }
        auto &stackFrames = env->stackFrames();
        stackFrames.push(Value((int64_t)index));
        stackFrames.push(ud);
        stackFrames.precall(method, 1);
        Value rst = env->stackFrames().topLocalRef(0);
        stackFrames.popFrame();
        stackFrames.push(rst);
    }

    void UserdataGetField(Env* env, Value ud, Name name) {
        auto& stackFrames = env->stackFrames();
        UserdataObject* obj = ud.ud();
        auto func = obj->getFunction(name);
        if(func) {
            stackFrames.push(Value(func));
            return;
        }
        auto method = obj->getFunction(lib_keywords::___field);
        if(!method) {
            stackFrames.push(Value());
            throw DumpException(env, ExecutionError::MethodNotFound, "__field method not found!");
        }
        stackFrames.push(Value(name));
        stackFrames.push(ud);
        stackFrames.precall(method, 1);
        method(env);
        Value rst = env->stackFrames().topLocalRef(0);
        stackFrames.popFrame();
        stackFrames.push(rst);
    }

}