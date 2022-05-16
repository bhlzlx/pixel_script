#include "vm_symbol.h"
#include <algorithm>

namespace compiler {
    
    SymbolLayout::QueryResult SymbolLayout::regSymbol(Name name, SymbolType type, Value val, Name moduleName) {
        auto iter = _locMap.find(name);
        if(iter == _locMap.end()) {
            _symbols.push_back(
                {
                    { type, moduleName },
                    val
                }
            );
            _locMap[name] = _symbols.size() - 1;
            if(type == SymbolType::Variable) {
                _varCount++;
            }
            return {&_symbols.back(), (uint32_t)_symbols.size() - 1};
        }
        return {&_symbols[iter->second], iter->second};
    }

    uint32_t SymbolLayout::querySymbolLoc(Name name) {
        auto iter = _locMap.find(name);
        if(iter == _locMap.end()) {
            return ~0UL;
        }
        return iter->second;
    }

    SymbolLayout::QueryResult SymbolLayout::querySymbol(Name name) {
        auto loc = querySymbolLoc(name);
        if(loc == ~0UL) {
            return {nullptr, loc};
        }
        return { &_symbols[loc], loc };
    }

    uint32_t SymbolLayout::size() const {
        return _symbols.size();
    }

    uint32_t SymbolLayout::varCount() const {
        return _varCount;
    }

    void SymbolLayout::reorderSymbols() {
        std::vector<std::pair<Item*, Name>> symbols;
        for(auto& p : _locMap) {
            Item* item = &_symbols[p.second];
            Name name = p.first;
            symbols.push_back({item, name});
        }
        std::sort(symbols.begin(), symbols.end(), [](auto&lhs, auto& rhs) {
            return lhs.first->symbol.type() < rhs.first->symbol.type();
        });
        _locMap.clear();
        for(uint32_t i = 0; i<symbols.size(); ++i) {
            _locMap.insert({symbols[i].second, i });
        }
        std::vector<Item> reordered;
        for(auto& p : symbols) {
            reordered.push_back(*p.first);
        }
        _symbols = std::move(reordered);
    }
}