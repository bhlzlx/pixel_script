#pragma once
#include "../ast_node.h"
#include "vm_types.h"

namespace compiler {

    class Symbol {
    private:
        SymbolType          _type;
        Name                _moduleName;
    public:
        Symbol(SymbolType type, Name moduleName)
            : _type(type)
            , _moduleName(moduleName)
        {
        }

        SymbolType type() const {
            return _type;
        }

        Name moduleName() const {
            return _moduleName;
        }
    };


    class SymbolLayout {
    public:
        struct Item {
            Symbol      symbol;
            Value       value;
        };
        struct QueryResult {
            Item*    item;
            uint32_t loc;
        };
    private:
        SymbolLayoutType        _type;
        std::vector<Item>       _symbols;
        /** 
         *  这里存的是跟_symbols对应的值，
         * 对应的值是函数的时候存对应的节点，这么做的原因是，我们把只读的信息放到Layout里，
         * 这里在使用点语法修改的时候，我们返回是这这里的值对象的拷贝，这样就不会影响代码的内容，
         * 如果是值类型，在Object层判断出来，返回Object里的值引用，这样就能对Object属性赋值了
         * 而且这样，Object里只存variable信息就行了，也省了很多内存
         */
        // std::vector<Value>                      _values;
        std::unordered_map<Name, uint32_t>      _locMap;
        uint32_t                _varCount;
    public:
        SymbolLayout(SymbolLayoutType type)
            : _type(type)
            , _symbols()
            , _locMap()
            , _varCount(0)
        {}

        QueryResult regSymbol(Name name, SymbolType type, Value val, Name moduleName);

        // reorder symbols for class object layout
        void reorderSymbols();

        uint32_t querySymbolLoc(Name name);

        QueryResult querySymbol(Name name);

        Value at(uint32_t loc) {
            return _symbols[loc].value;
        }

        SymbolLayoutType type() const {
            return _type;
        }
        uint32_t size() const;
        uint32_t varCount() const;

        std::vector<Item> const& symbols() const {
            return _symbols;
        }
    };

}