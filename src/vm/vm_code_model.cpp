#include <cassert>
#include "vm_code_model.h"
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

    Package* Env::preparePackage( ASTNode* ast ) {
        assert(ast->type() == ASTNodeType::Package);
        ASTPackage* package = (ASTPackage*)ast;
        auto pack = rootPackage();
        for( auto name : package->names() ) {
            pack = pack->prepareSubPackage(name);
        }
        return pack;
    }

    void Env::traverseAST(ASTNode const* ast, TraverseCallBack& callBack) {
        switch(ast->type()) {
            case ASTNodeType::Function: {
                auto func = static_cast<ASTFunction const*>(ast);
                callBack(func->body());
                traverseAST(func->body(), callBack);
                break;
            }
            case ASTNodeType::Block: {
                auto block = static_cast<ASTMultiExpr const*>(ast);
                for(auto& expr : block->expressions()) {
                    callBack(expr);
                    traverseAST(expr, callBack);
                }
                break;
            }
            case ASTNodeType::While: {
                auto whileNode = static_cast<ASTWhileStatement const*>(ast);
                callBack(whileNode->condition());
                traverseAST(whileNode->condition(), callBack);
                callBack(whileNode->body());
                traverseAST(whileNode->body(), callBack);
                break;
            }
            case ASTNodeType::If: {
                auto ifNode = static_cast<ASTIfStatement const*>(ast);
                callBack(ifNode->condition());
                traverseAST(ifNode->condition(), callBack);
                callBack(ifNode->thenBranch());
                traverseAST(ifNode->thenBranch(), callBack);
                callBack(ifNode->elseBranch());
                traverseAST(ifNode->elseBranch(), callBack);
                break;
            }
            case ASTNodeType::BinaryOp: {
                auto binOp = static_cast<ASTBinaryOpExpr const*>(ast);
                callBack(binOp->left());
                traverseAST(binOp->left(), callBack);
                callBack(binOp->right());
                traverseAST(binOp->right(), callBack);
                break;
            }
            case ASTNodeType::Closure: {
                auto closure = static_cast<ASTDoubleStructure const*>(ast);
                callBack(closure->first());
                traverseAST(closure->first(), callBack);
                callBack(closure->second());
                traverseAST(closure->second(), callBack);
                break;
            }
            case ASTNodeType::Variable: {
                auto var = static_cast<ASTVariable const*>(ast);
                callBack(var->valueExpr());
                traverseAST(var->valueExpr(), callBack);
                break;
            }
            case ASTNodeType::Identifier:
            case ASTNodeType::Integer: {
                break;
            }
            default: {
                // assert(false);
                break;
            }
        }

    }

    bool Env::compileCodeChunk(Name module, ASTNode* ast) {
        if(ast->type() == ASTNodeType::CodeChunk) {
            ASTMultiExpr* exprs = (ASTMultiExpr*)ast;
            {
                auto iter = exprs->expressions().begin();
                if(iter != exprs->expressions().end()) {
                    ASTNode* expr = *iter;
                    if(expr->type() == ASTNodeType::Package) {
                        ASTPackage* packNode = (ASTPackage*)expr;
                        auto package = this->preparePackage(packNode);
                        auto symbolLayout = package->symbolLayout();
                        //
                        ++iter;
                        while(iter != exprs->expressions().end()) {
                            auto expr = *iter;
                            if(expr->type() == ASTNodeType::Function) {
                                ASTFunction* func = (ASTFunction*)expr;
                                auto rst = symbolLayout->regSymbol(func->name().stringLiteral(), SymbolType::Function, func, Value(), module);
                                if(!rst) {
                                    assert(false);
                                    return false;
                                }
                            } else if( expr->type() == ASTNodeType::Variable ) {
                                ASTVariable* var = (ASTVariable*)expr;
                                auto rst = symbolLayout->regSymbol(var->name().stringLiteral(), SymbolType::Variable, var, Value(), module);
                            } else {
                                assert(false && "only function & variable can be defined in package");
                                return false;
                        }
                            ++iter;
                        }
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            }
            return false;
        }
        else {
            return false;
        }
    }
        
    std::vector<ASTIdentifier*> Env::compileFunction(Name module, ASTNode* ast) {
        assert(ast->type() == ASTNodeType::Function);
        SymbolLayout* symLayout = new SymbolLayout();
        std::vector<ASTIdentifier*> compilerErrors;
        // the callback
        TraverseCallBack processor = [&](compiler::ASTNode const* node) {
            if(node->type() == compiler::ASTNodeType::Variable) {
                auto var = static_cast<compiler::ASTVariable const*>(node);
                auto token = var->name();
                symLayout->regSymbol(token.stringLiteral(), SymbolType::Variable, node, Value(), module);
            } else if( node->type() == compiler::ASTNodeType::BinaryOp) {
                auto binExpr = static_cast<compiler::ASTBinaryOpExpr const*>(node);
                auto leftExpr = binExpr->left();
                auto rightExpr = binExpr->right();
                if(leftExpr->type() == compiler::ASTNodeType::Identifier) {
                    auto rst = locateIdentifier({symLayout, _rootPackage}, static_cast<compiler::ASTIdentifier const*>(leftExpr));
                    if(!rst) {
                        compilerErrors.push_back((ASTIdentifier*)leftExpr);
                    }
                } 
                if(rightExpr->type() == compiler::ASTNodeType::Identifier) {
                    if(binExpr->op() != compiler::TokenType::Dot) {
                        auto id = static_cast<compiler::ASTLeaf const*>(rightExpr);
                        auto token = id->token();
                        if(token.type() == compiler::TokenType::Identifier) {
                            auto rst = locateIdentifier({symLayout, _rootPackage}, static_cast<compiler::ASTIdentifier const*>(rightExpr));
                            if(!rst) {
                                compilerErrors.push_back((ASTIdentifier*)rightExpr);
                            }
                        }
                    }
                }
            } else if( node->type() == compiler::ASTNodeType::Primary) {
                auto primary = static_cast<compiler::ASTPrimary const*>(node);
                if(primary->operand()->type() == compiler::ASTNodeType::Identifier) {
                    auto id = primary->operand();
                    auto rst = locateIdentifier({symLayout, _rootPackage}, static_cast<compiler::ASTIdentifier const*>(id));
                    if(!rst) {
                        compilerErrors.push_back((ASTIdentifier*)id);
                    }
                }
            }
        };
        // traverse the ast
        traverseAST(ast, processor);
        if(!compilerErrors.size()) {
            ASTFunction* func = static_cast<ASTFunction*>(ast);
        }
        return compilerErrors;
    }

    bool Env::locateIdentifier(IdentifierLocatorEnv env, ASTIdentifier const* id) {
        auto name = id->token().stringLiteral();
        auto symbolLoc = env.layout->querySymbolLoc(name);
        if(~symbolLoc != 0) { // local var
            id->setValue( IdentifierType::Local, symbolLoc);
            return true;
        } else { // current package var
            auto value = env.package->operator[](name);
            if(value) {
                id->setValue(value);
                return true;
            } else { // global var/package
                value = _rootPackage->operator[](name);
                if(value) {
                    id->setValue(value);
                    return true;
                } else {
                    // failed to find the identifier
                    return false;
                }
            }
        }
        return true;
    }

}