#include "std_map.h"
#include <vm/internal_types/vm_userdata.h>
#include <vm/vm_env.h>

namespace compiler {

    namespace std_map_impl {

        // generate code like std_vec.cpp

        // define a userdata layout
        UserdataLayout* mapLayout = nullptr;

        using StdMap = std::map<Value, Value>;

        // generate map functions

        int __insert(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.argCount();
            if(argCount != 2) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "insert");
                throw except;
            }
            UserdataObject* self = stackFrames.self().ud();
            StdMap* map = (StdMap*)self->ptr();
            Value key = stackFrames.local(0);
            Value val = stackFrames.local(1);
            key.deref();
            val.deref();
            map->insert(std::make_pair(key, val));
            return 0;
        }

        int __erase(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.argCount();
            if(argCount != 1) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "erase");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.self().ud();
            StdMap* map = (StdMap*)self->ptr();
            Value key = stackFrames.local(0);
            key.deref();
            auto it = map->find(key);
            if(it != map->end()) {
                map->erase(it);
            }
            return 0;
        }

        int __size(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.argCount();
            if(argCount != 0) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "size");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.self().ud();
            StdMap* map = (StdMap*)self->ptr();
            Value ret((int64_t)map->size());
            stackFrames.push(ret);
            return 1;
        }

        int __clear(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.argCount();
            if(argCount != 1) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "clear");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.self().ud();
            StdMap* map = (StdMap*)self->ptr();
            map->clear();
            return 0;
        }

        int __find(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.argCount();
            if(argCount != 1) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "find");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.self().ud();
            StdMap* map = (StdMap*)self->ptr();
            Value key = stackFrames.local(0);
            key.deref();
            auto it = map->find(key);
            if(it == map->end()) {
                DumpException except(env, ExecutionError::KeyNotFound, "find");
                throw except;
            }
            Value val = it->second;
            stackFrames.push(val);
            return 1;
        }

        // generate reg items
        BridgeFuncPair regItems[] = {
            { __insert, "insert" },
            { __erase, "erase" },
            { __size, "size" },
            { __clear, "clear" },
            { __find, "find" },
            { __find, "__field" },
        };

        int create(Env* env) {
            assert(mapLayout && "vectorLayout is null");
            StdMap* map = new StdMap();
            UserdataObject* obj = new UserdataObject(map, mapLayout);
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.argCount();
            assert((argCount & 1) == 0);
            for(uint32_t i = 0; i<argCount; i+=2 ) {
                Value key = stackFrames.argAt(i); // derefed values
                Value val = stackFrames.argAt(i+1); // derefed values
                map->emplace(key, val);
            }
            stackFrames.push(Value(obj));
            return 1;
        }

        void init(Env* env) {
            if(mapLayout == nullptr) {
                mapLayout = new UserdataLayout();
                for(auto regItem : regItems) {
                    mapLayout->addBridgeFunc(env->createName(regItem.name), regItem.func);
                }
            }
        }

    }

}