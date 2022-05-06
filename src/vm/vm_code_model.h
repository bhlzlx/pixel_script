# pragma once

#include "../AST.h"
#include <unordered_map>
#include <functional>
#include "vm_types.h"

namespace compiler {

    enum class CompileResult {
        Success = 0,
        UndefinedVariable,
    };

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

        bool regSymbol(Name name, SymbolType type, ASTNode const* astNode, Value value, Name moduleName) {
            auto iter = _locMap.find(name);
            if(iter == _locMap.end()) {
                _symbols.push_back({type, astNode, value, moduleName});
                _locMap[name] = _symbols.size() - 1;
                return true;
            }
            return false;
        }

        uint32_t querySymbolLoc(Name name) {
            auto iter = _locMap.find(name);
            if(iter == _locMap.end()) {
                return ~0UL;
            }
            return iter->second;
        }

        Symbol* querySymbol(Name name) {
            auto loc = querySymbolLoc(name);
            if(loc == ~0UL) {
                return nullptr;
            }
            return &_symbols[loc];
        }

        size_t size() const {
            return _symbols.size();
        }
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

    using TraverseCallBack = std::function<void(ASTNode const*)>;

    class Env {
    private:
        SymbolLayout*                           _rootPackLayout;
        Package*                                _rootPackage;
    public:
        Env() {
            _rootPackLayout = new SymbolLayout();
            _rootPackage = new Package(_rootPackLayout);
        }

        Package* rootPackage() {
            return _rootPackage;
        }

        bool compileCodeChunk(Name module, ASTNode* ast);

        Package* preparePackage(ASTNode* ast);


        struct IdentifierLocatorEnv {
            SymbolLayout*   layout; // local symbol layout
            Package*        package; // current package
        };

        bool locateIdentifier(IdentifierLocatorEnv env, ASTIdentifier const* id) ;
        /**
         * @brief 
         * 
         * @param module 
         * @param ast 
         * @return true 
         * @return false 
         * 符号表：
         *  1. 参数列表
         *  2. ASTVariable
         * 找到就行了
         * 引用符号：
         *  1. 二元操作符两边的
         *  2. 
         */
        
        std::vector<ASTIdentifier*> compileFunction(Name module, ASTNode* ast);

        void traverseAST(ASTNode const* ast, TraverseCallBack& callBack);

    };

}