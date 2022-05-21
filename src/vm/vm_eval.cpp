#include "vm_env.h"
#include <vm/internal_types/vm_primitive_types.h>
#include <vm/internal_types/vm_userdata.h>
#include "stdlib/std_vec.h"
#include "stdlib/std_map.h"

namespace compiler {

    Value Env::eval(Node const* ast) {
        // MultiExpr,If,While,BinaryOp,Variable,Leaf,Primary,NegtiveOp,Pair,Function,StringList,
        Value nil;
        if(funcEnv()->retNow) {
            return Value();
        }
        switch(ast->structType()) {
            case SType::BinaryOp: {
                BinaryOpExpr* binExpr = ast->asBinaryExpr();
                Value left = eval(binExpr->left());
                Value right = eval(binExpr->right());
                updateEvaluingNode(ast);
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
                switch(ast->valueType()) {
                    case VType::Vector: {
                        Value ret = nil;
                        UserdataObject* vec = std_vec_impl::create(this);
                        try {
                            for(Node const* node : multiExpr->expressions()) {
                                auto eleVal = eval(node);
                                eleVal.deref();
                                std_vec_impl::__privateAdd(vec, eleVal);
                            }
                        } catch(...) {
                            vec->decRef(); // clean up the vec object
                            throw;
                        }
                        ret = Value(vec);
                        return ret;
                    }
                    case VType::Map: {
                        Value ret = nil;
                        UserdataObject* map = std_map_impl::create(this);
                        try {
                            for(Node const* node : multiExpr->expressions()) {
                                MapItem* item = node->asMapItem();
                                Value key = item->key();
                                Value val = eval(item->value());
                                val.deref();
                                std_map_impl::__privateAdd(map, key, val);
                            }
                        } catch(...) {
                            map->decRef(); // clean up the map object
                            throw;
                        }
                        ret = Value(map);
                        return ret;
                    }
                    default: {
                        Value rst;
                        for(auto expr: multiExpr->expressions()) {
                            rst = eval(expr);
                            rst.deref(); // 这里一定要注意，因为这里的rst可能是一个引用，所以要deref，如果不deref，后果严重，想想这里的逻辑！
                        }
                        return rst;
                    }
                }
                return nil;
            }
            case SType::NegtiveOp: {
                NegativeExpr* negtiveOp = ast->asNegativeExpr();
                Value val = eval(negtiveOp->value());
                Value rst;
                updateEvaluingNode(ast);
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
                        varExprEvalVal.deref();
                        varVtVal = varExprEvalVal;
                        return varVtVal;
                    }
                    case VType::FunctionCall: {
                        Value argsItems[16];
                        FunctionCall* caller = ast->asFunctionCall();
                        Value val = eval(caller->methodExpr());
                        updateEvaluingNode(ast);
                        assert(val.type() == PrimeVType::FunctionNode || val.type() == PrimeVType::BridgeFunc);
                        val = *val.ref();
                        Node* methodExpr = caller->methodExpr();
                        // registed c/c++ function
                        if(val.type() == PrimeVType::BridgeFunc) { // it must be a function
                            _stackFrames.pushArgBegin();
                            // 传递self对象，机智的我想到了这个办法
                            Value self;
                            if(methodExpr->valueType() == VType::DotAccess) {
                                DotAccess* dotAccess = methodExpr->asDotAccess();
                                self = *(eval(dotAccess->obj()).ref());
                                if(self.type() == PrimeVType::Userdata) {
                                    _stackFrames.pushValue(std::move(self));// 传递this指针！
                                }
                            }
                            if(caller->args()) {
                                for( auto& arg : caller->args()->expressions()) {
                                    _stackFrames.pushValue(eval(arg));
                                }
                            }
                            _stackFrames.pushArgEnd();
                            int ret = val.asBridgeFunc()(this);
                            Value bridgeRst;
                            if(ret) {
                                bridgeRst = _stackFrames.popValue(); // 只取一个值
                            }
                            _stackFrames.popToArgBegin();
                            return bridgeRst;
                        // function defined in script
                        } else if(val.type() == PrimeVType::FunctionNode) {
                            Node const* node = val.node();
                            if(node->structType() == SType::Function) {
                                switch(node->valueType()) {
                                    case VType::None: { // 普通函数调用（全局函数以及类函数）
                                        // 传递self对象，机智的我想到了这个办法
                                        Value self;
                                        if(methodExpr->valueType() == VType::DotAccess) {
                                            DotAccess* dotAccess = methodExpr->asDotAccess();
                                            self = *(eval(dotAccess->obj()).ref());
                                        }
                                        _stackFrames.pushArgBegin();
                                        if(self.stype() == SymbolLayoutType::Class) {
                                            _stackFrames.pushValue(std::move(self));// 传递this指针！
                                        }
                                        // 传递参数
                                        if(caller->args()) {
                                            for(auto expr: caller->args()->expressions()) {
                                                _stackFrames.pushValue(eval(expr));
                                            }
                                        }
                                        _stackFrames.pushArgEnd();
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
                        DotAccess* dotAccess = ast->asDotAccess();
                        Value obj = eval(dotAccess->obj()); // object must be a ref
                        Value* objPtr = obj.ref();
                        auto fieldLeaf = dotAccess->field()->asLeaf();
                        assert(fieldLeaf); assert(fieldLeaf->valueType() == VType::String);
                        Name fieldName = fieldLeaf->token().stringLiteral();
                        updateEvaluingNode(ast);
                        Value valPtr = (*objPtr)[fieldName]; // we should return the value's ref
                        return valPtr;
                    }
                    case VType::IndexAccess: {
                        IndexAccess* indexAccess = ast->asIndexAccess();
                        Value vec = eval(indexAccess->obj()); // object must be a ref
                        vec.deref();
                        if(vec.type() != PrimeVType::Userdata) {
                            DumpException except(this,  ExecutionError::IndexANoneObject, "index a none object!");
                            throw except;
                        }
                        Value index = eval(indexAccess->index());
                        updateEvaluingNode(ast);
                        // push args
                        _stackFrames.pushArgBegin();
                        _stackFrames.pushValue(vec);
                        _stackFrames.pushValue(Value(index));
                        _stackFrames.pushArgEnd();
                        Value rst = vec.indexAccess(this);
                        _stackFrames.popToArgBegin();
                        return rst;
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
        } else if(ap->type() == PrimeVType::String) {
            StringValue* sval = (StringValue*)ap;
            return sval->Op(this, op, *bp);
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