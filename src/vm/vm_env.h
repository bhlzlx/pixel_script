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
        size_t                  _argBeg;
    private:
        Value* currentFrame() const {
            return const_cast<Value*>(&_params[_frameBases.back()]);
        }
    public:
        StackFrames() {}
        void pushValue(Value const& value) {
            if(value.type() == PrimeVType::ValueRef) {
                Value v = value;
                v.deref();
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
            _argBeg = _params.size();
        }
        void pushArgEnd() {
            _frameBases.push_back(_argBeg);
            _argBeg = ~0;
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
            // Value*              vt;     // 废弃了
            Function*           func;   // function ast node
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
        std::vector<Token> postprocessFunction(Function* ast, IdLocateEnv env);
    public:

        Env();

        ~Env() {
            _stackFrames.popToArgBegin();
        }

        Value root() { return _package; }

        StackFrames& stackValues() {
            return _stackFrames;
        }

        SymbolLayout* newSymbolLayout(SymbolLayoutType type) {
            auto symLayout = new SymbolLayout(type);
            _symbolLayouts.push_back(symLayout);
            return symLayout;
        }

        Name createName(char const* str);

        Module* getModule(Name const& name);

        bool compileCodeChunk(char const* module, Node* ast);
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