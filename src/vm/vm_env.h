#pragma once
#include "vm_object.h"
#include "vm_module.h"
#include "../name_pool.h"
#include <functional>
#include <map>
#include <type_traits>

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
    private:
        std::vector<Value>      _params;
        std::vector<size_t>     _frameBases; // 
        std::vector<size_t>     _argBegs;
    private:
        Value* currentFrame() const {
            return const_cast<Value*>(&_params[_frameBases.back()]);
        }
        StackFrames(StackFrames const&) = delete;
        StackFrames(StackFrames &&) = delete;
        StackFrames& operator=(StackFrames const&) = delete;
        StackFrames& operator=(StackFrames &&) = delete;
    public:
        StackFrames() {
            _params.reserve(512);
        }
        // 给栈中的临时变量压值的话（赋值，初始化），不要存ref，因为后续处理会很麻烦，而且很没必要
        // 但是如果是存放返回值，则有可能是引用，所以这里要分两种情况！
        void pushValue(Value const& value, bool returnValue = false) {
            if(value.type() == PrimeVType::ValueRef) {
                Value v = value;
                if(!returnValue) { // 返回值不要强制解除引用
                    v.deref();
                }
                _params.push_back(v);
            } else {
                _params.push_back(value);
            }
        }
        void pushValue(Value&& value) {
            if(value.type() == PrimeVType::ValueRef) {
                value.deref();
            }
            _params.emplace_back(std::move(value));
        }
        void pushArgBegin() {
            _argBegs.push_back(_params.size());
        }
        void pushArgEnd() {
            _frameBases.push_back(_argBegs.back());
            _argBegs.pop_back();
        }
        void popToArgBegin() {
            while(_params.size() > _frameBases.back()) {
                _params.pop_back();
            }
            _frameBases.pop_back();
        }
        size_t topFrameSize() const {
            if(_params.size() == 0) {
                return 0;
            } else {
                return _params.size() - _frameBases.back();
            }
        }
        // 在vm里更新栈内值的时候，使用localValueRef
        Value localValueRef(size_t index) const {
            size_t frameSize = topFrameSize();
            if(index >= frameSize) {
                return Value();
            } else {
                return Value(currentFrame() + index);
            }
        }
        // 获取栈上的参数的时候，使用localValue
        Value localValue(size_t index) const {
            size_t frameSize = topFrameSize();
            if(index >= frameSize) {
                return Value();
            } else {
                return Value(currentFrame()[index]);
            }
        }
        Value popValue() {
            if(_params.size() > _frameBases.back()) {
                Value rst(std::move(_params.back()));
                _params.pop_back();
                return rst;
            } else {
                return Value();
            }
        }
    };

    class Env {
        friend class Module;
    private:
        struct FuncEnv {
            Function*           func;   // function ast node
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
        std::vector<Token> postprocessFunction(Function* ast, IdLocateEnv env);
    public:

        Env();

        ~Env() {
            _stackFrames.popToArgBegin();
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
        void initializeModule(char const* module);
        int callFunction(Value const& func);

        /**
         * @brief only for test
         * 
         * @param func 
         * @return Value 
         */
        Value callFuncWithPath(std::string func);

        /**
         * @brief 计算一个节点的值
        **/
        Value eval(Node const* ast);

        /**
         * @brief 
         *   二元表达式有点特殊，它是少数直接跟值打交道的，所以单独拿出来实现了
         * @param op 
         * @return Value 
         */
        Value evalBinaryOp(Token op, Value a, Value b);

        /**
         * @brief 
         *   计算一个标识符的值，是某个已经存在的于变量表里的变量引用
         * @param id 
         * @return Value 
         */
        Value evalIdentifier(Identifier const* id);

    };
}