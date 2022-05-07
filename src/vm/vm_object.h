# pragma once

#include "../AST.h"
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
        SymbolLayout*           _symbolLayout;
        std::vector<Value>      _members;
    public:
        Object(SymbolLayout* symbolLayout, ObjectType type = ObjectType::GeneralValue)
            : _type(type)
            , _symbolLayout(symbolLayout)
            , _members(symbolLayout->size())
        {
        }

        Value* getValue(Name name) {
            auto loc = _symbolLayout->querySymbolLoc(name);
            if(~loc == 0) {
                return nullptr;
            }
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
    };

    class Package: public Object {
    private:
        std::vector<SymbolLayout*> _subpackLayouts;
    public:
        Package(SymbolLayout* layout) 
            : Object( layout, ObjectType::Package)
            , _subpackLayouts()
        {}

        bool compileModule(Name name, ASTNode* ast);

        void unloadModule(Name name);

        Package* prepareSubPackage(Name name);
    };

}