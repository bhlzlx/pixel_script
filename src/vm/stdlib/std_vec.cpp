#include "std_vec.h"
#include <vm/internal_types/vm_userdata.h>
#include <vm/vm_env.h>

namespace compiler {

    namespace std_vec_impl {

        using StdVector = std::vector<Value>;

        UserdataLayout* vectorLayout = nullptr;


        int __push_back(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.argCount();
            if(argCount != 1) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "push_back");
                throw except;
            }
            auto selfVal = stackFrames.selfRef();
            selfVal.deref();
            UserdataObject* self = selfVal.ud();
            StdVector* vec = (StdVector*)self->ptr();
            Value val = stackFrames.local(0);
            val.deref();
            vec->push_back(val);
            return 0;
        }

        int __at(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.argCount();
            if(argCount != 1) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "at");
                throw except;
            }
            auto selfVal = stackFrames.selfRef();
            selfVal.deref();
            UserdataObject* self = selfVal.ud();
            StdVector* vec = (StdVector*)self->ptr();
            Value val = stackFrames.local(0);
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
            stackFrames.push(ret);
            return 1;
        }

        int __erase(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.argCount();
            if(argCount != 1) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "erase");
                throw except;
            }
            auto selfVal = stackFrames.selfRef();
            selfVal.deref();
            UserdataObject* self = selfVal.ud();
            StdVector* vec = (StdVector*)self->ptr();
            Value val = stackFrames.local(0);
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
            Value ret((int64_t)vec->size());
            stackFrames.push(ret);
            return 1;
        }

        int __size(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.argCount();
            if(argCount != 0) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "size");
                throw except;
            }
            auto selfVal = stackFrames.selfRef();
            selfVal.deref();
            UserdataObject* self = selfVal.ud();
            StdVector* vec = (StdVector*)self->ptr();
            stackFrames.push((int64_t)vec->size());
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

        int create(Env* env) {
            assert(vectorLayout && "vectorLayout is null");
            StdVector* vec = new StdVector();
            UserdataObject* obj = new UserdataObject(vec, vectorLayout);
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.argCount();
            for(uint32_t i = 0; i<argCount; ++i ) {
                Value val = stackFrames.local(i);
                val.deref();
                vec->push_back(val);
            }
            stackFrames.push(Value(obj));
            return 1;
        }

    }

}