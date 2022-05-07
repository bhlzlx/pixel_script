#pragma once
#include "vm_object.h"
#include "../name_pool.h"
#include <functional>

namespace compiler {
    using Name = ksgw::Name;
    using NamePool = ksgw::NamePool;

    using TraverseCallBack = std::function<void(ASTNode const*)>;

    class Env {
    public:
    private:
        NamePool                                _namePool;
        SymbolLayout*                           _rootPackLayout;
        Package*                                _rootPackage;
    public:
        Env() {
            _rootPackLayout = new SymbolLayout();
            _rootPackage = new Package(_rootPackLayout);
        }

        Name getName(char const* str);

        Package* rootPackage() {
            return _rootPackage;
        }

        bool compileCodeChunk(char const* module, ASTNode* ast);

        Package* preparePackage(ASTNode* ast);


        struct IdentifierLocatorEnv {
            SymbolLayout*   layout; // local symbol layout
            Package*        package; // current package
        };

        bool locateIdentifier(IdentifierLocatorEnv env, ASTIdentifier const* id) ;
        
        std::vector<ASTIdentifier*> compileFunction(Name module, ASTNode* ast);

        void traverseAST(ASTNode const* ast, TraverseCallBack& callBack);

    };
}