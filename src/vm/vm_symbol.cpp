#include "vm_symbol.h"

namespace compiler {
    
    std::pair<bool, uint32_t> SymbolLayout::regSymbol(Name name, SymbolType type, Name moduleName) {
        auto iter = _locMap.find(name);
        if(iter == _locMap.end()) {
            _symbols.push_back({type, moduleName});
            _locMap[name] = _symbols.size() - 1;
            return {true, _symbols.size() - 1};
        }
        return {false, iter->second};
    }

    uint32_t SymbolLayout::querySymbolLoc(Name name) {
        auto iter = _locMap.find(name);
        if(iter == _locMap.end()) {
            return ~0UL;
        }
        return iter->second;
    }

    Symbol* SymbolLayout::querySymbol(Name name) {
        auto loc = querySymbolLoc(name);
        if(loc == ~0UL) {
            return nullptr;
        }
        return &_symbols[loc];
    }

    size_t SymbolLayout::size() const {
        return _symbols.size();
    }
}