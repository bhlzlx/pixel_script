#include "vm_env.h"
#include "vm_primitive_types.h"

namespace compiler {

    Value Env::eval(Node const* ast) {
        // MultiExpr,If,While,BinaryOp,Variable,Leaf,Primary,NegtiveOp,Pair,Function,StringList,
        Value nil;
        if(funcEnv()->retNow) {
            return Value();
        }
        switch(ast->structType()) {
            case SType::BinaryOp: {
                updateEvaluingNode(ast);
                BinaryOpExpr* binExpr = ast->asBinaryExpr();
                Value left = eval(binExpr->left());
                Value right = eval(binExpr->right());
                return evalBinaryOp(binExpr->op(), left, right);
            }
            case SType::If: {
                IfStmt* ifExpr = ast->asIf();
                Value cond = eval(ifExpr->condition());
                cond.deref();
                if(cond && ifExpr->thenBranch()) {
                    eval(ifExpr->thenBranch()); 
                } else if(ifExpr->elseBranch()) {
                    eval(ifExpr->elseBranch());
                }
                return nil;
            }
            case SType::MultiExpr: {
                MultiExpr* multiExpr = ast->asMultiExpr();
                for(auto expr: multiExpr->expressions()) {
                    eval(expr);
                }
                return nil;
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
                        // auto fenv = funcEnv();
                        auto loc = id->valueLoc();
                        Value varVtVal = _stackFrames.localValueRef(loc); // ref
                        Value varExprEvalVal = eval(var->valueExpr());
                        varVtVal = *varExprEvalVal.ref();
                        return varVtVal;
                    }
                    case VType::FunctionCall: {
                        updateEvaluingNode(ast);
                        Value argsItems[16];
                        FunctionCall* caller = ast->asFunctionCall();
                        Value val = eval(caller->methodExpr());
                        assert(val.type() == PrimeVType::FunctionNode || val.type() == PrimeVType::BridgeFunc);
                        val = *val.ref();
                        Node* methodExpr = caller->methodExpr();
                        // registed c/c++ function
                        if(val.type() == PrimeVType::BridgeFunc) { // it must be a function
                            _stackFrames.pushArgBegin();
                            for( auto& arg : caller->args()->expressions()) {
                                _stackFrames.pushValue(eval(arg));
                            }
                            _stackFrames.pushArgEnd();
                            int ret = val.asBridgeFunc()(this);
                            Value rst;
                            if(ret) {
                                rst = _stackFrames.popValue(); // 只取一个值
                            }
                            _stackFrames.popToArgBegin();
                            return rst;
                        // function defined in script
                        } else if(val.type() == PrimeVType::FunctionNode) {
                            Node const* node = val.node();
                            if(node->structType() == SType::Function) {
                                // 传递self对象，机智的我想到了这个办法
                                Value self;
                                if(methodExpr->valueType() == VType::DotAccess) {
                                    DotAccess* dotAccess = methodExpr->asDotAccess();
                                    self = *(eval(dotAccess->obj()).ref());
                                }
                                switch(node->valueType()) {
                                    case VType::None: { // 普通函数调用（全局函数以及类函数）
                                        _stackFrames.pushArgBegin();
                                        if(self && self.stype() == SymbolLayoutType::Class) {
                                            _stackFrames.pushValue(std::move(self));// 传递this指针！
                                        }
                                        _stackFrames.pushArgEnd();
                                        // 传递参数
                                        if(caller->args()) {
                                            for(auto expr: caller->args()->expressions()) {
                                                _stackFrames.pushValue(eval(expr));
                                            }
                                        }
                                        auto rstCount = callFunction(val);
                                        assert(rstCount == 1);
                                        Value rst = _stackFrames.popValue();
                                        _stackFrames.popToArgBegin();
                                        return rst;
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
                        } else {
                            return Value();
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
                        updateEvaluingNode(ast);
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
            case SType::Return: {
                ReturnStmt* returnStmt = ast->asReturn();
                if(returnStmt->expr()) {
                    auto ret = eval(returnStmt->expr());
                    _stackFrames.pushValue(ret);
                }
                _funcEnvs.back().retNow = true;
                return Value();
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
                        case TokenType::True: {
                            leafRst.setBool(true);
                            break;
                        }
                        case TokenType::False: {
                            leafRst.setBool(false);
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
            return ival->Op(this, op, *bp);
        } else if(ap->type() == PrimeVType::Float64) {
            FloatValue const* fval = (FloatValue const*)ap;
            return fval->Op(this, op, *bp);
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
                assert(loc < _stackFrames.topFrameSize());
                rst = _stackFrames.localValueRef(loc); // create a ref
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
                auto self = _stackFrames.localValue(0);// 第一个参数就是self //self is a ref
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