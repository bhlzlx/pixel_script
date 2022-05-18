# pragma once

#include "../ast_node.h"
#include <unordered_map>
#include "vm_types.h"
#include "vm_symbol.h"
#include <vector>

namespace compiler {

    enum class CompileResult {
        Success = 0,
        UndefinedVariable,
    };

    class Object {
        friend class Value;
    protected:
        SymbolLayoutType        _type;
        uint32_t                _refCount;           
        SymbolLayout*           _symbolLayout;
        std::vector<Value>      _members;
    public:
        Object(SymbolLayout* symbolLayout);

        void incRef() {
            ++_refCount;
        }

        void decRef() {
            --_refCount;
            if(_refCount == 0) {
                delete this;
            }
        }

        Value getValue(Name name) const;

        Value at(uint32_t loc) const;

        Value operator[](Name name) const;

        uint32_t size() const {
            return _members.size();
        }

        std::string toString() const;

        SymbolLayout* symbolLayout() const {
            return _symbolLayout;
        }

        SymbolLayout::QueryResult addSymbol(Name name, SymbolType type, Value value, Name moduleName);

    };

}