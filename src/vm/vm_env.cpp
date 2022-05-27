#include <cassert>
#include "vm_env.h"
#include "stdlib/std_io.h"
// #include "stdlib/std_string.h"
#include "stdlib/std_vec.h"
#include "stdlib/std_map.h"
#include "internal_types/vm_primitive_types.h"
#include <sstream>

#include <iostream>

namespace compiler {

    Env::Env() {
        auto layout = newSymbolLayout(SymbolLayoutType::Package);
        _package = Value(layout);
        compiler::lang_keywords::init(this);
        compiler::lib_keywords::init(this);
        compiler::std_map_impl::init(this);
        compiler::std_vec_impl::init(this);
        io::init(this);
    }

    Name Env::createName(char const* str) {
        return _namePool.getName(str);
    }

    void Env::updateEvaluingNode(Node const* ast) {
        _funcEnvs.back().evaluingNode = ast;
    }
    
    Value Env::preparePackage( Node* ast ) {
        auto package = ast->asStringList();
        auto pack = root();
        for( auto name : package->names() ) {
            Object* parent = pack.asObject();
            auto layout = newSymbolLayout(SymbolLayoutType::Package);
            Value subpack(layout);
            parent->addSymbol(name.stringLiteral(), SymbolType::Package, subpack, Name());
            pack = subpack;
        }
        return pack;
    }

