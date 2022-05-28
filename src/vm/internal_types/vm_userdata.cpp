#include "vm_userdata.h"
#include "../vm_env.h"

namespace compiler {

    void callUserdataMethod(Env* env, Value ud, Name methodName, int argc) {
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

}