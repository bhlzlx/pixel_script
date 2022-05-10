#pragma once
#include "vm_object.h"
#include "../name_pool.h"
#include <functional>

namespace compiler {
    using Name = ksgw::Name;
    using NamePool = ksgw::NamePool;
    using namespace ast;

    using TraverseCallBack = std::function<void(Node const*)>;

    class Env {
    public:
    private:
        NamePool                                _namePool;
        std::vector<SymbolLayout*>              _symbolLayouts;
        Value                                   _package;
        std::vector<Value>                      _stackFrame;
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

        Name getName(char const* str);

        Value rootPackage() {
            return _package;
        }

        bool compileCodeChunk(char const* module, Node* ast);

        Value currentFrame() {
            return _stackFrame.back();
        }

        Value preparePackage(Node* ast);

        struct IdentifierLocatorEnv {
            SymbolLayout*   functionLayout; // local symbol layout
            SymbolLayout*   packageLayout;    // local symbol layout
        };

        bool locateIdentifier(IdentifierLocatorEnv env, ASTIdentifier const* id) ;
        
        std::vector<Token> compileFunction(ASTFunction* ast);

        void traverseAST(Node const* ast, TraverseCallBack& callBack);

        Value eval(Node const* ast);

        Value evalBinaryOp(Token op, Value const* a, Value const* b);

        Value callFunction(Value const& func, std::vector<Value> const& args);

        Value callFunction(std::string func); // test

    };
}