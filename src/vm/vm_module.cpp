#include "vm_module.h"
#include "vm_env.h"
#include "vm_object.h"


namespace compiler {

    void Module::addFunction(Node* node) {
        _functions.push_back(node);
    }
    void Module::addClass(Value cls) {
        _classes.push_back(cls);
    }
    void Module::addInitliaze(uint32_t loc, Node* node) {
        if(node) {
            _initliazeList.push_back(std::make_pair(loc, node));
        }
    }

    void Module::initialize(Env* env) {
        postprocess(env);
        IdLocateEnv locateEnv = {
            nullptr, 
            nullptr,
            _package.asObject()->symbolLayout(),
            env->root().asObject()->symbolLayout()
        };
        for (auto& pair : _initliazeList) {
            auto loc = pair.first;
            auto node = pair.second;
            postprocessFunction(env, node, locateEnv);
            Value valRef = _package[loc];
            int retCount = env->callFunction(Value(node));
            assert(retCount == 1);
            valRef = env->stackValues().popValue();
            // valRef = env->stackValues().topValue();
        }
    }

    void Module::setHostPackage(Value package) {
        _package = package;
    }

    std::vector<Token> Module::postprocess(Env* env) {
        IdLocateEnv locateEnv = {
            nullptr, // function local
            nullptr, // class
            _package.asObject()->symbolLayout(), // local package
            env->root().asObject()->symbolLayout() // global
        };
        auto rst = std::vector<Token>();
        for(auto func : _functions) {
            rst = postprocessFunction(env, func, locateEnv);
        }
        for(auto cls : _classes) {
            locateEnv.classLayout = cls.asObject()->symbolLayout();
            cls.enumerateFunctions([&](ast::Function* val) {
                // if(val->asFunc()) {
                    auto err = postprocessFunction(env, val, locateEnv);
                    if(err.size()) {
                        rst.insert(rst.end(), err.begin(), err.end());
                    }
                // }
            });
        }
        return rst;
    }

    std::vector<Token> Module::postprocessFunction(Env* env, Node* ast, IdLocateEnv locateEnv) {
        assert(ast->structType() == SType::Function);
        SymbolLayout* symLayout = env->newSymbolLayout(SymbolLayoutType::Function);
        locateEnv.functionLayout = symLayout;
        std::vector<Token> compilerErrors;
        Function* func = (Function*)ast;
        func->setSymbolLayout(symLayout);
        for(auto param: func->params()) {
            auto rst = symLayout->regSymbol(param.stringLiteral(), SymbolType::Variable, Value(), func->module());
            if(!rst.item) {
                compilerErrors.push_back(param);
            }
        }
        // the callback
        // 我们关心特定的节点，这些节点代表变量定义与引用
        /**
         * @brief 
         * 1. Variable 变量定义
         * 2. 二元操作符里的Id
         *   * id在左边一定是变量引用
         *   * id在右边如果二元操作符是dot，不是变量否则是变量引用
         * 3. Primary的operand如果是id，则是变量引用
         */
        TraverseCallBack processor = [&](compiler::Node const* node) {
            Identifier* id = node->asId();
            if(!id) {
                return;
            }
            // IdLocateEnv locateEnv = {symLayout, func->hostPackage().asObject()->symbolLayout()};
            auto parent = node->parent();
            switch(parent->structType()) {
                case SType::Pair: { // define variable        
                    if(parent->valueType() == VType::Variable) {
                        auto regRst = symLayout->regSymbol(id->token().stringLiteral(), SymbolType::Variable, Value(), func->module());
                        id->setValue(IdentifierType::FunctionLocal, regRst.loc);
                    } else if(parent->valueType() == VType::DotAccess) {
                        if(!locateIdentifier(locateEnv, id)) {
                            compilerErrors.push_back(id->token());
                        }
                    } else if(parent->valueType() == VType::FunctionCall) {
                        if(!locateIdentifier(locateEnv, id)) {
                            compilerErrors.push_back(id->token());
                        }
                    }
                    return;
                }
                case SType::BinaryOp: {
                    auto binOp = parent->asBinaryExpr();
                    if(binOp->op() == TokenType::Dot) {
                        if(node == binOp->right()) {
                            return;
                        }
                    }
                    if(!locateIdentifier(locateEnv, id)) {
                        compilerErrors.push_back(id->token());
                    }
                    return;
                }
                default: {
                    if(!locateIdentifier(locateEnv, id)) {
                        compilerErrors.push_back(id->token());
                    }
                }
            }
        };
        // traverse the ast
        this->traverseAST(ast, processor);
        if(!compilerErrors.size()) {
            Function* func = static_cast<Function*>(ast);
        }
        func->_valid = !compilerErrors.size();
        func->_compiled = true;
        return compilerErrors;
    }

