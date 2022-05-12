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
    private:
        struct FuncEnv {
            Value               vt;     // variable table
            ASTFunction*        func;   // function ast node
        };
    private:
        NamePool                                _namePool;
        Value                                   _package;
        std::vector<FuncEnv>                    _funcEnvs;
        std::vector<SymbolLayout*>              _symbolLayouts;

        std::map<Name, Module*, Name::FastLess> _modules;
    private:
        SymbolLayout* newSymbolLayout() {
            auto symLayout = new SymbolLayout();
            _symbolLayouts.push_back(symLayout);
            return symLayout;
        }
    public:
        Env() {
            auto layout = newSymbolLayout();
            _package = Value(layout);
            //
            compiler::keywords::init(this);
        }

        void initializeModule(char const* module);

        Name getName(char const* str);

        Value rootPackage() {
            return _package;
        }

        Module* getModule(Name const& name) {
            auto it = _modules.find(name);
            if (it != _modules.end()) {
                return it->second;
            } else {
                Module* mod = new Module();
                auto rst = _modules.insert(std::make_pair(name, mod));
                return rst.first->second;
            }
        }

        bool compileCodeChunk(char const* module, Node* ast);

        FuncEnv const* funcEnv() {
            return &_funcEnvs.back();
        }

        Value preparePackage(Node* ast);

        struct IdLocateEnv {
            SymbolLayout*   functionLayout;     // local symbol layout
            SymbolLayout*   packageLayout;      // local symbol layout
        };

        bool locateIdentifier(IdLocateEnv env, ASTIdentifier const* id) ;
        
        std::vector<Token> compileFunction(ASTFunction* ast);

        void traverseAST(Node const* ast, TraverseCallBack& callBack);

        Value callFunction(Value const& func, std::vector<Value> const& args);

        Value callFunction(std::string func); // test

        // eval functions
        /**
         * @brief evalAST
         * @param ast
         * 
        **/
        Value eval(Node const* ast);
        Value evalBinaryOp(Token op, Value a, Value b);
        Value evalIdentifier(ASTIdentifier const* id);
        // Value* evalId(Value const& value);

    };
}