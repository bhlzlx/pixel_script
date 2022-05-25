#pragma once
#include "internal_types/vm_object.h"
#include "internal_types/vm_module.h"
#include <name_pool.h>
#include <functional>
#include <map>
#include <type_traits>
#include "vm_bytecode.h"

namespace compiler {
    using Name = ksgw::Name;
    using NamePool = ksgw::NamePool;
    using namespace ast;

    using TraverseCallBack = std::function<void(Node const*)>;

    class Module;

    /**
     * @brief 
     * sizeof(Value) * 8 * 64 = 512
     */

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
            BytecodeFunction const* func;           // 当前函数 可能一般用不到
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
        StackFrames()
            : _frameInfos()
            , _values(2048)
        {
            FrameInfo fi = {
                0, 0, 0, 0, 0, Value(), Value(), nullptr, nullptr, nullptr
            };
            _frameInfos.push_back(fi);
            _frame = fi;
        } 
        void push(Value&& value, bool deref = false) {
            _values[_frame.sp] = std::move(value);
            if(deref) {
                _values[_frame.sp].deref();
            }
            ++_frame.sp;
        }
        void push(Value const& value, bool deref = false) {
            _values[_frame.sp] = value;
            if(deref) {
                _values[_frame.sp].deref();
            }
            ++_frame.sp;
        }
        void pop() { 
            if(_frame.sp > _frame.ap) {
                --_frame.sp;
                _values[_frame.sp].nilIt();
            }
        }
        void popFrame() {
            if(_frameInfos.size()) {
                _frame = _frameInfos.back();
                _frameInfos.pop_back();
            }
        }
        void pushFrame() {
            _frameInfos.push_back(_frame);
            auto const& last = _frameInfos.back();
            _frame.sp = _frame.ap = _frame.fp = last.sp;
        }
        // 强制设置栈顶位置
        void precall(BytecodeFunction const* func) {
            _frame.ap = _frame.sp;
            _frame.ip = func->ip();
            _frame.package = func->package();
            _frame.instr = func->instruction();
            _frame.constants = func->module()->constants();
            _frame.func = func;
            _frame.lp = _frame.sp += func->localSize();
        }
        size_t argCount() const {
            return _frame.ap - _frame.fp;
        }
        void reserveValues(size_t count) {
            assert(count + _frame.sp <= _values.size());
            _frame.sp += count;
        }
        void popN(size_t n) {
            while(n && _frame.sp > _frame.ap) {
                --_frame.sp;
                _values[_frame.sp].nilIt();
                --n;
            }
        }
        Value local(uint32_t index) {
            assert(index <= _frame.lp - _frame.fp);
            return Value(&_values[_frame.fp + index]); // return a reference
        }
        // 用于运算，有写回的必要
        Value& topLocalRef(uint32_t index) {
            assert(index < _frame.sp - _frame.ap);
            return _values[_frame.sp - index - 1];
            // Value* val = &_values[_frame.sp - index - 1];
            // Value rst;
            // if(val->type() == PrimeVType::ValueRef) {
            //     return *val;
            // } else {
            //     rst = val;
            // }
            // return rst;
        }
        Value topLocal(uint32_t index) {
            assert(index < _frame.sp - _frame.ap);
            Value val = _values[_frame.sp - index - 1];
            if(val.type() == PrimeVType::ValueRef) {
                val.deref();
            }
            return val;
        }
        Value package() {
            return _frame.package;
        }
        Value self() {
            return _frame.self;
        }
        Value const* constants() const {
            return _frame.constants;
        }
        Instruction const* instr() {
            return _frame.instr + _frame.ip;
        }
        void peekIP() {
            ++_frame.ip;
        }
    };

    class Env {
        friend class Module;
    private:
        struct FuncEnv {
            Function*           func;   // function ast node
            Value               package; // current package
            Value               self;
            Node const*         evaluingNode;
            bool                retNow;
        };
    private:
        NamePool                                _namePool;
        Value                                   _package;
        StackFrames                             _stackFrames;
        std::vector<FuncEnv>                    _funcEnvs;
        std::vector<SymbolLayout*>              _symbolLayouts;
        std::map<Name, Module*, Name::FastLess> _modules;


    private:
        // utility functions
        FuncEnv const* funcEnv() { return &_funcEnvs.back(); }
        Value preparePackage(Node* ast);
        struct IdLocateEnv {
            SymbolLayout*   functionLayout;     // local symbol layout
            SymbolLayout*   packageLayout;      // current package symbol layout
            SymbolLayout*   classLayout;        // class symbol layout  
        };
        bool locateIdentifier(IdLocateEnv env, Identifier const* id);
        void traverseAST(Node const* ast, TraverseCallBack& callBack);
        void updateEvaluingNode(Node const* ast);
        // std::vector<Token> postprocessFunction(Function* ast, IdLocateEnv env);
    public:

        Env();

        ~Env() {
        }

        Value root() { return _package; }

        StackFrames& stackFrames() {
            return _stackFrames;
        }

        SymbolLayout* newSymbolLayout(SymbolLayoutType type) {
            auto symLayout = new SymbolLayout(type);
            _symbolLayouts.push_back(symLayout);
            return symLayout;
        }

        Name createName(char const* str);

        std::string backtrace(char const* errorType) const ;

        Module* getModule(Name const& name);
        Module const* getModule(Name const& name) const;
        /**
         * @brief 
         *     只是生成简单的符号表，类，包，函数，全局变量
         * @return true 
         * @return false 
         */
        bool compileCodeChunk(char const* module, Node* ast, DebugInfoMap* debugInfoMap = nullptr);
        bool postprocessModule(char const* module);
        void compilerBytecode();
        void initializeModule(char const* module);
        int callBytecodeFunc();

        /**
         * @brief only for test
         * 
         * @param func 
         * @return Value 
         */
        Value callFuncWithPath(std::string func);

        void execute();

        Value _valueInScope( ScopeType scope, uint32_t loc);
        // void exeInstr(Instruction const* instr);

        // /**
        //  * @brief 计算一个节点的值
        // **/
        // Value eval(Node const* ast);

        // /**
        //  * @brief 
        //  *   二元表达式有点特殊，它是少数直接跟值打交道的，所以单独拿出来实现了
        //  * @param op 
        //  * @return Value 
        //  */
        // Value evalBinaryOp(Token op, Value a, Value b);

        // /**
        //  * @brief 
        //  *   计算一个标识符的值，是某个已经存在的于变量表里的变量引用
        //  * @param id 
        //  * @return Value 
        //  */
        // Value evalIdentifier(Identifier const* id);

    };
}