    void Env::traverseAST(Node const* ast, TraverseCallBack& callBack) {
        switch(ast->structType()) {
            case SType::Function: {
                auto func = static_cast<Function const*>(ast);
                callBack(func->body());
                traverseAST(func->body(), callBack);
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
                callBack(ifNode->elseBranch());
                traverseAST(ifNode->elseBranch(), callBack);
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
            default: {
                assert(false);
                break;
            }
        }
    }

    bool Env::compileCodeChunk(char const* mod, Node* ast, DebugInfoMap* debugInfoMap)  {
        if(ast->structType() == SType::MultiExpr) {
            MultiExpr* exprs = (MultiExpr*)ast;
            if(!exprs->expressions().size()) {
                return false;
            }
            auto iter = exprs->expressions().begin();
            Node* expr = *iter;
            if(expr->valueType() != VType::Package) {
                return false;
            }
            auto package = preparePackage(expr);
            auto packObj = package.asObject();
            auto moduleName = createName(mod);
            auto module = getModule(moduleName);
            module->setAst(ast);
            module->setHostPackage(package);
            module->setDebugInfo(std::move(*debugInfoMap));
            //
            ++iter;
            while(iter != exprs->expressions().end()) {
                auto expr = *iter;
                if(expr->structType() == SType::Function) {
                    Function* func = (Function*)expr;
                    BytecodeFunction* bcFunc = new BytecodeFunction(func->name().stringLiteral(), package, module);
                    auto rst = packObj->addSymbol(func->name().stringLiteral(), SymbolType::Function, Value(bcFunc), moduleName);
                    if(!rst.item) {
                        assert(false);
                        return false;
                    }
                } else if(expr->valueType() == VType::Variable ) {
                    Variable* var = (Variable*)expr;
                    // 注意这个地方，value不是ast节点，而是实际给了一个空值，占位。
                    auto rst = packObj->addSymbol(var->name().stringLiteral(), SymbolType::Variable, Value(), moduleName);
                    assert(rst.item);
                    if(var->valueExpr()) { // 创建一个特别的function，给var初始化，方便代码重用，处理
                        MultiExpr* funcBody = new MultiExpr(VType::Block);
                        funcBody->addExpr(var->valueExpr());
                        Function* func = new Function(VType::Closure);
                        func->setBody(funcBody);
                        func->setHostPackage(package);
                        func->setModule(moduleName);
                        module->addInitliaze(rst.loc, func); // 添加变量到初始化列表
                    }
                } else if(expr->structType() == SType::Class) { // 对 Class 节点处理
                    Class* clazz = (Class*)expr; 
                    auto clazzLayout = new SymbolLayout(SymbolLayoutType::Class);
                    auto clazzValue = Value(clazzLayout);
                    auto clazzObj = clazzValue.asObject();
                    for(auto expr : clazz->body()->expressions()) {
                        if(expr->structType() == SType::Function) {
                            Function* func = (Function*)expr;
                            BytecodeFunction* bcFunc = new BytecodeFunction(func->name().stringLiteral(), package, module);
                            auto rst = clazzObj->addSymbol(func->name().stringLiteral(), SymbolType::Function, Value(bcFunc), moduleName);
                            if(!rst.item) {
                                assert(false);
                                return false;
                            }
                        } else if(expr->valueType() == VType::Variable) {
                            Variable* var = (Variable*)expr;
                            // 不支持默认值
                            auto rst = clazzObj->addSymbol(var->name().stringLiteral(), SymbolType::Variable, Value(), moduleName);
                            if(!rst.item) {
                                assert(false && "add symbol failed!");
                                return false;
                            }
                        }
                    }
                    clazzLayout->reorderSymbols(); // 类需要重新排序
                    // 将class信息添加到包里
                    packObj->addSymbol(clazz->name(), SymbolType::Class, clazzValue, moduleName);
                }else {
                    assert(false && "only function & variable can be defined in package");
                    return false;
                }
                ++iter;
            }
            return true;
        } else {
            return false;
        }
    }

    bool Env::locateIdentifier(IdLocateEnv env, Identifier const* id) {
        auto name = id->token().stringLiteral();
        auto symbolLoc = env.functionLayout->querySymbolLoc(name);
        if(~symbolLoc != 0) { // local var
            id->setValue( IdentifierType::FunctionLocal, symbolLoc);
            return true;
        } else {
            symbolLoc = env.classLayout->querySymbolLoc(name);
            if(~symbolLoc != 0) {
                id->setValue(IdentifierType::ClassMember, symbolLoc);
                return true;
            } else { // current package var
                symbolLoc = env.packageLayout->querySymbolLoc(name);
                if(~symbolLoc != 0) {
                    id->setValue( IdentifierType::CurrentPackage, symbolLoc);
                    return true;
                }
                else { // global
                    symbolLoc = _package.asObject()->symbolLayout()->querySymbolLoc(name);
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


    int Env::call(int argc) {
        auto fn = stackFrames().topLocal(0).asBytecodeFunc();
        stackFrames().pop();
        stackFrames().precall(fn,argc);
        try {
            execute();
        } catch (DumpException& e) {
            std::cout << e.dumpMessage() << std::endl;
            return 0;
        }
        auto rst = stackFrames().retVal();
        rst.deref();
        stackFrames().popFrame();
        stackFrames().push(rst);
        return 1;
    }

    Value Env::callFuncWithPath(std::string const& func) {
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
                val = attr;
            } else {
                return Value();
            }
            if(pos == std::string::npos) {
                break;
            }
            last = pos + 1;
        } while(true);
        BytecodeFunction* byteFunc = val.asBytecodeFunc();
        Value rst;
        if(byteFunc) {
            auto& stack = stackFrames();
            stack.push(Value()); // push self (nil)
            stack.push(val); // push function
            call(0);
            rst = stack.retVal();
            stack.pop();
        }
        return rst;
    }

    Value Env::_valueInScope( ScopeType scope, uint32_t loc) {
        Value rst;
        switch(scope) {
            case ScopeType::Local: {
                rst = stackFrames().local(loc);
                break;
            }
            case ScopeType::Global: {
                rst = _package[loc];
                break;
            }
            case ScopeType::Member: {
                auto self = stackFrames().self();
                self.deref();
                rst = self[loc];
                break;
            }
            case ScopeType::Package: {
                auto pack = stackFrames().package();
                rst = pack[loc];
                break;
            }
            case ScopeType::Register: {
                rst = stackFrames().topLocalRef(loc);
                break;
            }
            case ScopeType::Constant: { 
                rst = stackFrames().constants()[loc];
                break;
            }
            case ScopeType::Self: {
                return stackFrames().self();
            }
            default: {
                assert(false);
                break;
            }
        }
        return rst;
    }

    void Env::_executeBinaryOp(Opcode op) {
        Value& left = stackFrames().topLocalRef(1);
        Value* leftRef = left.ref();
        Value& right = stackFrames().topLocalRef(0);
        right.deref();
        Value rst;
        if(leftRef->type() == PrimeVType::Int64) {
            IntegerValue* ival = (IntegerValue*)leftRef;
            rst = ival->Op(this, op, right);
        } else if(left.type() == PrimeVType::Float64) {
            // FloatValue const* fval = (FloatValue const*)&left;
            // return fval->Op(this, op, right);
        } else if(left.type() == PrimeVType::String) {
            // StringValue* sval = (StringValue*)ap;
            // return sval->Op(this, op, *bp);
        }
        else {
            assert(false && "unsupported type");
        }
        left.deref();
        left = rst;
        stackFrames().pop();
    }

    void Env::execute() {
        auto& frames = stackFrames();
        while(true) {
            auto const& instr = *frames.instr();
            Opcode code = (Opcode)instr.opcode;
            Value obj;
            Value field;
            Value val;
            switch(code) {
                case Opcode::Nop: {
                    break;
                }
                case Opcode::New: {
                    auto& self = frames.topLocalRef(0);
                    assert(self.type() == PrimeVType::Object && "");
                    val = Value(self.asObject()->symbolLayout());
                    self = val;
                    break;
                }
                case Opcode::Jump: {
                    frames.jump(instr.jump.pos);
                    continue;
                }
                case Opcode::JumpZero: {
                    val = frames.topLocal(0);
                    if(instr.jump.pop) {
                        frames.pop();
                    }
                    if(!val) {
                        frames.jump(instr.jump.pos);
                        continue;
                    }
                    break;
                }
                case Opcode::Push: {
                    val = _valueInScope((ScopeType)instr.srcType, instr.src);
                    frames.push(val);
                    break;
                }
                case Opcode::Pop: {
                    frames.popN(instr.pop);
                    break;
                }
                case Opcode::Add:
                case Opcode::Mul:
                case Opcode::Div:
                case Opcode::Less:
                case Opcode::Assign:
                case Opcode::Sub: {
                    _executeBinaryOp(code);
                    break;
                }
                case Opcode::Move: {
                    ScopeType dstType = (ScopeType)instr.dstType;
                    Value dst = _valueInScope(dstType, instr.dst);
                    assert(dst.type() == PrimeVType::ValueRef);
                    ScopeType srcType = (ScopeType)instr.srcType;
                    Value src = _valueInScope(srcType, instr.src);
                    src.deref();
                    dst = src;
                    if(instr.pop) {
                        stackFrames().popN(instr.pop);
                    }
                    break;
                }
                case Opcode::PushFrame: {
                    assert(false); // 现在没有了
                    // stackFrames().pushFrame(instr.src);
                    break;
                }
                case Opcode::Call: {
                    // auto func = _valueInScope((ScopeType)instr.srcType, instr.src);
                    // 先不支持不定参了
                    auto argCount = instr.src; // 知道参数个数，要重新设置fp,ap,lp的位置
                    auto func = stackFrames().topLocal(0);
                    func.deref();
                    stackFrames().pop();
                    BytecodeFunction const* bcFunc = func.asBytecodeFunc();
                    stackFrames().precall(bcFunc, argCount); // setup stack frame
                    if(bcFunc) { // is a bytecode function
                        continue; // fetch next instruction & execute!
                    }
                    // bridge function calling
                    BridgeFunc bridgeFunc = func.asBridgeFunc();
                    int rst = bridgeFunc(this);
                    auto topRef = stackFrames().topLocalRef(0);
                    stackFrames().popFrame();
                    stackFrames().push(topRef);
                    break;
                }
                case Opcode::Return: {
                    val = stackFrames().retVal();
                    val.decRef();
                    stackFrames().popFrame();
                    stackFrames().push(val);
                    if(!stackFrames().instr()) {
                        return;
                    }
                    break;
                }
                case Opcode::GetIndexed: {
                    obj = stackFrames().topLocal(1);
                    obj.deref();
                    if(
                        obj.type() != PrimeVType::Object ||
                        obj.type() != PrimeVType::Userdata
                    ) {
                        assert(false);
                        // throw RuntimeError("indexed operator can only be applied to object");
                    }
                    field = stackFrames().topLocal(0);
                    if(field.type() != PrimeVType::Int64) {
                        assert(false);
                        // throw RuntimeError("indexed operator can only be applied to string");
                    }
                    stackFrames().popN(2);
                    stackFrames().push(obj[field.intValue()]);
                    break;
                }
                case Opcode::GetField: {
                    if(instr.srcType == (uint32_t)ScopeType::Register) {
                        obj = stackFrames().topLocal(instr.src);
                        obj.deref();
                    } else if(instr.srcType == (uint32_t)ScopeType::Self) {
                        obj = stackFrames().self();
                        obj.deref();
                    } else {
                        assert(false);
                    }
                    if(instr.dstType == (uint32_t)ScopeType::Register) {
                        field = stackFrames().topLocal(instr.dst);
                        field.deref();
                    } else if(instr.dstType == (uint32_t)ScopeType::Constant) {
                        field = stackFrames().constants()[instr.dst];
                        field.deref();
                    } else {
                        assert(false);
                    }
                    if(
                        obj.type() != PrimeVType::Object &&
                        obj.type() != PrimeVType::Userdata
                    ) {
                        assert(false);
                        // throw RuntimeError("indexed operator can only be applied to object");
                    }
                    // Value field = stackFrames().topLocal(0);
                    if(field.type() != PrimeVType::String) {
                        assert(false);
                        // throw RuntimeError("indexed operator can only be applied to string");
                    }
                    if(instr.pop) {
                        stackFrames().popN(instr.pop);
                    }
                    stackFrames().push(obj[field.stringValue()]);
                    break;
                }
                default: {
                    assert(false && "unsupported instruction");
                    break;
                }
            }
            frames.peekIP();
        }

    }

    void Env::initializeModule(char const* module) {
        auto modName = createName(module);
        auto mod = getModule(modName);
        mod->initialize(this);
    }

    bool Env::postprocessModule(char const* module) {
        auto modName = createName(module);
        auto mod = getModule(modName);
        auto rst =  mod->postprocess(this);
        if(rst.size()) {
            return false;
        }
        return true;
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

    Module const* Env::getModule(Name const& name) const{
        auto it = _modules.find(name);
        if (it != _modules.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }

    std::string Env::backtrace(char const* errorType) const {
        std::stringstream ss;
        ss << "[backtrace] : " << errorType << std::endl;
        for(auto it = _funcEnvs.rbegin(); it != _funcEnvs.rend(); ++it) {
            auto module = getModule(it->func->module());
            auto debugInfo = module->debugInfo().find(it->evaluingNode);
            ss << "  ";
            ss << "[" << it->func->module().text() << "] :"; 
            ss << "" << it->func->name().stringLiteral().text() << "()";
            ss << debugInfo->second.line << ":" << debugInfo->second.column << std::endl;
        }
        return ss.str();
    }
}