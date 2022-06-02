#include "vm_module.h"
#include "vm_object.h"
#include <vm/stdlib/std_vec.h>
#include <vm/stdlib/std_map.h>
#include <vm/vm_env.h>
#include <algorithm>


namespace compiler {

    DebugInfoNode* DebugInfoNode::addSubInfo(CodeDebugInfo info) {
        DebugInfoNode* node = new DebugInfoNode();
        node->_info.info = info;
        this->_subInfos.push_back(node);
        return node;
    }

    void DebugInfoNode::setRange(uint32_t beg, uint32_t end) {
        _info.beg = beg;
        _info.end = end;
    }

    void DebugInfoNode::setBegin(uint32_t beg) {
        _info.beg = beg;
    }

    void DebugInfoNode::setEnd(uint32_t end) {
        _info.end = end;
    }

    void DebugInfoNode::setInfo(CodeDebugInfo inf) {
        _info.info = inf;
    }

    DebugInfoNode::~DebugInfoNode() {
        for(auto info :_subInfos) {
            delete info;
        }
    }

    bool DebugInfoNode::find(uint32_t ip, std::pair<int,int>& out) {
        if(ip>=_info.beg && ip <=_info.end) {
            for(auto sub: _subInfos) {
                if(sub->find(ip, out)) {
                    return true;
                }
            }
            out.first = _info.info.line;
            out.second = _info.info.column;
            return true;
        }
        return false;
    }

    DebugInfo::Handle DebugInfo::newDbgInfo(CodeDebugInfo info) {
        DebugInfoNode* node = nullptr;
        if(_buildStack.size()) {
            node = _nodes.back()->addSubInfo(info);
        } else {
            node = new DebugInfoNode();
            node->setInfo(info);
            _nodes.push_back(node);
        }
        node->setBegin(_bytecode->size());
        _buildStack.push_back(node);
        return Handle(this);
    }

    std::pair<int,int> DebugInfo::locateIp(uint32_t ip) {
        std::pair<int,int> rst = {-1, -1};
        for(auto node: _nodes) {
            if(node->find(ip, rst)) {
                break;
            }
        }
        return rst;
    }

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

    void Module::addInitialize(uint32_t loc, Node* node) {
        if(node) {
            _initializeExprs.push_back(std::make_pair(loc, node));
        }
    }

