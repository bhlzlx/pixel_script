#include "vm_env.h"

namespace compiler {

    Name Env::getName(char const* str) {
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
                callBack(var->valueExpr());
                traverseAST(var->valueExpr(), callBack);
                break;
            }
            case SType::Leaf: {
                callBack(ast);
                break;
            }
            default: {
                assert(false);
                break;
            }
        }

    }

    bool Env::compileCodeChunk(char const* mod, Node* ast) {
        auto module = getName(mod);
        if(ast->structType() == SType::MultiExpr) {
            ASTMultiExpr* exprs = (ASTMultiExpr*)ast;
            auto iter = exprs->expressions().begin();
            if(iter != exprs->expressions().end()) {
                Node* expr = *iter;
                if(expr->valueType() == VType::Package) {
                    auto package = preparePackage(expr);
                    auto packObj = package.asObject();
                    //
                    ++iter;
                    while(iter != exprs->expressions().end()) {
                        auto expr = *iter;
                        if(expr->structType() == SType::Function) {
                            ASTFunction* func = (ASTFunction*)expr;
                            auto rst = packObj->addSymbol(func->name().stringLiteral(), SymbolType::Function, Value(func), module);
                            func->setPackageSymbolLayout(packObj->symbolLayout());
                            func->setModule(getName(mod));
                            if(!rst.first) {
                                assert(false);
                                return false;
                            }
                        } else if( expr->structType() == SType::Variable ) {
                            ASTVariable* var = (ASTVariable*)expr;
                            auto rst = packObj->addSymbol(var->name().stringLiteral(), SymbolType::Variable, Value(var), module);
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
        
    std::vector<Token> Env::compileFunction(ASTFunction* ast) {
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
        TraverseCallBack processor = [&](compiler::Node const* node) {
            if(node->structType() == compiler::SType::Variable) {
                auto var = static_cast<compiler::ASTVariable const*>(node);
                auto token = var->name();
                auto regRst = symLayout->regSymbol(token.stringLiteral(), SymbolType::Variable, func->module());
                var->id()->setValue(IdentifierType::Local, regRst.second);
            } else if( node->structType() == compiler::SType::BinaryOp) {
                auto binExpr = static_cast<compiler::ASTBinaryOpExpr const*>(node);
                auto leftExpr = binExpr->left();
                auto rightExpr = binExpr->right();
                if(leftExpr->valueType() == compiler::VType::Id) {
                    auto rst = locateIdentifier({symLayout, }, static_cast<compiler::ASTIdentifier const*>(leftExpr));
                    if(!rst) {
                        compilerErrors.push_back(((ASTIdentifier*)leftExpr)->token());
                    }
                } 
                if(rightExpr->valueType() == compiler::VType::Id) {
                    if(binExpr->op() != compiler::TokenType::Dot) {
                        auto id = static_cast<compiler::ASTLeaf const*>(rightExpr);
                        auto token = id->token();
                        if(token.type() == compiler::TokenType::Identifier) {
                            auto rst = locateIdentifier({symLayout, func->packageSymbolLayout()}, static_cast<compiler::ASTIdentifier const*>(rightExpr));
                            if(!rst) {
                                compilerErrors.push_back(((ASTIdentifier*)rightExpr)->token());
                            }
                        }
                    }
                }
            } else if( node->structType() == compiler::SType::Primary) {
                auto primary = static_cast<compiler::ASTPrimary const*>(node);
                if(primary->operand()->valueType() == compiler::VType::Id) {
                    auto id = primary->operand();
                    auto rst = locateIdentifier({symLayout, func->packageSymbolLayout()}, static_cast<compiler::ASTIdentifier const*>(id));
                    if(!rst) {
                        compilerErrors.push_back(((ASTIdentifier*)id)->token());
                    }
                }
            } else if(node->valueType() == compiler::VType::Id) {
                if(node->parent()->structType() == compiler::SType::MultiExpr) {
                    auto rst = locateIdentifier({symLayout, func->packageSymbolLayout()}, static_cast<compiler::ASTIdentifier const*>(node));
                    if(!rst) {
                        compilerErrors.push_back(((ASTIdentifier*)node)->token());
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

    bool Env::locateIdentifier(IdentifierLocatorEnv env, ASTIdentifier const* id) {
        auto name = id->token().stringLiteral();
        auto symbolLoc = env.functionLayout->querySymbolLoc(name);
        if(~symbolLoc != 0) { // local var
            id->setValue( IdentifierType::Local, symbolLoc);
            return true;
        } else { // current package var
            symbolLoc = env.packageLayout->querySymbolLoc(name);
            if(~symbolLoc != 0) {
                id->setValue( IdentifierType::Package, symbolLoc);
                return true;
            }
            else {
                symbolLoc = _package.asObject()->symbolLayout()->querySymbolLoc(name);
                if(~symbolLoc != 0) {
                    id->setValue( IdentifierType::Package, symbolLoc);
                    return true;
                }
                return false;
            }
        }
        return false;
    }

    Value Env::eval(Node const* ast) {
        // MultiExpr,If,While,BinaryOp,Variable,Leaf,Primary,NegtiveOp,Pair,Function,StringList,
        Value nil;
        switch(ast->structType()) {
            case SType::BinaryOp: {
                ASTBinaryOpExpr* binExpr = ast->asBinaryExpr();
                Value left = eval(binExpr->left());
                Value right = eval(binExpr->right());
                return evalBinaryOp(binExpr->op(), &left, &right);
            }
            case SType::If: {
                ASTIfStatement* ifExpr = ast->asIf();
                Value cond = eval(ifExpr->condition());
                if(cond) {
                    return eval(ifExpr->elseBranch()); 
                } else {
                    return eval(ifExpr->thenBranch());
                }
                break;
            }
            case SType::While: {
                Value rst;
                ASTWhileStatement* whileExpr = ast->asWhile();
                Value cond = eval(whileExpr->condition());
                while(cond) {
                    rst = eval(whileExpr->body());
                    cond = eval(whileExpr->condition());
                }
                return rst;
            }
            case SType::MultiExpr: {
                ASTMultiExpr* multiExpr = ast->asMultiExpr();
                Value rst;
                for(auto expr: multiExpr->expressions()) {
                    rst = eval(expr);
                }
                return rst;
            }
            case SType::NegtiveOp: {
                ASTNegativeExpression* negtiveOp = ast->asNegativeExpr();
                Value val = eval(negtiveOp->value());
                Value rst;
                if(val.type() == ValueType::Int64) {
                    rst.setInt64(-val.intValue());
                    return rst;
                } else if(val.type() == ValueType::Float64) {
                    rst.setFloat64(-val.floatValue());
                    return rst;
                } else {
                    ExecuteException except(negtiveOp->op());
                    throw except;
                }
                return Value();
            }
            case SType::Variable:{
                ASTVariable* var = ast->asVar();
                ASTIdentifier* id = var->id();
                Value frame = currentFrame();
                auto loc = id->valueLoc();
                Value* val = frame[loc];
                *val = eval(var->valueExpr());
                return *val;
            }
            case SType::Primary: {
                ASTPrimary* primary = (ASTPrimary*)ast;
                Value val = eval(primary->operand());
                if(val.type() != ValueType::ASTNode) {
                    return nil;
                } else {
                    std::vector<Value> args;
                    for(auto expr: primary->args()->expressions()) {
                        args.push_back(eval(expr));
                    }
                    Node const* node = val.node();
                    if(node->structType() == SType::Function) {
                        ASTFunction* func = (ASTFunction*)node;
                        return callFunction(val,args);
                    } else {
                        return Value();
                    }
                }
                break;
            }
            case SType::Leaf: {
                Value rst;
                auto leaf = ast->asLeaf();
                Token token = leaf->token();
                if(leaf->valueType() == VType::Id) {
                    rst = Value(leaf);
                    // auto id = leaf->asId();
                    // auto loc = id->valueLoc();
                    // rst = *currentFrame()[loc];
                } else {
                    switch(token.type()) {
                        case TokenType::Float: {
                            rst.setFloat64(token.floatLiteral());
                            break;
                        }
                        case TokenType::Integer: {
                            rst.setInt64(token.integerLiteral());
                            break;
                        }
                        case TokenType::String: {
                            rst.setString(token.stringLiteral());
                            break;
                        }
                    }
                }
                return rst;
            }
            default: {
                assert(false);
                break;
            }
        }
        return Value();
    }

    Value Env::evalBinaryOp(Token op, Value const* a, Value const* b) {
        auto frame = currentFrame();
        if(a->type() == ValueType::ASTNode) {
            assert(a->node()->structType() == SType::Leaf);
            assert(a->node()->valueType() == VType::Id);
            auto loc = a->node()->asId()->valueLoc();
            a = frame[loc];
        }
        if(b->type() == ValueType::ASTNode) {
            assert(b->node()->structType() == SType::Leaf);
            assert(b->node()->valueType() == VType::Id);
            auto loc = b->node()->asId()->valueLoc();
            b = frame[loc];
        }
        if(a->type() == ValueType::Int64) {
            IntegerValue const* ival = (IntegerValue const*)a;
            return ival->Op(op, *b);
        } else if(a->type() == ValueType::Float64) {
            FloatValue const* fval = (FloatValue const*)a;
            return fval->Op(op, *b);
        } else {
            assert(false && "unsupported type");
        }
        return Value();
    }

    Value Env::callFunction(Value const& func, std::vector<Value> const& args) {
        // assert(func.type() == ValueType::Function);
        ASTFunction* fn = (ASTFunction*)func.node();
        if(!fn->compiled()){
            auto errs = this->compileFunction(fn);
            if(errs.size()) {
                return Value();
            }
        }
        if(!fn->valid()) {
            return Value();
        }
        auto params = fn->params();
        _stackFrame.emplace_back(fn->symbolLayout());
        auto frame = currentFrame();
        for(size_t i = 0; (i < params.size())&&(i<args.size()); i++) {
            Value* argRef = frame[i];
            *argRef = args[i];
        }
        Value rst = eval(fn->body());
        if(rst.type() == ValueType::ASTNode) {
            auto id = rst.node()->asId();
            rst = *currentFrame()[id->valueLoc()];
        }
        // clean up the stack frame
        _stackFrame.pop_back();
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
            auto name = getName(id.c_str());
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
}