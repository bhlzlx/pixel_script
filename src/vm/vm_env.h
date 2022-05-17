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

    class StackValues {
    private:
        std::vector<Value>      _params;
        std::vector<size_t>     _frameBases; // 
    private:
        Value* currentFrame() const {
            return const_cast<Value*>(&_params[_frameBases.back()]);
        }
    public:
        StackValues() {}
        void pushValue(Value const& value) {
            _params.push_back(value);
        }
        void pushValue(Value&& value) {
            _params.emplace_back(std::move(value));
        }
        void prepareNextFrame() {
            _frameBases.push_back(_params.size());
        }
        void popFrame() {
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
        Value localValueRef(size_t index) const {
            size_t frameSize = topFrameSize();
            if(index >= frameSize) {
                return Value();
            } else {
                return Value(currentFrame() + index);
            }
        }
        Value popValue() {
            Value value(std::move(_params.back()));
            _params.pop_back();
            return value;
        }
        // Value topValueRef() const {
        //     return Value(const_cast<Value*>(&_params.back()));
        // }
        // Value topValue() const {
        //     return _params.back();
        // }
    };

    class Env {
        friend class Module;
    private:
        struct FuncEnv {
            // Value*              vt;     // 废弃了
            Function*           func;   // function ast node
        };
    private:
        NamePool                                _namePool;
        Value                                   _package;
        StackValues                             _stackValues;
        std::vector<FuncEnv>                    _funcEnvs;
        std::vector<SymbolLayout*>              _symbolLayouts;

        std::map<Name, Module*, Name::FastLess> _modules;
    private:
    private:
        // utility functions
        Value rootPackage() { return _package; }
        FuncEnv const* funcEnv() { return &_funcEnvs.back(); }
        Value preparePackage(Node* ast);
        struct IdLocateEnv {
            SymbolLayout*   functionLayout;     // local symbol layout
            SymbolLayout*   packageLayout;      // current package symbol layout
            SymbolLayout*   classLayout;        // class symbol layout  
        };
        bool locateIdentifier(IdLocateEnv env, Identifier const* id) ;
        void traverseAST(Node const* ast, TraverseCallBack& callBack);
        std::vector<Token> postprocessFunction(Function* ast, IdLocateEnv env);
    public:

        Env() {
            auto layout = newSymbolLayout(SymbolLayoutType::Package);
            _package = Value(layout);
            compiler::keywords::init(this);
            _stackValues.prepareNextFrame();
            _stackValues.pushValue(_package);
        }

        ~Env() {
            _stackValues.popFrame();
        }

        StackValues& stackValues() {
            return _stackValues;
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