#include "vm_env.h"

namespace compiler {

    Value Env::eval(Node const* ast) {
        // MultiExpr,If,While,BinaryOp,Variable,Leaf,Primary,NegtiveOp,Pair,Function,StringList,
        Value rst;
        switch(ast->structType()) {
            case SType::BinaryOp: {
                ASTBinaryOpExpr* binExpr = ast->asBinaryExpr();
                Value left = eval(binExpr->left());
                Value right = eval(binExpr->right());
                return evalBinaryOp(binExpr->op(), left, right);
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
                auto fenv = funcEnv();
                auto loc = id->valueLoc();
                Value* varVtVal = fenv->vt[loc];
                Value varExprEvalVal = eval(var->valueExpr());
                if(varExprEvalVal.type() == ValueType::ValueRef) {
                    *varVtVal = *varExprEvalVal.ref();
                } else {
                    *varVtVal = varExprEvalVal;
                }
                return *varVtVal;
            }
            case SType::Primary: {
                ASTPrimary* primary = (ASTPrimary*)ast;
                Value val = eval(primary->operand());
                assert(val.type() == ValueType::ValueRef);
                val = *val.ref();
                if(val.type() != ValueType::FunctionNode) { // it must be a function
                    return rst;
                } else {
                    std::vector<Value> args;
                    for(auto expr: primary->args()->expressions()) {
                        args.push_back(eval(expr));
                    }
                    Node const* node = val.node();
                    if(node->structType() == SType::Function) {
                        ASTFunction* func = (ASTFunction*)node;
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
                break;
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
        } else if(ap->type() == ValueType::Object) {
            // test
            // bp->type() == ValueType::
            // ap->operator[]()
        }
        else {
            assert(false && "unsupported type");
        }
        return Value();
    }

    Value Env::evalIdentifier(ASTIdentifier const* id) {
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