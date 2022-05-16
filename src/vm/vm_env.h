#pragma once
#include "vm_object.h"
#include "vm_module.h"
#include "../name_pool.h"
#include <functional>
#include <map>

namespace compiler {
    using Name = ksgw::Name;
    using NamePool = ksgw::NamePool;
    using namespace ast;

    using TraverseCallBack = std::function<void(Node const*)>;

    class Module;

    class Env {
        friend class Module;
    private:
        struct FuncEnv {
            Value               vt;     // variable table
            Function*        func;   // function ast node
        };
    private:
        NamePool                                _namePool;
        Value                                   _package;
        std::vector<FuncEnv>                    _funcEnvs;
        std::vector<SymbolLayout*>              _symbolLayouts;

        std::map<Name, Module*, Name::FastLess> _modules;
    private:
    private:
        // utility functions
        Value rootPackage() { return _package; }
        FuncEnv const* funcEnv() { return &_funcEnvs.back(); }
        Value preparePackage(Node* ast);
        struct IdLocateEnv {
            SymbolLayout*   functionLayout;     // local symbol layout
            SymbolLayout*   packageLayout;      // current package symbol layout
            SymbolLayout*   classLayout;        // class symbol layout  
        };
        bool locateIdentifier(IdLocateEnv env, Identifier const* id) ;
        void traverseAST(Node const* ast, TraverseCallBack& callBack);
        std::vector<Token> postprocessFunction(Function* ast, IdLocateEnv env);
    public:

        Env() {
            auto layout = newSymbolLayout(SymbolLayoutType::Package);
            _package = Value(layout);
            compiler::keywords::init(this);
        }

        SymbolLayout* newSymbolLayout(SymbolLayoutType type) {
            auto symLayout = new SymbolLayout(type);
            _symbolLayouts.push_back(symLayout);
            return symLayout;
        }

        Name createName(char const* str);

        Module* getModule(Name const& name);

        bool compileCodeChunk(char const* module, Node* ast);
        bool postprocessModule(char const* module);
        void initializeModule(char const* module);
        Value callFunction(Value const& func, std::vector<Value> const& args);

        /**
         * @brief only for test
         * 
         * @param func 
         * @return Value 
         */
        Value callFunction(std::string func);

        /**
         * @brief 计算一个节点的值
        **/
        Value eval(Node const* ast);

        /**
         * @brief 
         *   二元表达式有点特殊，它是少数直接跟值打交道的，所以单独拿出来实现了
         * @param op 
         * @return Value 
         */
        Value evalBinaryOp(Token op, Value a, Value b);

        /**
         * @brief 
         *   计算一个标识符的值，是某个已经存在的于变量表里的变量引用
         * @param id 
         * @return Value 
         */
        Value evalIdentifier(Identifier const* id);

    };
}