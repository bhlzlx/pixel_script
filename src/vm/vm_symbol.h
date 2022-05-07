#pragma once
#include "../AST.h"
#include "vm_types.h"

namespace compiler {

    enum class SymbolType {
        Variable,
        Package,
        Function,
        Class
    };

    class Symbol {
    private:
        SymbolType          _type;
        ASTNode const*      _astNode;
        Value               _value;
        Name                _moduleName;
    public:
        Symbol(SymbolType type, ASTNode const* astNode, Value value, Name moduleName)
            : _type(type)
            , _astNode(astNode)
            , _value(value)
            , _moduleName(moduleName)
        {
        }

        Value& value() {
            return _value;
        }

        ASTNode const* astNode() const{
            return _astNode;
        }

        SymbolType type() const {
            return _type;
        }

        Name moduleName() const {
            return _moduleName;
        }
    };

    class SymbolLayout {
    private:
        std::vector<Symbol>                 _symbols;
        std::unordered_map<Name, uint32_t>  _locMap;
    public:
        SymbolLayout() {}

        bool regSymbol(Name name, SymbolType type, ASTNode const* astNode, Value value, Name moduleName);

        uint32_t querySymbolLoc(Name name);

        Symbol* querySymbol(Name name);

        size_t size() const;
    };

}