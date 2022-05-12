#include "vm_env.h"

namespace compiler {

    Name Env::createName(char const* str) {
        return _namePool.getName(str);
    }

    Value Env::preparePackage( Node* ast ) {
        auto package = ast->asStringList();
        auto pack = rootPackage();
        for( auto name : package->names() ) {
            Object* parent = pack.asObject();
            auto layout = newSymbolLayout();
            Value subpack(layout);
            parent->addSymbol(name.stringLiteral(), SymbolType::Package, subpack, Name());
            pack = subpack;
        }
        return pack;
    }

    void Env::traverseAST(Node const* ast, TraverseCallBack& callBack) {
        switch(ast->structType()) {
            case SType::Function: {
                auto func = static_cast<ASTFunction const*>(ast);
                callBack(func->body());
                traverseAST(func->body(), callBack);
                break;
            }
            case SType::MultiExpr: {
                auto block = static_cast<ASTMultiExpr const*>(ast);
                for(auto& expr : block->expressions()) {
                    callBack(expr);
                    traverseAST(expr, callBack);
                }
                break;
            }
            case SType::While: {
                auto whileNode = static_cast<ASTWhileStatement const*>(ast);
                callBack(whileNode->condition());
                traverseAST(whileNode->condition(), callBack);
                callBack(whileNode->body());
                traverseAST(whileNode->body(), callBack);
                break;
            }
            case SType::If: {
                auto ifNode = static_cast<ASTIfStatement const*>(ast);
                callBack(ifNode->condition());
                traverseAST(ifNode->condition(), callBack);
                callBack(ifNode->thenBranch());
                traverseAST(ifNode->thenBranch(), callBack);
                callBack(ifNode->elseBranch());
                traverseAST(ifNode->elseBranch(), callBack);
                break;
            }
            case SType::BinaryOp: {
                auto binOp = static_cast<ASTBinaryOpExpr const*>(ast);
                callBack(binOp->left());
                traverseAST(binOp->left(), callBack);
                callBack(binOp->right());
                traverseAST(binOp->right(), callBack);
                break;
            }
            case SType::Variable: {
                auto var = static_cast<ASTVariable const*>(ast);
                callBack(var->id());
                // id is a leaf node, so no need to traverse it
                callBack(var->valueExpr());
                traverseAST(var->valueExpr(), callBack);
                break;
            }
            case SType::Leaf: { // current is leaf, no need to traverse
                break;
            }
            case SType::Primary: {
                auto operand =ast->asPrimary()->operand(); 
                auto args = ast->asPrimary()->args();
                callBack(operand);
                traverseAST(operand, callBack);
                if(args) {
                    traverseAST(args, callBack);
                }
                break;
            }
            default: {
                assert(false);
                break;
            }
        }
    }

