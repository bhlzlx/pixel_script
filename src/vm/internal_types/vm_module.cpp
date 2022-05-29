#include "vm_module.h"
#include "vm_object.h"
#include <vm/stdlib/std_vec.h>
#include <vm/stdlib/std_map.h>
#include <vm/vm_env.h>


namespace compiler {

    Opcode tokenToBinaryOpcode(TokenType type) {
        // convert token to binary opcode
        switch(type) {
            case TokenType::Plus:
                return Opcode::Add;
            case TokenType::Minus:
                return Opcode::Sub;
            case TokenType::Star:
                return Opcode::Mul;
            case TokenType::Less:
                return Opcode::Less;
            case TokenType::Greater:
                return Opcode::Greater;
            case TokenType::LessEqual:
                return Opcode::LessEqual;
            case TokenType::GreaterEqual:
                return Opcode::GreaterEqual;
            case TokenType::NotEqual:
                return Opcode::NotEqual;
            case TokenType::Equal:
                return Opcode::Equal;
            case TokenType::Slash:
                return Opcode::Div;
            case TokenType::Assign:
                return Opcode::Assign;
            // case TokenType::Mod:
            //     return Opcode::Mod;
            default:
                return Opcode::Nop;
        }
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
        Bytecode bytecode;
        MultiExpr* multiExpr = _ast->asMultiExpr();
        // 处理所有的全局函数与类成员函数
        for(auto const& expr :multiExpr->expressions()) {
            auto globalFunc = expr->asFunction();
            if(globalFunc) {
                // 全局函数 
                auto name = globalFunc->name().stringLiteral();
                auto byteFunc = _package[name].asBytecodeFunc();
                compileNode(globalFunc, &bytecode);
                _funcInstrs.emplace_back();
                bytecode.exportInstr(_funcInstrs.back());
                if(_funcInstrs.back().back().opcode != (uint32_t)Opcode::Return) {
                    Instruction instr;
                    instr.opcode = (uint32_t)Opcode::Return;
                    _funcInstrs.back().push_back(instr);
                }
                byteFunc->setArgc(globalFunc->params().size());
                byteFunc->setSymbolLayout(globalFunc->symbolLayout());
                byteFunc->setInstruction(_funcInstrs.back().data());
            } else {
                auto clazz = expr->asClass();
                if(clazz) {
                    auto body = clazz->body();
                    for(auto const& stmt : body->expressions()) {
                        auto memFunc = stmt->asFunction();
                        if(memFunc) {
                            Name className = clazz->name();
                            auto classObject = _package[className];
                            auto memFuncValue = classObject[memFunc->name().stringLiteral()];
                            auto bytecodeFunc = memFuncValue.asBytecodeFunc();
                            assert(bytecodeFunc);
                            compileNode(memFunc, &bytecode);
                            _funcInstrs.emplace_back();
                            bytecode.exportInstr(_funcInstrs.back());
                            if(_funcInstrs.back().back().opcode != (uint32_t)Opcode::Return) {
                                Instruction instr;
                                instr.opcode = (uint32_t)Opcode::Return;
                                _funcInstrs.back().push_back(instr);
                            }
                            bytecodeFunc->setArgc(memFunc->params().size());
                            bytecodeFunc->setSymbolLayout(memFunc->symbolLayout());
                            bytecodeFunc->setInstruction(_funcInstrs.back().data());
                        }
                    }
                }
            }
        }
        bytecode.exportConstants(_constants);
        // for (auto& pair : _initliazeList) {
        //     auto loc = pair.first;
        //     auto node = pair.second;
        //     postprocessFunction(env, node, locateEnv);
        //     Value valRef = _package[loc];
        //     int retCount = env->callBytecodeFunc(Value(node));
        //     assert(retCount == 1);
        //     valRef = env->stackFrames().topLocal(0);
        //     // valRef = env->stackValues().topLocal();
        // }
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
        MultiExpr* multiExpr = _ast->asMultiExpr();
        // 处理所有的全局函数与类成员函数
        for(auto const& expr :multiExpr->expressions()) {
            auto func = expr->asFunction();
            if(func) {
                rst = postprocessFunction(env, func, locateEnv);
            } else {
                auto clazz = expr->asClass();
                if(clazz) {
                    locateEnv.classLayout = _package[clazz->name()].asObject()->symbolLayout();
                    auto body = clazz->body();
                    for(auto const& stmt : body->expressions()) {
                        auto func = stmt->asFunction();
                        if(func) {
                            auto errs = postprocessFunction(env, func, locateEnv);
                            rst.insert(rst.end(), errs.begin(), errs.end());
                        }
                    }
                }
            }
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
                assert(false);
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

        /**
         * @brief 
         *   现在我们又有了新的后处理需求，需要看函数调用引用的self对象，如果是成员函数，则需要给FunctionCall添加self节点
         */

        TraverseCallBack processor = [&](compiler::Node const* node) {
            auto valueType = node->valueType();
            if(VType::Id == valueType) {
                auto id = node->asId();
                // IdLocateEnv locateEnv = {symLayout, func->hostPackage().asObject()->symbolLayout()};
                auto parent = node->parent();
                switch(parent->structType()) {
                    case SType::Pair: { // define variable        
                        if(parent->valueType() == VType::Variable) {
                            auto regRst = symLayout->regSymbol(id->token().stringLiteral(), SymbolType::Variable, Value(), func->module());
                            id->setValue(IdentifierType::FunctionLocal, regRst.loc);
                        } else if(parent->valueType() == VType::DotAccess) {
                            if(!locateIdentifier(locateEnv, id)) {
                                assert(false);
                                compilerErrors.push_back(id->token());
                            }
                        } else if(parent->valueType() == VType::FunctionCall) {
                            if(!locateIdentifier(locateEnv, id)) {
                                assert(false);
                                compilerErrors.push_back(id->token());
                            }
                        } else if(parent->valueType() == VType::IndexAccess) {
                            if(!locateIdentifier(locateEnv, id)) {
                                assert(false);
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
                            assert(false);
                            compilerErrors.push_back(id->token());
                        }
                        return;
                    }
                    default: {
                        if(!locateIdentifier(locateEnv, id)) {
                            assert(false);
                            compilerErrors.push_back(id->token());
                        }
                    }
                }
            } else if(VType::FunctionCall == valueType) {
                auto funcCall = node->asFunctionCall();
                auto self = funcCall->self();
                if(!self) { // 如果没有self，且id是类成员函数，则需要添加self对象
                    auto funcExpr = funcCall->funcExpr();
                    if(funcExpr->valueType() == VType::Id) {
                        auto id = funcExpr->asId();
                        if(!locateIdentifier(locateEnv, id)) {
                            assert(false);
                            compilerErrors.push_back(id->token());
                        }
                        // if(id->type() == IdentifierType::ClassMember) {
                        //     auto selfNode = new ScopeNode(IdentifierType::ClassMember);
                        //     funcCall->setSelf(selfNode);
                        // }
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
        if(id->token().stringLiteral() == lang_keywords::_self) {
            id->setValue(IdentifierType::Self, 0);
            return true;
        }
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
            case SType::MapItem: {
                auto item = static_cast<MapItem const*>(ast);
                callBack(item->value());
                traverseAST(item->value(), callBack);
                break;
            }
            case SType::Triple: {
                auto triple = static_cast<TripleExpr const*>(ast);
                if(triple->first()) {
                    callBack(triple->first());
                    traverseAST(triple->first(), callBack);
                }
                auto valueType = ast->valueType();
                switch (valueType)
                {
                    case VType::FunctionCall: {
                        break;
                    }
                    default: {
                        if(triple->second()) {
                            callBack(triple->second());
                            traverseAST(triple->second(), callBack);
                        }
                        break;
                    }
                }
                if(triple->third()) {
                    callBack(triple->third());
                    traverseAST(triple->third(), callBack);
                }
                break;
            }
            // case SType::Scope: {
            //     break;
            // }
            default: {
                assert(false);
                break;
            }
        }
    }

    /**
     * @brief 
     *      函数内的语句块，有可能有压栈的效果，比如一个函数执行完，没有变量接收，
     * 或者只写了一个变量，这样并没有语法错误的，但是还是会有一个压栈的效果，所以我们需要判断这个语句有没有压栈的效果
     * 如果有压栈的效果，则手动给它一个弹栈的操作。
     * 
     * @param node 
     * @return true 
     * @return false 
     */

    bool needPopInstr(ast::Node const* node) {
        switch(node->structType()) {
            // 不需要pop的语句
            case SType::If:
            case SType::Return:
            // 需要pop的语句
            case SType::Leaf:
            case SType::NegtiveOp:
            case SType::BinaryOp: {
                return true;
            }
            case SType::Pair: {
                switch(node->valueType()) {
                    // 不需要pop
                    case VType::Variable:
                    case VType::WhileStmt: {
                        return false;
                    }
                    // 需要pop
                    case VType::DotAccess:
                    case VType::IndexAccess: {
                        return true;
                    }
                    default: {
                        assert(false && "unhandled");
                        return false;
                    }
                }
            }
            case SType::Triple: {
                switch(node->valueType()) {
                    case VType::FunctionCall: {
                        return true;
                    }
                    default: {
                        assert(false && "unhandled");
                        return false;
                    }
                }
            }
            default: {
                assert(false && "unhandled");
                return false;
            }
        }
    }

    void Module::compileNode(ast::Node const* node, Bytecode* bytecode) {
        switch(node->structType()) {
            case SType::Function: {
                auto func = node->asFunction();
                compileNode(func->body(), bytecode);
                break;
            }
            case SType::MapItem: {
                auto item = node->asMapItem();
                Name key = item->key();
                Instruction pushName = {};
                pushName.opcode = (uint32_t)Opcode::Push;
                pushName.src = bytecode->getConstant(Value(key));
                pushName.srcType = (uint32_t)ScopeType::Constant;
                bytecode->pushInstr(pushName); // push key
                compileNode(item->value(), bytecode); // push value
                break;
            }
            case SType::MultiExpr: {
                auto block = node->asMultiExpr();
                switch(node->valueType()) {
                    case VType::Vector: {
                        for(auto& expr : block->expressions()) {
                            compileNode(expr, bytecode); // map items
                        }
                        Instruction pushFunc;
                        pushFunc.opcode = (uint32_t)Opcode::Push;
                        pushFunc.src = bytecode->getConstant(Value(std_vec_impl::create));
                        pushFunc.srcType = (uint32_t)ScopeType::Constant;
                        Instruction call;
                        call.opcode = (uint32_t)Opcode::Call;
                        call.src = block->expressions().size();
                        bytecode->pushInstr(pushFunc); // self， 随便给一个实际不会用的
                        bytecode->pushInstr(pushFunc); // func
                        bytecode->pushInstr(call);
                        break;
                    }
                    case VType::Map: {
                        for(auto& item : block->expressions()) {
                            compileNode(item, bytecode);
                        }
                        Instruction pushFunc;
                        pushFunc.opcode = (uint32_t)Opcode::Push;
                        pushFunc.src = bytecode->getConstant(Value(std_map_impl::create));
                        pushFunc.srcType = (uint32_t)ScopeType::Constant;
                        Instruction call;
                        call.opcode = (uint32_t)Opcode::Call;
                        call.src = block->expressions().size() * 2;
                        bytecode->pushInstr(pushFunc); // self， 随便给一个实际不会用的
                        bytecode->pushInstr(pushFunc); // func
                        bytecode->pushInstr(call);
                        break;
                    }
                    case VType::Block: {
                        for(auto& expr : block->expressions()) {
                            compileNode(expr, bytecode);
                            Instruction pop = {};
                            pop.opcode = (uint32_t)Opcode::Pop;
                            pop.pop = 1;
                            if(needPopInstr(expr)) {
                                bytecode->pushInstr(pop);// pop the result of the last expression
                            }
                        }
                        break;
                    }
                    default:
                        assert(false);
                        break;
                    }
                break;
            }
            case SType::BinaryOp: {
                auto binOp = node->asBinaryExpr();
                compileNode(binOp->left(), bytecode);
                compileNode(binOp->right(), bytecode);
                if(binOp->op() == TokenType::Assign) {
                    Instruction assign = {};
                    assign.opcode = (uint32_t)Opcode::Move;
                    assign.srcType = (uint32_t)ScopeType::Register;
                    assign.src = 0;
                    assign.dstType = (uint32_t)ScopeType::Register;
                    assign.dst = 1;
                    assign.pop = 1;
                    bytecode->pushInstr(assign);
                } else {
                    Instruction instr;
                    instr.opcode = (uint32_t)tokenToBinaryOpcode(binOp->op());
                    bytecode->pushInstr(instr);
                }
                break;
            }
            case SType::Leaf: {
                auto leaf = node->asLeaf();
                Instruction instr = {};
                instr.opcode = (uint32_t)Opcode::Push; // push specific value to top
                if(leaf->valueType() == VType::Id) {
                    auto id = leaf->asId();
                    instr.src = id->valueLoc();
                    switch(id->type()) {
                        case IdentifierType::FunctionLocal: {
                            instr.srcType = (uint8_t)ScopeType::Local;
                            break;
                        }
                        case IdentifierType::ClassMember: {
                            instr.srcType = (uint8_t)ScopeType::Member;
                            break;
                        }
                        case IdentifierType::CurrentPackage: {
                            instr.srcType = (uint8_t)ScopeType::Package;
                            break;
                        }
                        case IdentifierType::Global: {
                            instr.srcType = (uint8_t)ScopeType::Global;
                            break;
                        }
                        case IdentifierType::Self: {
                            instr.srcType = (uint8_t)ScopeType::Self;
                            break;
                        }
                        default: {
                            assert(false);
                            break;
                        }
                    }
                } else { 
                    uint32_t constLoc = 0;
                    if(leaf->token().type() == TokenType::Integer) {
                        constLoc = bytecode->getConstant(Value((int64_t)leaf->token().integerLiteral()));
                    } else if(leaf->token().type() == TokenType::Float) {
                        constLoc = bytecode->getConstant(Value(leaf->token().floatLiteral()));
                    } else if(leaf->token().type() == TokenType::String) {
                        constLoc = bytecode->getConstant(Value(leaf->token().stringLiteral()));
                    } else if(leaf->token().type() == TokenType::True) {
                        constLoc = bytecode->getConstant(Value(true));
                    } else if(leaf->token().type() == TokenType::False) {
                        constLoc = bytecode->getConstant(Value(false));
                    } else if(leaf->valueType() == VType::Field) {
                        constLoc = bytecode->getConstant(Value(leaf->token().stringLiteral()));
                    } else {
                        assert(false);
                    }
                    instr.src = constLoc;
                    instr.srcType = (uint8_t)ScopeType::Constant;
                }
                bytecode->pushInstr(instr);
                break;
            }
            case SType::Return: {
                auto ret = node->asReturn();
                if(ret->expr()) {
                    compileNode(ret->expr(), bytecode); // will push expr result to stack
                }
                Instruction instr;
                instr.opcode = (uint32_t)Opcode::Return; // jump to the last frame pc+1, 
                bytecode->pushInstr(instr);
                break;
            }
            case SType::Pair: {
                switch(node->valueType()) {
                    case VType::Variable: {
                        auto var = node->asVar();
                        if(var->valueExpr()) {
                            // eval the var's ref to the top 
                            compileNode(var->id(), bytecode);
                            // eval the value to the top
                            compileNode(var->valueExpr(), bytecode);
                            Instruction instr;// move value to variable && pop the value
                            instr.opcode = (uint32_t)Opcode::Move; 
                            instr.src = 0;
                            instr.srcType = (uint8_t)ScopeType::Register;
                            instr.dst = 1;
                            instr.dstType = (uint8_t)ScopeType::Register;
                            instr.pop = 1;
                            bytecode->pushInstr(instr); 
                            Instruction pop;
                            pop.opcode = (uint32_t)Opcode::Pop;
                            pop.pop = 1;
                            bytecode->pushInstr(pop);
                            // pop??
                        }
                        break;
                    }
                    case VType::WhileStmt: {
                        auto whileStmt = node->asWhile();
                        uint32_t jumpBackPos = bytecode->size();
                        compileNode(whileStmt->condition(), bytecode);
                        Instruction jz = {};
                        jz.jump.opcode = (uint32_t)Opcode::JumpZero;
                        jz.jump.pop = 1; // pop the condition result
                        bytecode->pushInstr(jz); // 跳转位置一会才能计算出来
                        size_t jzIdx = bytecode->size()-1;
                        compileNode(whileStmt->body(), bytecode);
                        Instruction jmpToCond = {};
                        jmpToCond.jump.opcode = (uint32_t)Opcode::Jump;
                        jmpToCond.jump.pos = jumpBackPos;
                        bytecode->pushInstr(jmpToCond);
                        bytecode->getInstr(jzIdx).jump.pos = bytecode->size();
                        break;
                    }
                    case VType::DotAccess: {
                        auto dotAccess = node->asDotAccess();
                        compileNode(dotAccess->obj(), bytecode);
                        compileNode(dotAccess->field(), bytecode);
                        Instruction instr;
                        instr.opcode = (uint32_t)Opcode::GetField;
                        instr.src = 1;
                        instr.srcType = (uint8_t)ScopeType::Register;
                        instr.dst = 0;
                        instr.dstType = (uint8_t)ScopeType::Register;
                        instr.pop = 2;
                        bytecode->pushInstr(instr);
                        break;
                    }
                    case VType::IndexAccess: {
                        auto indexAccess = node->asIndexAccess();
                        compileNode(indexAccess->obj(), bytecode);
                        compileNode(indexAccess->index(), bytecode);
                        Instruction instr;
                        instr.opcode = (uint32_t)Opcode::GetIndexed;
                        bytecode->pushInstr(instr);
                        break;
                    }
                    default: {
                        assert(false);
                    }
                }
                break;
            }
            // fp - args - sp
            case SType::Triple: {
                switch(node->valueType()) {
                    case VType::FunctionCall: {
                        // 注意这里，我们要先计算参数值，压栈，然后再把self压栈，然后压函数，执行调用
                        auto call = node->asFunctionCall();
                        // 计算并压入所有的参数
                        auto args = call->args();
                        if(args) {
                            for(auto& arg : args->expressions()) {
                                compileNode(arg, bytecode);
                            }
                        }
                        // 设置self寄存器
                        if(call->self()) {
                            compileNode(call->self(), bytecode); // push self on stack
                        } else {
                            // 直接拿当前的self传
                            Instruction instr = {};
                            instr.opcode = (uint32_t)Opcode::Push;
                            instr.src = 0; 
                            instr.srcType = (uint8_t)ScopeType::Self;
                            instr.dst = 1; // readonly
                            bytecode->pushInstr(instr);
                        }
                        // 压入函数对象
                        if(call->self()) { 
                            // a.b.func(args)
                            assert(call->funcExpr()->asLeaf() && "must be a leaf!");
                            compileNode(call->funcExpr(), bytecode); // 这里的funcExpr应该是一个field，字符串
                            Instruction getField = {};
                            getField.opcode = (uint32_t)Opcode::GetField;
                            getField.src = 1; // self的位置
                            getField.srcType = (uint8_t)ScopeType::Register;
                            getField.dst = 0; // field 位置
                            getField.dstType = (uint8_t)ScopeType::Register;
                            getField.pop = 1; // pop field
                            bytecode->pushInstr(getField);
                        } else {
                            // a.b.d()(args) 调用一个方法返回的函数对象，一定没有this，而且这个值是动态获取的
                            compileNode(call->funcExpr(), bytecode); 
                        }
                        // 这样，栈上的状态是 args|method object，然后我们再去调用这个函数
                        Instruction callFunc;
                        callFunc.opcode = (uint32_t)Opcode::Call;
                        callFunc.src = args ? args->expressions().size() : 0;
                        bytecode->pushInstr(callFunc);
                        // 函数调用完，ip的偏移量
                        // 函数调用完之后，返回到上一个函数栈的状态，但是ip需要重定位到调用函数后的指令上
                        // 所以就需要这个偏移量
                        break;
                    }
                    case VType::NewOperator: {
                        auto newOp = node->asFunctionCall();
                        compileNode(newOp->self(), bytecode);
                        Instruction newObj = {};
                        newObj.opcode = (uint32_t)Opcode::New;
                        newObj.src = 0;
                        newObj.srcType = (uint8_t)ScopeType::Register;
                        newObj.dst = 0;
                        newObj.dstType = (uint8_t)ScopeType::Register;
                        newObj.pop = 1;
                        bytecode->pushInstr(newObj);
                        break;
                    }
                    default: {
                        assert(false);
                    }
                }
                break;
            }
            case SType::If: {
                auto ifStmt = node->asIf();
                compileNode(ifStmt->condition(), bytecode);
                Instruction jz = {};
                jz.jump.opcode = (uint32_t)Opcode::JumpZero;
                jz.jump.pop = 1; // pop the condition result
                bytecode->pushInstr(jz);
                auto jzIdx = bytecode->size()-1;
                compileNode(ifStmt->thenBranch(), bytecode);
                bytecode->getInstr(jzIdx).jump.pos = bytecode->size();
                if(ifStmt->elseBranch()) {
                    compileNode(ifStmt->elseBranch(), bytecode);
                }
            }
            case SType::StringList: {
                break;
            }
            default: {
                assert(false);
            }
        }
    }

}