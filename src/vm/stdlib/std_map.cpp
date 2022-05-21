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
            auto argCount = stackFrames.topFrameSize();
            if(argCount != 3) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "insert");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.localValue(0).ud();
            StdMap* map = (StdMap*)self->ptr();
            Value key = stackFrames.localValue(1);
            Value val = stackFrames.localValue(2);
            key.deref();
            val.deref();
            map->insert(std::make_pair(key, val));
            return 0;
        }

        int __erase(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.topFrameSize();
            if(argCount != 2) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "erase");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.localValue(0).ud();
            StdMap* map = (StdMap*)self->ptr();
            Value key = stackFrames.localValue(1);
            key.deref();
            auto it = map->find(key);
            if(it == map->end()) {
                return 0;
            }
            map->erase(it);
            return 0;
        }

        int __size(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.topFrameSize();
            if(argCount != 1) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "size");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.localValue(0).ud();
            StdMap* map = (StdMap*)self->ptr();
            Value ret(map->size());
            stackFrames.pushValue(ret);
            return 1;
        }

        int __clear(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.topFrameSize();
            if(argCount != 1) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "clear");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.localValue(0).ud();
            StdMap* map = (StdMap*)self->ptr();
            map->clear();
            return 0;
        }

        int __find(Env* env) {
            auto& stackFrames = env->stackFrames();
            auto argCount = stackFrames.topFrameSize();
            if(argCount != 2) {
                DumpException except(env, ExecutionError::ArgumentCountMismatch, "find");
                throw except;
            }
            UserdataObject* self = (UserdataObject*)stackFrames.localValue(0).ud();
            StdMap* map = (StdMap*)self->ptr();
            Value key = stackFrames.localValue(1);
            key.deref();
            auto it = map->find(key);
            if(it == map->end()) {
                DumpException except(env, ExecutionError::KeyNotFound, "find");
                throw except;
            }
            Value val = it->second;
            stackFrames.pushValue(val);
            return 1;
        }

        // generate reg items
        BridgeFuncPair regItems[] = {
            { __insert, "insert" },
            { __erase, "erase" },
            { __size, "size" },
            { __clear, "clear" },
            { __find, "find" },
        };

        UserdataObject* create(Env* env) {
            StdMap* map = new StdMap();
            UserdataObject* obj = new UserdataObject(map, mapLayout);
            return obj;
        }

        void init(Env* env) {
            if(mapLayout == nullptr) {
                mapLayout = new UserdataLayout();
                for(auto regItem : regItems) {
                    mapLayout->addBridgeFunc(env->createName(regItem.name), regItem.func);
                }
            }
        }

        void __privateAdd(UserdataObject* vec, Value const& key, Value const& val) {
            StdMap* map = (StdMap*)vec->ptr();
            map->insert(std::make_pair(key, val));
        }
    }

}