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
        Object(SymbolLayout* symbolLayout)
            : _type(symbolLayout->type())
            , _refCount(1)
            , _symbolLayout(symbolLayout)
            , _members(symbolLayout->varCount())
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

        Value getValue(Name name) const {
            Value nil;
            SymbolLayout::QueryResult rst = _symbolLayout->querySymbol(name);
            if(!rst.item) {
                return nil;
            }
            // 在给一个对象成员赋值的时候，只有成员变量是可以操作的，这时我们给它返回一个引用类的值类型
            if(rst.item->symbol.type() == SymbolType::Variable) {
                Value const* valPtr = &_members[rst.loc];
                return Value(const_cast<Value*>(valPtr)); // creat a value reference
            } else {
                // 否则，直接返回一个值拷贝，这样给一个对象的成员函数赋值时就不会出错
                return rst.item->value;
            }
        }

        Value at(uint32_t loc) const {
            if(loc >= _members.size()) {
                return _symbolLayout->at(loc);
            }
            return const_cast<Value*>(&_members[loc]);
        }

        Value operator[](Name name) const {
            auto loc = _symbolLayout->querySymbolLoc(name);
            if(loc == ~0UL) {
                return Value();
            }
            return at(loc);
        }

        uint32_t size() const {
            return _members.size();
        }

        SymbolLayout* symbolLayout() const {
            return _symbolLayout;
        }

        SymbolLayout::QueryResult addSymbol(Name name, SymbolType type, Value value, Name moduleName) {
            auto rst = _symbolLayout->regSymbol(name, type, value, Name());
            if(type == SymbolType::Variable) {
                _members.push_back(value);
            }
            return rst;
        }

    };

}