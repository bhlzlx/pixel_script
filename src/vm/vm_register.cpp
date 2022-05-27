#include "vm_register.h"
#include "vm_bytecode.h"
#include "internal_types/vm_module.h"

namespace compiler {

    StackFrames::StackFrames()
        : _frameInfos()
        , _values(2048)
    {
        FrameInfo fi = {
            0, 0, 0, 0, 0, Value(), Value(), nullptr, nullptr, nullptr
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
            _frame = _frameInfos.back();
            _frameInfos.pop_back();
        }
    }
    void StackFrames::pushFrame(uint32_t ipOffset) {
        // _frame.ip += ipOffset;
        _frameInfos.push_back(_frame);
        _frameInfos.back().ip += ipOffset;
        auto const& last = _frameInfos.back();
        _frame.sp = _frame.ap = _frame.fp = last.sp;
    }
    // 强制设置栈顶位置
    void StackFrames::precall(BytecodeFunction const* func) {
        // self拿来存一下
        auto self = _values[_frame.sp-1];
        self.deref();
        pop();
        //
        _frameInfos.push_back(_frame); // 返回后的栈情况需要再调整
        // 调整
        _frameInfos.back().sp -= func->argCount();  // 函数调用压参前蝗栈顶位置
        // 设置当前帧的状态
        _frame.self = self;
        _frame.fp = _frame.sp - func->argCount();   // 
        _frame.ap = _frame.sp;                      // 参数位置调整
        _frame.lp = _frame.sp += func->localSize(); // 局部变量空间调整
        _frame.ip = func->ip();
        _frame.package = func->package();
        _frame.instr = func->instruction();
        _frame.constants = func->module()->constants();
        _frame.func = func;
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
    Value StackFrames::self() {
        return &_frame.self;
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

}