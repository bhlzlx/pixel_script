#include "vm_env.h"

namespace compiler {

    Value Env::eval(Node const* ast) {
        // MultiExpr,If,While,BinaryOp,Variable,Leaf,Primary,NegtiveOp,Pair,Function,StringList,
        Value rst;
        switch(ast->structType()) {
            case SType::BinaryOp: {
                BinaryOpExpr* binExpr = ast->asBinaryExpr();
                Value left = eval(binExpr->left());
                Value right = eval(binExpr->right());
                return evalBinaryOp(binExpr->op(), left, right);
            }
            case SType::If: {
                IfStmt* ifExpr = ast->asIf();
                Value cond = eval(ifExpr->condition());
                if(cond) {
                    return eval(ifExpr->elseBranch()); 
                } else {
                    return eval(ifExpr->thenBranch());
                }
                break;
            }
            case SType::MultiExpr: {
                MultiExpr* multiExpr = ast->asMultiExpr();
                Value rst;
                for(auto expr: multiExpr->expressions()) {
                    rst = eval(expr);
                }
                return rst;
            }
            case SType::NegtiveOp: {
                NegativeExpr* negtiveOp = ast->asNegativeExpr();
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
            case SType::Pair:{
                auto valType = ast->valueType();
                switch(valType) {
                    case VType::Variable: {
                        Variable* var = ast->asVar();
                        Identifier* id = var->id();
                        auto fenv = funcEnv();
                        auto loc = id->valueLoc();
                        Value* varVtVal = fenv->vt[loc];
                        Value varExprEvalVal;
                        varExprEvalVal = eval(var->valueExpr());
                        if(varExprEvalVal.type() == ValueType::ValueRef) {
                            *varVtVal = *varExprEvalVal.ref();
                        } else {
                            *varVtVal = varExprEvalVal;
                        }
                        return *varVtVal;
                    }
                    case VType::FunctionCall: {
                        FunctionCall* caller = ast->asFunctionCall();
                        Value val = eval(caller->methodExpr());
                        assert(val.type() == ValueType::ValueRef);
                        val = *val.ref();
                        if(val.type() != ValueType::FunctionNode) { // it must be a function
                            return rst;
                        } else {
                            std::vector<Value> args;
                            if(caller->args()) {
                                for(auto expr: caller->args()->expressions()) {
                                    args.push_back(eval(expr));
                                }
                            }
                            Node const* node = val.node();
                            if(node->structType() == SType::Function) {
                                Function* func = (Function*)node;
                                for(auto& arg : args) {
                                    if(arg.type() == ValueType::ValueRef) {
                                        arg = *arg.ref(); 
                                    }
                                }
                                return callFunction(val,args);
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
                        Value* valPtr = (*objPtr)[fieldName]; // we should return the value's ref
                        if(valPtr) {
                            return Value(valPtr); // create ref
                        } else {
                            return Value();
                        }
                    }
                    default: {
                        break;
                    }
                }
            }
            case SType::Leaf: {
                auto leaf = ast->asLeaf();
                Token token = leaf->token();
                if(leaf->valueType() == VType::Id) {
                    rst = evalIdentifier(leaf->asId());
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
                        default: {
                            assert(false);
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
        return rst;
    }

    Value Env::evalBinaryOp(Token op, Value a, Value b) {
        auto fenv = funcEnv();
        Value* ap = a.ref();
        Value* bp = b.ref();
        if(ap->type() == ValueType::Int64) {
            IntegerValue const* ival = (IntegerValue const*)ap;
            return ival->Op(op, *bp);
        } else if(ap->type() == ValueType::Float64) {
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
        switch(idType) {
            case IdentifierType::Global: {
                return Value(_package[id->valueLoc()]);
            }
            case IdentifierType::FunctionLocal: {
                auto loc = id->valueLoc();
                auto fenv = funcEnv();
                return Value(fenv->vt[loc]);
            }
            case IdentifierType::CurrentPackage: {
                auto fenv = funcEnv();
                Value hostPack = fenv->func->hostPackage();
                auto loc = id->valueLoc();
                return Value(hostPack[loc]);
            }
            case IdentifierType::Null: {
            }
            default: {
                assert(false);
                break;
            }
        }
        return Value();
    }

}