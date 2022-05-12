# pragma once

#include "../ast_node.h"
#include <unordered_map>
#include "vm_types.h"
#include "vm_symbol.h"

namespace compiler {

    enum class CompileResult {
        Success = 0,
        UndefinedVariable,
    };

    enum class ObjectType {
        Package,
        GeneralValue,
        UserData,
    };

    class Object {
    protected:
        ObjectType              _type;
        uint32_t                _refCount;           
        SymbolLayout*           _symbolLayout;
        std::vector<Value>      _members;
    public:
        Object(SymbolLayout* symbolLayout, ObjectType type = ObjectType::GeneralValue)
            : _type(type)
            , _refCount(1)
            , _symbolLayout(symbolLayout)
            , _members(symbolLayout->size())
        {
        }

        void incRef() {
            ++_refCount;
        }

        void decRef() {
            --_refCount;
            if(_refCount == 0) {
                delete this;
            }
        }

        Value* getValue(Name name) {
            auto loc = _symbolLayout->querySymbolLoc(name);
            if(~loc == 0) {
                return nullptr;
            }
            return &_members[loc];
        }

        Value* at(uint32_t loc) {
            return &_members[loc];
        }

        Value const* at(uint32_t loc) const {
            return &_members[loc];
        }

        Value* operator[](Name name) {
            return getValue(name);
        }

        Value const& operator[](Name name) const {
            auto loc = _symbolLayout->querySymbolLoc(name);
            if(loc == ~0UL) {
                return _members.back();
            }
            return _members[loc];
        }

        uint32_t size() const {
            return _members.size();
        }

        SymbolLayout* symbolLayout() const {
            return _symbolLayout;
        }

        std::pair<bool, uint32_t> addSymbol(Name name, SymbolType type, Value value, Name moduleName) {
            auto rst = _symbolLayout->regSymbol(name, type, Name());
            if(rst.first) {
                _members.push_back(value);
            }
            return rst;
        }
    };

    // class Package: public Object {
    // private:
    //     std::vector<SymbolLayout*> _subpackLayouts;
    // public:
    //     Package(SymbolLayout* layout) 
    //         : Object( layout, ObjectType::Package)
    //         , _subpackLayouts()
    //     {}

    //     bool compileModule(Name name, Node* ast);

    //     void unloadModule(Name name);

    //     Package* prepareSubPackage(Name name);
    // };

}