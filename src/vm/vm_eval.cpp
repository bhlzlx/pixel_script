#include "vm_env.h"

namespace compiler {

    Value Env::eval(Node const* ast) {
        // MultiExpr,If,While,BinaryOp,Variable,Leaf,Primary,NegtiveOp,Pair,Function,StringList,
        Value nil;
        switch(ast->structType()) {
            case SType::BinaryOp: {
                BinaryOpExpr* binExpr = ast->asBinaryExpr();
                Value left = eval(binExpr->left());
                Value right = eval(binExpr->right());
                return evalBinaryOp(binExpr->op(), left, right);
            }
            case SType::If: {
                IfStmt* ifExpr = ast->asIf();
                Value ifRst;
                Value cond = eval(ifExpr->condition());
                cond.deref();
                if(cond) {
                    ifRst = eval(ifExpr->elseBranch()); 
                } else {
                    ifRst = eval(ifExpr->thenBranch());
                }
                ifRst.deref();
                return ifRst;
                break;
            }
            case SType::MultiExpr: {
                Value multiExprRst;
                MultiExpr* multiExpr = ast->asMultiExpr();
                for(auto expr: multiExpr->expressions()) {
                    multiExprRst = eval(expr);
                    multiExprRst.deref();
                }
                return multiExprRst;
            }
            case SType::NegtiveOp: {
                NegativeExpr* negtiveOp = ast->asNegativeExpr();
                Value val = eval(negtiveOp->value());
                Value rst;
                if(val.type() == PrimeVType::Int64) {
                    rst.setInt64(-val.intValue());
                    return rst;
                } else if(val.type() == PrimeVType::Float64) {
                    rst.setFloat64(-val.floatValue());
                    return rst;
                } else {
                    ExecuteException except(negtiveOp->op());
                    throw except;
                }
                return nil;
            }
            case SType::Pair:{
                auto valType = ast->valueType();
                switch(valType) {
                    case VType::Variable: {
                        Variable* var = ast->asVar();
                        Identifier* id = var->id();
                        auto fenv = funcEnv();
                        auto loc = id->valueLoc();
                        Value varVtVal = fenv->vt[loc];
                        Value varExprEvalVal = eval(var->valueExpr());
                        varVtVal = *varExprEvalVal.ref();
                        return varVtVal;
                    }
                    case VType::FunctionCall: {
                        FunctionCall* caller = ast->asFunctionCall();
                        Value val = eval(caller->methodExpr());
                        assert(val.type() == PrimeVType::FunctionNode);
                        val = *val.ref();
                        if(val.type() != PrimeVType::FunctionNode) { // it must be a function
                            return nil;
                        } else {
                            Node const* node = val.node();
                            if(node->structType() == SType::Function) {
                                // 传递self对象，机智的我想到了这个办法
                                Value self;
                                Node* methodExpr = caller->methodExpr();
                                if(methodExpr->valueType() == VType::DotAccess) {
                                    DotAccess* dotAccess = methodExpr->asDotAccess();
                                    self = *(eval(dotAccess->obj()).ref());
                                }
                                switch(node->valueType()) {
                                    case VType::None: {
                                        std::vector<Value> args;
                                        if(self && self.stype() == SymbolLayoutType::Class) {
                                            args.push_back(self); // 传递this指针！
                                        }
                                        if(caller->args()) {
                                            for(auto expr: caller->args()->expressions()) {
                                                args.push_back(eval(expr));
                                            }
                                        }
                                        Function* func = (Function*)node;
                                        for(auto& arg : args) {
                                            if(arg.type() == PrimeVType::ValueRef) {
                                                arg = std::move(*arg.ref());
                                            }
                                        }
                                        return callFunction(val,args);
                                    }
                                    case VType::NewOperator: {
                                        NewOperator* newOp = node->asNew();
                                        // create a new object value with the layout
                                        Value val = Value(newOp->symbolLayout());
                                        return val;
                                    }
                                    default: {
                                        assert(false);
                                    }
                                }
                            } else {
                                return Value();
                            }
                        }
                    }
                    case VType::WhileStmt: {
                        Value rst;
                        WhileStmt* whileExpr = ast->asWhile();
                        Value cond = eval(whileExpr->condition());
                        while(cond) {
                            rst = eval(whileExpr->body());
                            cond = eval(whileExpr->condition());
                        }
                        return rst;
                    }
                    case VType::DotAccess: {
                        DotAccess* dotAccess = ast->asDotAccess();
                        Value obj = eval(dotAccess->obj()); // object must be a ref
                        Value* objPtr = obj.ref();
                        auto fieldLeaf = dotAccess->field()->asLeaf();
                        assert(fieldLeaf); assert(fieldLeaf->valueType() == VType::String);
                        Name fieldName = fieldLeaf->token().stringLiteral();
                        Value valPtr = (*objPtr)[fieldName]; // we should return the value's ref
                        return valPtr;
                    }
                    default: {
                        break;
                    }
                }
            }
            case SType::Leaf: {
                Value leafRst;
                auto leaf = ast->asLeaf();
                Token token = leaf->token();
                if(leaf->valueType() == VType::Id) {
                    leafRst = evalIdentifier(leaf->asId()); // a ref from var table
                } else {
                    switch(token.type()) {
                        case TokenType::Float: {
                            leafRst.setFloat64(token.floatLiteral());
                            break;
                        }
                        case TokenType::Integer: {
                            leafRst.setInt64(token.integerLiteral());
                            break;
                        }
                        case TokenType::String: {
                            leafRst.setString(token.stringLiteral());
                            break;
                        }
                        default: {
                            assert(false);
                            break;
                        }
                    }
                }
                return leafRst;
            }
            default: {
                assert(false);
                break;
            }
        }
        return nil;
    }

    Value Env::evalBinaryOp(Token op, Value a, Value b) {
        auto fenv = funcEnv();
        if(op.type() == TokenType::Assign) {
            a = *b.ref();
            return a;
        }
        Value* ap = a.ref();
        Value* bp = b.ref();
        if(ap->type() == PrimeVType::Int64) {
            IntegerValue const* ival = (IntegerValue const*)ap;
            return ival->Op(op, *bp);
        } else if(ap->type() == PrimeVType::Float64) {
            FloatValue const* fval = (FloatValue const*)ap;
            return fval->Op(op, *bp);
        }
        else {
            assert(false && "unsupported type");
        }
        return Value();
    }

    Value Env::evalIdentifier(Identifier const* id) {
        auto idType = id->type(); 
        Value rst;
        switch(idType) {
            case IdentifierType::Global: {
                rst = _package[id->valueLoc()];
                break;
            }
            case IdentifierType::FunctionLocal: {
                auto loc = id->valueLoc();
                auto fenv = funcEnv();
                rst = fenv->vt[loc];
                break;
            }
            case IdentifierType::CurrentPackage: {
                auto fenv = funcEnv();
                Value hostPack = fenv->func->hostPackage();
                auto loc = id->valueLoc();
                rst = hostPack[loc];
                break;
            }
            case IdentifierType::ClassMember: {
                auto fenv = funcEnv();
                auto self = *fenv->vt[0].ref(); //self is a ref
                // self = std::move(*self.ref());
                assert(self.stype() == SymbolLayoutType::Class);
                return self[id->token().stringLiteral()];
            }
            case IdentifierType::Null: {
            }
            default: {
                assert(false);
                break;
            }
        }
        return rst;
    }

}