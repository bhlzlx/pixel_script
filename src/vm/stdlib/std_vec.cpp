#include "std_vec.h"
#include "../vm_userdata.h"
#include "../vm_env.h"

namespace compiler {

    namespace std_vec_impl {

        using StdVector = std::vector<Value>;

        UserdataLayout* vectorLayout = nullptr;

        int __push_back(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.topFrameSize();
            if(argCount != 2) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "push_back");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.localValue(0).ud();
            StdVector* vec = (StdVector*)self->ptr();
            Value val = stackFrames.localValue(1);
            val.deref();
            vec->push_back(val);
            return 0;
        }

        int __at(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.topFrameSize();
            if(argCount != 2) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "at");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.localValue(0).ud();
            StdVector* vec = (StdVector*)self->ptr();
            Value val = stackFrames.localValue(1);
            val.deref();
            if(val.type() != PrimeVType::Int64) {
                DumpException except(env, ExecutionError::ArgumentTypeMismatch, "at");
                throw except;
            }
            int index = val.intValue();
            if(index < 0 || index >= (int)vec->size()) {
                DumpException except(env, ExecutionError::IndexOutOfRange, "at");
                throw except;
            }
            Value ret = &vec->at(index); // 返回引用，这样就能进行赋值了！细节细节！
            stackFrames.pushValue(ret, true);
            return 1;
        }

        int __erase(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.topFrameSize();
            if(argCount != 2) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "erase");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.localValue(0).ud();
            StdVector* vec = (StdVector*)self->ptr();
            Value val = stackFrames.localValue(1);
            val.deref();
            if(val.type() != PrimeVType::Int64) {
                DumpException except(env, ExecutionError::ArgumentTypeMismatch, "erase");
                throw except;
            }
            int index = val.intValue();
            if(index < 0 || index >= (int)vec->size()) {
                DumpException except(env, ExecutionError::IndexOutOfRange, "erase");
                throw except;
            }
            vec->erase(vec->begin() + index);
            Value ret(vec->size());
            stackFrames.pushValue(ret);
            return 1;
        }

        int __size(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.topFrameSize();
            if(argCount != 1) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "size");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.localValue(0).ud();
            StdVector* vec = (StdVector*)self->ptr();
            stackFrames.pushValue(Value(vec->size()));
            return 1;
        }

        BridgeFuncPair regItems[] = {
            { __push_back, "push_back" },
            { __at, "at" },
            { __at, "__index" },
            { __erase, "erase" },
            { __size, "size" },
        };

        void init(Env* env) {   
            if(vectorLayout == nullptr) {
                vectorLayout = new UserdataLayout();
                for(auto& item : regItems) {
                    vectorLayout->addBridgeFunc(env->createName(item.name), item.func);
                }
            }
        }

        UserdataObject* create(Env* env) {
            assert(vectorLayout && "vectorLayout is null");
            StdVector* vec = new StdVector();
            UserdataObject* obj = new UserdataObject(vec, vectorLayout);
            return obj;
        }

        void __privateAdd(UserdataObject* vec, Value const& val) {
            StdVector* obj = (StdVector*)vec->ptr();
            obj->push_back(val);
        }

    }

}