    bool Env::compileCodeChunk(char const* mod, Node* ast) {
        if(ast->structType() == SType::MultiExpr) {
            ASTMultiExpr* exprs = (ASTMultiExpr*)ast;
            auto iter = exprs->expressions().begin();
            if(iter != exprs->expressions().end()) {
                Node* expr = *iter;
                if(expr->valueType() == VType::Package) {
                    auto package = preparePackage(expr);
                    auto packObj = package.asObject();
                    auto moduleName = createName(mod);
                    auto module = getModule(moduleName);
                    module->setHostPackage(package);
                    //
                    ++iter;
                    while(iter != exprs->expressions().end()) {
                        auto expr = *iter;
                        if(expr->structType() == SType::Function) {
                            ASTFunction* func = (ASTFunction*)expr;
                            auto rst = packObj->addSymbol(func->name().stringLiteral(), SymbolType::Function, Value(func), moduleName);
                            func->setHostPackage(package);
                            func->setModule(moduleName);
                            if(!rst.first) {
                                assert(false);
                                return false;
                            }
                        } else if( expr->structType() == SType::Variable ) {
                            ASTVariable* var = (ASTVariable*)expr;
                            // 注意这个地方，value不是ast节点，而是实际给了一个空值，占位。
                            auto rst = packObj->addSymbol(var->name().stringLiteral(), SymbolType::Variable, Value(), moduleName);
                            if(var->valueExpr()) { // 创建一个特别的function，给var初始化，方便代码重用，处理
                                ASTMultiExpr* funcBody = new ASTMultiExpr(VType::Block);
                                funcBody->addExpr(var->valueExpr());
                                ASTFunction* func = new ASTFunction(VType::Closure);
                                func->setBody(funcBody);
                                func->setHostPackage(package);
                                func->setModule(moduleName);
                                module->addInitliaze(rst.second, func); // 添加变量到初始化列表
                            }
                        } else {
                            assert(false && "only function & variable can be defined in package");
                            return false;
                        }
                        ++iter;
                    }
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        else {
            return false;
        }
    }
        
    std::vector<Token> Env::postprocessFunction(ASTFunction* ast) {
        assert(ast->structType() == SType::Function);
        SymbolLayout* symLayout = newSymbolLayout();
        std::vector<Token> compilerErrors;
        ASTFunction* func = (ASTFunction*)ast;
        func->setSymbolLayout(symLayout);
        for(auto param: func->params()) {
            auto rst = symLayout->regSymbol(param.stringLiteral(), SymbolType::Variable, func->module());
            if(!rst.first) {
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
            ASTIdentifier* id = node->asId();
            if(!id) {
                return;
            }
            IdLocateEnv locateEnv = {symLayout, func->hostPackage().asObject()->symbolLayout()};
            auto parent = node->parent();
            switch(parent->structType()) {
                case SType::Variable: { // define variable        
                    auto regRst = symLayout->regSymbol(id->token().stringLiteral(), SymbolType::Variable, func->module());
                    id->setValue(IdentifierType::FunctionLocal, regRst.second);
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
        traverseAST(ast, processor);
        if(!compilerErrors.size()) {
            ASTFunction* func = static_cast<ASTFunction*>(ast);
        }
        func->_valid = !compilerErrors.size();
        func->_compiled = true;
        return compilerErrors;
    }

    bool Env::locateIdentifier(IdLocateEnv env, ASTIdentifier const* id) {
        auto name = id->token().stringLiteral();
        auto symbolLoc = env.functionLayout->querySymbolLoc(name);
        if(~symbolLoc != 0) { // local var
            id->setValue( IdentifierType::FunctionLocal, symbolLoc);
            return true;
        } else { // current package var
            symbolLoc = env.packageLayout->querySymbolLoc(name);
            if(~symbolLoc != 0) {
                id->setValue( IdentifierType::CurrentPackage, symbolLoc);
                return true;
            }
            else {
                symbolLoc = _package.asObject()->symbolLayout()->querySymbolLoc(name);
                if(~symbolLoc != 0) {
                    id->setValue( IdentifierType::Global, symbolLoc);
                    return true;
                }
                return false;
            }
        }
        return false;
    }

    Value Env::callFunction(Value const& func, std::vector<Value> const& args) {
        // assert(func.type() == ValueType::Function);
        ASTFunction* fn = (ASTFunction*)func.node();
        if(!fn->compiled()){
            auto errs = this->postprocessFunction(fn);
            if(errs.size()) {
                return Value();
            }
        }
        if(!fn->valid()) {
            return Value();
        }
        auto params = fn->params();
        auto vt = Value(fn->symbolLayout());  // value table
        FuncEnv fenv = { vt, fn };
        _funcEnvs.push_back(fenv); // 实际上创建了一个新的局部变量表
        Value rst;
        {
            auto fenv = funcEnv();
            for(size_t i = 0; (i < params.size())&&(i<args.size()); i++) {
                Value* argRef = fenv->vt[i];
                *argRef = args[i];
            }
            rst = eval(fn->body());
            if(rst.type() == ValueType::ValueRef) {
                rst = *rst.ref();
            }
        }
        // clean up the stack frame
        _funcEnvs.pop_back();
        return rst;
    }

    Value Env::callFunction(std::string func) {
        std::string id;
        size_t last =0;
        size_t pos;
        Value val = _package;
        do {
            pos = func.find('.', last);
            if(pos == std::string::npos) {
                id = std::string(func.begin()+last, func.end());
            } else {
                id = std::string(func.begin()+last, func.begin()+pos);
            }
            auto name = createName(id.c_str());
            auto attr = val[name];
            if(attr) {
                val = *attr;
            } else {
                return Value();
            }
            if(pos == std::string::npos) {
                break;
            }
            last = pos + 1;
        } while(true);
        auto astFunc = val.asFunc();
        if(astFunc) {
            return callFunction(val, std::vector<Value>());
        }
        return Value();
    }

    void Env::initializeModule(char const* module) {
        auto modName = createName(module);
        auto mod = getModule(modName);
        mod->initialize(this);
    }

    Module* Env::getModule(Name const& name) {
        auto it = _modules.find(name);
        if (it != _modules.end()) {
            return it->second;
        } else {
            Module* mod = new Module();
            auto rst = _modules.insert(std::make_pair(name, mod));
            return rst.first->second;
        }
    }
}