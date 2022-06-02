#include "vm_register.h"
#include "vm_bytecode.h"
#include "internal_types/vm_module.h"
#include <sstream>

namespace compiler {

    StackFrames::StackFrames()
        : _frameInfos()
        , _values(2048)
    {
        FrameInfo fi = {
            0, 0, 0, 0, 0, Value(), Value(), nullptr, nullptr, { nullptr }
        };
        _frameInfos.push_back(fi);
        _frame = fi;
    } 
    void StackFrames::push(Value&& value, bool deref) {
        _values[_frame.sp] = std::move(value);
        if(deref) {
            _values[_frame.sp].deref();
        }
        ++_frame.sp;
    }
    // 压参数的时候一定要deref，但是判断不了。。。
    void StackFrames::push(Value const& value, bool deref ) {
        _values[_frame.sp] = value;
        if(deref) {
            _values[_frame.sp].deref();
        }
        ++_frame.sp;
    }
    void StackFrames::pop() { 
        if(_frame.sp > _frame.ap) {
            --_frame.sp;
            _values[_frame.sp].nilIt();
        }
    }
    void StackFrames::popFrame() {
        if(_frameInfos.size()) {
            auto prev_sp = _frameInfos.back().sp;
            for(size_t i = prev_sp; i < _frame.sp; ++i) {
                _values[i].nilIt();
            }
            _frame = _frameInfos.back();
            _frameInfos.pop_back();
        }
    }
    void StackFrames::pushFrame(){
        _frameInfos.push_back(_frame);
        auto const& last = _frameInfos.back();
        _frame.sp = _frame.ap = _frame.fp = last.sp;
    }

    // 预处理一下栈状态
    void StackFrames::precall(BytecodeFunction const* func, int argc ) {
        // self拿来存一下
        auto self = _values[_frame.sp-1];
        self.deref();
        pop();
        // 传参里不可能有值引用类型
        for(int i = 0; i < argc; ++i) {
            _values[_frame.sp-1-i].deref();
            assert(_values[_frame.sp-1-i].type() != PrimeVType::ValueRef);
        }
        _frameInfos.push_back(_frame); // 返回后的栈情况需要再调整
        // 调整(函数执行完，回去的时候，sp的原位置)
        _frameInfos.back().sp -= argc;  // 函数调用压参前蝗栈顶位置，参数数量可能比函数定的多，多的直接扔了它
        // 设置当前帧的状态
        _frame.self = self;
        _frame.fp = _frameInfos.back().sp;   // 
        _frame.ap = _frame.fp + func->argCount();  // 扔了多余的
        _frame.lp = _frame.sp = _frame.ap + func->localSize(); // 局部变量空间调整
        _frame.ip = func->ip();
        _frame.package = func->package();
        _frame.instr = func->instruction();
        _frame.constants = func->module()->bytecode()->constants();
        _frame.func = func;
    }

    void StackFrames::precall(BridgeFunc func, int argc) {
        auto self = _values[_frame.sp-1];
        self.deref();
        pop();
        // 传参里不可能有值引用类型
        for(int i = 0; i < argc; ++i) {
            _values[_frame.sp-1-i].deref();
            assert(_values[_frame.sp-1-i].type() != PrimeVType::ValueRef);
        }
        _frameInfos.push_back(_frame);
        _frameInfos.back().sp -= argc;
        _frame.self = self;
        _frame.fp = _frameInfos.back().sp;   // 
        _frame.lp = _frame.ap = _frame.fp + argc;  // 扔了多余的
        _frame.ip = 0; // bridge func 没有ip
        _frame.package = Value(); // bridge func没有包
        _frame.instr = nullptr;
        _frame.constants = nullptr;
        _frame.bridgeFunc = func;
    }

    size_t StackFrames::argCount() const {
        return _frame.ap - _frame.fp;
    }
    void StackFrames::reserveValues(size_t count) {
        assert(count + _frame.sp <= _values.size());
        _frame.sp += count;
    }
    void StackFrames::popN(size_t n) {
        while(n && _frame.sp > _frame.ap) {
            --_frame.sp;
            _values[_frame.sp].nilIt();
            --n;
        }
    }
    Value StackFrames::argAt(uint32_t index) {
        assert(index<_frame.ap - _frame.fp);
        return _values[_frame.fp + index];
    }
    Value StackFrames::local(uint32_t index) {
        assert(index <= _frame.lp - _frame.fp);
        return Value(&_values[_frame.fp + index]); // return a reference
    }
    // 用于运算，有写回的必要
    Value& StackFrames::topLocalRef(uint32_t index) {
        assert(index < _frame.sp - _frame.ap);
        return _values[_frame.sp - index - 1];
    }
    Value StackFrames::topLocal(uint32_t index) {
        assert(index < _frame.sp - _frame.ap);
        Value val = _values[_frame.sp - index - 1];
        if(val.type() == PrimeVType::ValueRef) {
            val.deref();
        }
        return val;
    }
    Value StackFrames::retVal() const {
        if(_frame.sp > _frame.lp) {
            Value val = _values[_frame.sp - 1];
            if(val.type() == PrimeVType::ValueRef) {
                val.deref();
            }
            return val;
        } else {
            return Value();
        }
    }
    Value StackFrames::package() {
        return _frame.package;
    }
    Value StackFrames::selfRef() { // Value Ref
        return &_frame.self;
    }
    Value StackFrames::self() {
        assert(_frame.self.type() != PrimeVType::ValueRef);
        return _frame.self;
    }
    Value const* StackFrames::constants() const {
        return _frame.constants;
    }
    Instruction const* StackFrames::instr() {
        return _frame.instr + _frame.ip;
    }
    void StackFrames::jump(size_t pos) {
        _frame.ip = pos;
    }
    void StackFrames::peekIP() {
        ++_frame.ip;
    }

    std::string StackFrames::backtrace(char const* baseError) const {
        std::stringstream ss;
        ss << "[backtrace] : " << baseError << std::endl;

        auto backtraceFrame = [&ss](FrameInfo const& frame) {
            ss << "  ";
            if(frame.instr) { // bytecode
                auto module = frame.func->module()->name().text();
                auto loc = frame.func->module()->getIpDbgLoc(frame.ip);
                auto funcName = frame.func->name().text();
                ss << "[" << module << "] :"; 
                ss << "" << funcName << "()";
                ss << loc.first << ":" << loc.second << std::endl;
            } else {
                ss << "[bridge func]" << std::endl;
            }
        };
        backtraceFrame(_frame);
        for(auto iter = _frameInfos.rbegin(); iter != _frameInfos.rend(); ++iter) {
            backtraceFrame(*iter);
        }
        return ss.str();
    }
}