#pragma once
#include "internal_types/vm_types.h"

namespace compiler {

    class StackFrames {
        struct FrameInfo {
            size_t                  fp;             // 函数栈底位
            size_t                  ap;             // 参数栈顶位
            size_t                  lp;             // 局部变量栈顶位
            size_t                  sp;             // 当前栈顶位  
            size_t                  ip;             // 当前指令位
            Value                   package;        // 当前包
            Value                   self;           // self对象
            Instruction const*      instr;          // 当前指令块起始地址
            Value const*            constants;      // 静态常量区
            union {
                BytecodeFunction const* func;           // 当前函数 可能一般用不到
                BridgeFunc bridgeFunc;
            };
        };
        // delete default assign constructor
        StackFrames(StackFrames const&) = delete;
        StackFrames(StackFrames &&) = delete;
        StackFrames& operator=(StackFrames const&) = delete;
        StackFrames& operator=(StackFrames &&) = delete;
    private:
        FrameInfo                   _frame;
        std::vector<FrameInfo>      _frameInfos;
        std::vector<Value>          _values;
    public:
        StackFrames();
        void push(Value&& value, bool deref = false);
        void push(Value const& value, bool deref = false);
        void pop();
        void popFrame();
        void pushFrame();
        /**
         * @brief 
         *   保证先压入参数，后压入self（如果有必要）
         * @param func 
         */
        void precall(BytecodeFunction const* func, int argc);
        void precall(BridgeFunc func, int argc);
        // void precall(int argc);
        size_t argCount() const;
        void reserveValues(size_t count);
        void popN(size_t n);
        // 
        Value argAt(uint32_t index);
        Value local(uint32_t index);
        // 用于运算，有写回的必要
        Value& topLocalRef(uint32_t index);
        Value topLocal(uint32_t index);
        Value retVal() const;
        Value package();
        Value selfRef();
        Value self();
        Value const* constants() const;
        Instruction const* instr();
        void jump(size_t pos);
        void peekIP();
    };


}