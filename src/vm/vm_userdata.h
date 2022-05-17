#pragma once

#include "../ast_common.h"
#include "../name_pool.h"
#include "vm_object.h"

namespace compiler {

    using Name = ksgw::Name;

    using BridgeFunc = Value(*)(Value const* args, uint32_t argc);

    class UserdataLayout {
    private:
        std::vector<BridgeFunc>     _bridgeFuncs;
        std::map<Name, uint32_t>    _bridgeFuncIndexMap;
    public:
        UserdataLayout() {}

        void addBridgeFunc(Name name, BridgeFunc func) {
            auto iter = _bridgeFuncIndexMap.find(name);
            if(iter !=_bridgeFuncIndexMap.end()) {
                return;
            }
            _bridgeFuncs.push_back(func);
            _bridgeFuncIndexMap[name] = _bridgeFuncs.size() - 1;
        }

        BridgeFunc getFunction(Name name) const {
            auto iter = _bridgeFuncIndexMap.find(name);
            if(iter == _bridgeFuncIndexMap.end()) {
                return nullptr;
            } else {
                return _bridgeFuncs[iter->second];
            }
        }
    };

    class UserdataObject {
    private:
        UserdataLayout*     _layout;
        void*               _data;
        size_t              _ref;
    public:
        UserdataObject(void* ptr, UserdataLayout* layout)
            : _data(ptr)
            , _layout(layout)
        {}

        void incRef() {
            _ref++;
        }

        void decRef() {
            _ref--;
            if(_ref == 0) {
                delete this;
            }
        }

        Value callMemberMethod(Name methodName) {
            return Value();
        }

        void* ptr() const {
            return _data;
        }
    };

}