    /**
     * @brief 
     *  把所有函数编译成字节码
     * 把所有全局变量初始化逻辑编译成字节码
     * 
     * @param env 
     */
    void Module::compileBytecode(Env* env) {
        auto errors = checkIdentifiers(env);
        if(errors.size()) {
            throw CompilingException(std::move(errors));
        }
        // IdLocateEnv locateEnv = {
        //     nullptr, 
        //     nullptr,
        //     _package.asObject()->symbolLayout(),
        //     env->root().asObject()->symbolLayout()
        // };
        Instruction returnInstr;
        returnInstr.opcode = (uint32_t)Opcode::Return;
        std::vector<BytecodeFunction*> compiledFunctions;
        std::vector<uint32_t> compiledFunctionOffsets;
        MultiExpr* multiExpr = _ast->asMultiExpr();
        // 处理所有的全局函数与类成员函数
        BytecodeFunction* bytecodeFunc = nullptr;
        for(auto const& expr :multiExpr->expressions()) {
            auto globalFunc = expr->asFunction();
            if(globalFunc) {
                // 为包内函数编译字节码
                auto name = globalFunc->name().stringLiteral();
                bytecodeFunc = _package[name].asBytecodeFunc();
                compiledFunctionOffsets.push_back(_bytecode.size()); // save offsets
                compiledFunctions.push_back(bytecodeFunc);
                _compileNode(globalFunc, &_bytecode);
                if(_bytecode.getInstr(_bytecode.size() - 1).opcode != (uint32_t)Opcode::Return) {
                    _bytecode.pushInstr(returnInstr);
                }
                bytecodeFunc->setArgc(globalFunc->params().size());
                bytecodeFunc->setSymbolLayout(globalFunc->symbolLayout());
            } else {
                // 为类成员函数编译字节码
                auto clazz = expr->asClass();
                if(clazz) {
                    auto body = clazz->body();
                    for(auto const& stmt : body->expressions()) {
                        auto memFunc = stmt->asFunction();
                        if(memFunc) {
                            Name className = clazz->name();
                            auto classObject = _package[className];
                            auto memFuncValue = classObject[memFunc->name().stringLiteral()];
                            bytecodeFunc = memFuncValue.asBytecodeFunc();
                            assert(bytecodeFunc);
                            compiledFunctionOffsets.push_back(_bytecode.size());
                            compiledFunctions.push_back(bytecodeFunc);
                            _compileNode(memFunc, &_bytecode);
                            // _funcInstrs.emplace_back();
                            if(_bytecode.getInstr(_bytecode.size() - 1).opcode == (uint32_t)Opcode::Return) {
                                _bytecode.pushInstr(returnInstr);
                            }
                            bytecodeFunc->setArgc(memFunc->params().size());
                            bytecodeFunc->setSymbolLayout(memFunc->symbolLayout());
                        }
                    }
                }
            }
        }
        // 为模块内所有的函数添加入口地址
        for(size_t i = 0; i<compiledFunctions.size(); ++i) {
            auto bytecodeFunc = compiledFunctions[i];
            auto offset = compiledFunctionOffsets[i];
            bytecodeFunc->setInstructionPtr(_bytecode.begin(), offset);
        }
        /**
         * @brief 为模块内的全局变量生成初始化字节码逻辑
         * 
         * @param _initializeExprs 
         */
        auto initializePos = _bytecode.size();
        for (auto& pair : _initializeExprs) {
            // auto loc = pair.first;
            auto node = pair.second;
            auto var = node->asVar();
            if(var->valueExpr()) {
                _compileNode(node, &_bytecode);
            }
        }
        _bytecode.pushInstr(returnInstr); // 给逻辑追加一个return指令结束调用
        // 为初始化逻辑生成一个BytecodeFunction对象
        _initializeFunc = new BytecodeFunction(env->createName("__initialize"), _package, this);
        _initializeFunc->setArgc(0);
        _initializeFunc->setInstructionPtr(_bytecode.begin(), initializePos);
        _initializeFunc->setSymbolLayout(nullptr);
        // clean up ast tree
        _initializeExprs.clear();
        delete _ast; _ast = nullptr;
        _astDebugInfos.clear();
    }

    void Module::setHostPackage(Value package) {
        _package = package;
    }

    void Module::setCodeDbgInfo(std::vector<CodeDebugInfo>&& dbgInfo) {
        _astDebugInfos = std::move(dbgInfo);
    }