    bool Module::locateIdentifier(IdLocateEnv env, Identifier const* id) {
        auto name = id->token().stringLiteral();
        auto symbolLoc = env.functionLayout->querySymbolLoc(name);
        if(~symbolLoc != 0) { // local symbol
            id->setValue( IdentifierType::FunctionLocal, symbolLoc);
            return true;
        } else {
            if(env.classLayout) {
                symbolLoc = env.classLayout->querySymbolLoc(name);
            }
            if(~symbolLoc != 0) { // member symbol
                id->setValue(IdentifierType::ClassMember, symbolLoc);
                return true;
            } else { // current package var
                symbolLoc = env.packageLayout->querySymbolLoc(name);
                if(~symbolLoc != 0) {
                    id->setValue( IdentifierType::CurrentPackage, symbolLoc);
                    return true;
                }
                else { // global
                    symbolLoc = env.globalLayout->querySymbolLoc(name);
                    if(~symbolLoc != 0) {
                        id->setValue( IdentifierType::Global, symbolLoc);
                        return true;
                    }
                    return false;
                }
            }
        }
        return false;
    }

    void Module::traverseAST(Node const* ast, TraverseCallBack& callBack) {
        switch(ast->structType()) {
            case SType::Function: {
                auto func = static_cast<Function const*>(ast);
                if(func->body()) {
                    callBack(func->body());
                    traverseAST(func->body(), callBack);
                }
                break;
            }
            case SType::MultiExpr: {
                auto block = static_cast<MultiExpr const*>(ast);
                for(auto& expr : block->expressions()) {
                    callBack(expr);
                    traverseAST(expr, callBack);
                }
                break;
            }
            case SType::If: {
                auto ifNode = static_cast<IfStmt const*>(ast);
                callBack(ifNode->condition());
                traverseAST(ifNode->condition(), callBack);
                callBack(ifNode->thenBranch());
                traverseAST(ifNode->thenBranch(), callBack);
                if(ifNode->elseBranch()) {
                    callBack(ifNode->elseBranch());
                    traverseAST(ifNode->elseBranch(), callBack);
                }
                break;
            }
            case SType::BinaryOp: {
                auto binOp = static_cast<BinaryOpExpr const*>(ast);
                callBack(binOp->left());
                traverseAST(binOp->left(), callBack);
                callBack(binOp->right());
                traverseAST(binOp->right(), callBack);
                break;
            }
            case SType::Pair: {
                auto pair = static_cast<PairExpr const*>(ast);
                callBack(pair->first());
                traverseAST(pair->first(), callBack);
                if(pair->second()) {
                    callBack(pair->second());
                    traverseAST(pair->second(), callBack);
                }
                break;
            }
            case SType::Leaf: { // current is leaf, no need to traverse
                break;
            }
            case SType::Return: {
                auto ret = static_cast<ReturnStmt const*>(ast);
                if(ret->expr()) {
                    callBack(ret->expr());
                    traverseAST(ret->expr(), callBack);
                }
                break;
            }
            default: {
                assert(false);
                break;
            }
        }
    }

}