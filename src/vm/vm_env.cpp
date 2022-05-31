#include <cassert>
#include "vm_env.h"
#include "stdlib/stdlib.h"
#include "internal_types/vm_userdata.h"
#include "internal_types/vm_primitive_types.h"
#include <sstream>

#include <iostream>

namespace compiler {

    Env::Env() {
        auto layout = newSymbolLayout(SymbolLayoutType::Package);
        _package = Value(layout);
        compiler::lang_keywords::init(this);
        compiler::lib_keywords::init(this);
        compiler::stdlib::init(this);
    }

    Name Env::createName(char const* str) {
        return _namePool.getName(str);
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

    bool Env::preprocessModuleAST(char const* mod, Node* ast, DebugInfoMap* debugInfoMap)  {
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
            // module->setDebugInfo(std::move(*debugInfoMap));
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
                        module->addInitialize(rst.loc, var); // 添加变量到初始化列表
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

    Value Env::_valueInScope( ScopeType scope, uint32_t loc, bool readonly) {
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
                auto self = stackFrames().selfRef();
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
                Value& regVal = stackFrames().topLocalRef(loc);
                if(regVal.type() == PrimeVType::ValueRef) {
                    rst = regVal;
                } else {
                    rst = &regVal;
                }
                break;
            }
            case ScopeType::Constant: { 
                rst = stackFrames().constants()[loc];
                break;
            }
            case ScopeType::Self: {
                rst = stackFrames().selfRef();
                break;
            }
            default: {
                assert(false);
                break;
            }
        }
        if(readonly) {
            rst.deref();
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
        } else if(leftRef->type() == PrimeVType::Float64) {
            FloatValue* fval = (FloatValue*)&left;
            fval->Op(this, op, right);
        } else if(leftRef->type() == PrimeVType::String) {
            StringValue* sval = (StringValue*)&left;
            sval->Op(this, op, right);
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
                    auto type = (ScopeType)instr.srcType;
                    val = _valueInScope(type, instr.src, !!instr.dst);
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
                    switch(func.type()) {
                        case PrimeVType::BytecodeFunction: {
                            BytecodeFunction const* bytecode = func.asBytecodeFunc();
                            stackFrames().precall(bytecode, argCount); // setup stack frame
                            continue;
                            break;
                        }
                        case PrimeVType::BridgeFunc: {
                            BridgeFunc bridgeFunc = func.asBridgeFunc();
                            stackFrames().precall(bridgeFunc, argCount);
                            int rst = bridgeFunc(this);
                            auto bridgeRet = stackFrames().retVal();
                            stackFrames().popFrame();
                            stackFrames().push(bridgeRet);
                            break;
                        }
                        default: {
                            assert(false);
                            break;;
                        }
                    }
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
                        obj.type() != PrimeVType::Object &&
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
                    switch(obj.type()) {
                        case PrimeVType::Object: {
                            auto val = obj[field.intValue()];
                            stackFrames().push(val);
                            break;
                        }
                        case PrimeVType::Userdata: {
                            UserdataObject* ud = obj.ud();
                            stackFrames().push(field); // 压入参数
                            UserdataCallFunction(this, obj, lib_keywords::___index, 1); // 调用 __index，结果会压栈
                            break;
                        }
                        default: {
                            assert(false);
                            break;
                        }
                    }
                    break;
                }
                case Opcode::GetField: {
                    if(instr.srcType == (uint32_t)ScopeType::Register) {
                        obj = stackFrames().topLocal(instr.src);
                        obj.deref();
                    } else if(instr.srcType == (uint32_t)ScopeType::Self) {
                        obj = stackFrames().selfRef();
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
                    if(obj.type() == PrimeVType::Object) {
                        stackFrames().push(obj[field.stringValue()]);
                    } else {
                        UserdataGetField(this, obj, field.stringValue());
                    }
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

    void Env::compileModule(char const* module, DebugInfoMap const* debugInfo) {
        auto modName = createName(module);
        auto mod = getModule(modName);
        mod->compileBytecode(this);
    }

    void Env::initializeModule(char const* module) {
        auto modName = createName(module);
        auto mod = getModule(modName);
        mod->initialize(this);
    }

    bool Env::checkIdentifiers(char const* module) {
        auto modName = createName(module);
        auto mod = getModule(modName);
        auto rst =  mod->checkIdentifiers(this);
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
        return "";
        // std::stringstream ss;
        // ss << "[backtrace] : " << errorType << std::endl;
        // for(auto it = _funcEnvs.rbegin(); it != _funcEnvs.rend(); ++it) {
        //     auto module = getModule(it->func->module());
        //     auto debugInfo = module->debugInfo().find(it->evaluingNode);
        //     ss << "  ";
        //     ss << "[" << it->func->module().text() << "] :"; 
        //     ss << "" << it->func->name().stringLiteral().text() << "()";
        //     ss << debugInfo->second.line << ":" << debugInfo->second.column << std::endl;
        // }
        // return ss.str();
    }
}