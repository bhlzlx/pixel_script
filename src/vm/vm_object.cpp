#include <cassert>
#include "vm_object.h"
#include "../AST.h"

namespace compiler {

    bool Package::compileModule(Name name, ASTNode* ast) {
        assert(ast->type() == ASTNodeType::CodeChunk);
        ASTMultiExpr* multiExpr = (ASTMultiExpr*)ast;
        for(auto expr : multiExpr->expressions()) {
            if(expr->type() == ASTNodeType::Function) {
                ASTFunction* func = (ASTFunction*)expr;
                auto rst = this->_symbolLayout->regSymbol(func->name().stringLiteral(), SymbolType::Function, func, Value(), name);
                if(!rst) {
                    assert(false);
                    return false;
                }
            } else if( expr->type() == ASTNodeType::Variable ) {
                ASTVariable* var = (ASTVariable*)expr;
                auto rst = _symbolLayout->regSymbol(var->name().stringLiteral(), SymbolType::Variable, var, Value(), name);
                if(!rst) {
                    assert(false);
                    return false;
                }
            } else {
                assert(false && "only function & variable can be defined in package");
                return false;
            }
        }
        return true;
    }

    void Package::unloadModule(Name name) {
    }

    Package* Package::prepareSubPackage(Name name) {
        auto rst = _symbolLayout->regSymbol(name, SymbolType::Package, nullptr, Value(), Name());
        Symbol* symbol = _symbolLayout->querySymbol(name);
        if(rst) {
            auto subpackLayout = new SymbolLayout();
            symbol->value().setUd(new Package(subpackLayout));
        }
        auto &value = symbol->value();
        auto pack = (Package*)(value.ud());
        return pack;
    }


}