    std::vector<Token> Module::checkIdentifiers(Env* env) {
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
                rst = _checkFunctions(env, func, locateEnv);
            } else {
                auto clazz = expr->asClass();
                if(clazz) {
                    locateEnv.classLayout = _package[clazz->name()].asObject()->symbolLayout();
                    auto body = clazz->body();
                    for(auto const& stmt : body->expressions()) {
                        auto func = stmt->asFunction();
                        if(func) {
                            auto errs = _checkFunctions(env, func, locateEnv);
                            rst.insert(rst.end(), errs.begin(), errs.end());
                        }
                    }
                }
            }
        }
        // 处理初始化
        locateEnv.classLayout = nullptr;
        locateEnv.functionLayout = nullptr;

        // ast::Function* initializeFunc = new ast::Function(VType::None);
        for(auto const& pair : _initializeExprs) {
            // auto loc = pair.first;
            auto node = pair.second;
            auto errs = _checkVars(env, node, locateEnv);
            rst.insert(rst.end(), errs.begin(), errs.end());
        }

        return rst;
    }

    std::vector<Token> Module::_checkVars(Env* env, Node const* ast, IdLocateEnv locateEnv) {
        assert(ast->valueType() == VType::Variable);

        std::vector<Token> compilerErrors;

        TraverseCallBack IdentifierTraverser = [&](compiler::Node const* node) {
            auto valueType = node->valueType();
            if(VType::Id == valueType) {
                auto id = node->asId();
                // IdLocateEnv locateEnv = {symLayout, func->hostPackage().asObject()->symbolLayout()};
                auto parent = node->parent();
                switch(parent->structType()) {
                    case SType::Pair: { // define variable        
                        switch(parent->valueType()) {
                            case VType::DotAccess:
                            case VType::FunctionCall:
                            case VType::IndexAccess:
                            case VType::Variable: {
                                if(!_locateIdentifier(locateEnv, id)) {
                                    assert(false);
                                    compilerErrors.push_back(id->token());
                                }
                            }
                            default: {
                                // skip
                                break;
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
                        if(!_locateIdentifier(locateEnv, id)) {
                            assert(false);
                            compilerErrors.push_back(id->token());
                        }
                        return;
                    }
                    default: {
                        if(!_locateIdentifier(locateEnv, id)) {
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
                        if(!_locateIdentifier(locateEnv, id)) {
                            assert(false);
                            compilerErrors.push_back(id->token());
                        }
                    }
                }
            }
        };
        // traverse the ast
        this->_traverseAST(ast, IdentifierTraverser);
        return compilerErrors;
    }

    std::vector<Token> Module::_checkFunctions(Env* env, Node const* ast, IdLocateEnv locateEnv) {
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

        TraverseCallBack IdentifierTraverser = [&](compiler::Node const* node) {
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
                            if(!_locateIdentifier(locateEnv, id)) {
                                assert(false);
                                compilerErrors.push_back(id->token());
                            }
                        } else if(parent->valueType() == VType::FunctionCall) {
                            if(!_locateIdentifier(locateEnv, id)) {
                                assert(false);
                                compilerErrors.push_back(id->token());
                            }
                        } else if(parent->valueType() == VType::IndexAccess) {
                            if(!_locateIdentifier(locateEnv, id)) {
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
                        if(!_locateIdentifier(locateEnv, id)) {
                            assert(false);
                            compilerErrors.push_back(id->token());
                        }
                        return;
                    }
                    default: {
                        if(!_locateIdentifier(locateEnv, id)) {
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
                        if(!_locateIdentifier(locateEnv, id)) {
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
        this->_traverseAST(ast, IdentifierTraverser);
        if(!compilerErrors.size()) {
            // Function* func = const_cast<Function*>((Function const*)ast);
        }
        func->_valid = !compilerErrors.size();
        func->_compiled = true;
        return compilerErrors;
    }

    bool Module::_locateIdentifier(IdLocateEnv env, Identifier const* id) {
        if(id->token().stringLiteral() == lang_keywords::_self) {
            id->setValue(IdentifierType::Self, 0);
            return true;
        }
        auto name = id->token().stringLiteral();
        uint32_t symbolLoc = ~0;
        if(env.functionLayout) {
            symbolLoc = env.functionLayout->querySymbolLoc(name);
        }
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

    void Module::_traverseAST(Node const* ast, TraverseCallBack& callBack) {
        switch(ast->structType()) {
            case SType::Function: {
                auto func = static_cast<Function const*>(ast);
                if(func->body()) {
                    callBack(func->body());
                    _traverseAST(func->body(), callBack);
                }
                break;
            }
            case SType::MultiExpr: {
                auto block = static_cast<MultiExpr const*>(ast);
                for(auto& expr : block->expressions()) {
                    callBack(expr);
                    _traverseAST(expr, callBack);
                }
                break;
            }
            case SType::If: {
                auto ifNode = static_cast<IfStmt const*>(ast);
                callBack(ifNode->condition());
                _traverseAST(ifNode->condition(), callBack);
                callBack(ifNode->thenBranch());
                _traverseAST(ifNode->thenBranch(), callBack);
                if(ifNode->elseBranch()) {
                    callBack(ifNode->elseBranch());
                    _traverseAST(ifNode->elseBranch(), callBack);
                }
                break;
            }
            case SType::BinaryOp: {
                auto binOp = static_cast<BinaryOpExpr const*>(ast);
                callBack(binOp->left());
                _traverseAST(binOp->left(), callBack);
                callBack(binOp->right());
                _traverseAST(binOp->right(), callBack);
                break;
            }
            case SType::Pair: {
                auto pair = static_cast<PairExpr const*>(ast);
                callBack(pair->first());
                _traverseAST(pair->first(), callBack);
                if(pair->second()) {
                    callBack(pair->second());
                    _traverseAST(pair->second(), callBack);
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
                    _traverseAST(ret->expr(), callBack);
                }
                break;
            }
            case SType::MapItem: {
                auto item = static_cast<MapItem const*>(ast);
                callBack(item->value());
                _traverseAST(item->value(), callBack);
                break;
            }
            case SType::Triple: {
                auto triple = static_cast<TripleExpr const*>(ast);
                if(triple->first()) {
                    callBack(triple->first());
                    _traverseAST(triple->first(), callBack);
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
                            _traverseAST(triple->second(), callBack);
                        }
                        break;
                    }
                }
                if(triple->third()) {
                    callBack(triple->third());
                    _traverseAST(triple->third(), callBack);
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

    void Module::_compileNode(ast::Node const* node, Bytecode* bytecode) {
        DebugInfo::Handle debugHandle(nullptr);
        if(-1 != node->dbgId()) {
            debugHandle = _debugInfo.newDbgInfo(_astDebugInfos[node->dbgId()]);
        }
        switch(node->structType()) {
            case SType::Function: {
                auto func = node->asFunction();
                _compileNode(func->body(), bytecode);
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
                _compileNode(item->value(), bytecode); // push value
                break;
            }
            case SType::MultiExpr: {
                auto block = node->asMultiExpr();
                switch(node->valueType()) {
                    case VType::Vector: {
                        for(auto& expr : block->expressions()) {
                            _compileNode(expr, bytecode); // map items
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
                            _compileNode(item, bytecode);
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
                            _compileNode(expr, bytecode);
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
                _compileNode(binOp->left(), bytecode);
                _compileNode(binOp->right(), bytecode);
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
                    _compileNode(ret->expr(), bytecode); // will push expr result to stack
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
                            _compileNode(var->id(), bytecode);
                            // eval the value to the top
                            _compileNode(var->valueExpr(), bytecode);
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
                        _compileNode(whileStmt->condition(), bytecode);
                        Instruction jz = {};
                        jz.jump.opcode = (uint32_t)Opcode::JumpZero;
                        jz.jump.pop = 1; // pop the condition result
                        bytecode->pushInstr(jz); // 跳转位置一会才能计算出来
                        size_t jzIdx = bytecode->size()-1;
                        _compileNode(whileStmt->body(), bytecode);
                        Instruction jmpToCond = {};
                        jmpToCond.jump.opcode = (uint32_t)Opcode::Jump;
                        jmpToCond.jump.pos = jumpBackPos;
                        bytecode->pushInstr(jmpToCond);
                        bytecode->getInstr(jzIdx).jump.pos = bytecode->size();
                        break;
                    }
                    case VType::DotAccess: {
                        auto dotAccess = node->asDotAccess();
                        _compileNode(dotAccess->obj(), bytecode);
                        _compileNode(dotAccess->field(), bytecode);
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
                        _compileNode(indexAccess->obj(), bytecode);
                        _compileNode(indexAccess->index(), bytecode);
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
                                _compileNode(arg, bytecode);
                            }
                        }
                        // 设置self寄存器
                        if(call->self()) {
                            _compileNode(call->self(), bytecode); // push self on stack
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
                            _compileNode(call->funcExpr(), bytecode); // 这里的funcExpr应该是一个field，字符串
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
                            _compileNode(call->funcExpr(), bytecode); 
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
                        _compileNode(newOp->self(), bytecode);
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
                _compileNode(ifStmt->condition(), bytecode);
                Instruction jz = {};
                jz.jump.opcode = (uint32_t)Opcode::JumpZero;
                jz.jump.pop = 1; // pop the condition result
                bytecode->pushInstr(jz);
                auto jzIdx = bytecode->size()-1;
                _compileNode(ifStmt->thenBranch(), bytecode);
                bytecode->getInstr(jzIdx).jump.pos = bytecode->size();
                if(ifStmt->elseBranch()) {
                    _compileNode(ifStmt->elseBranch(), bytecode);
                }
            }
            case SType::StringList: {
                break;
            }
            default: {
                assert(false);
            }
        }
        debugHandle.release();
    }

    void Module::initialize(Env* env) {
        // 初始化所有全局变量
        auto &stackFrames = env->stackFrames();
        stackFrames.push(Value()); // self
        stackFrames.precall(_initializeFunc, 0);
        env->execute();
        stackFrames.popFrame();
    }

    std::pair<int, int> Module::getIpDbgLoc(uint32_t ip) {
        return _debugInfo.locateIp(ip);
    }

}