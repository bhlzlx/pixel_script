#pragma once
#include "../ast_node.h"
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
        // Node const*         _astNode;
        // Value               _value;
        Name                _moduleName;
    public:
        Symbol(SymbolType type, Name moduleName)
            : _type(type)
            // , _astNode(astNode)
            // , _value(value)
            , _moduleName(moduleName)
        {
        }

        // Value& value() {
        //     return _value;
        // }

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

        std::pair<bool, uint32_t> regSymbol(Name name, SymbolType type, Name moduleName);

        uint32_t querySymbolLoc(Name name);

        Symbol* querySymbol(Name name);

        size_t size() const;
    };

}