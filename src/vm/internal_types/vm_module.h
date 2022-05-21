#pragma once
#include <compiler_common.h>
#include "vm_object.h"
// #include "../token.h"
#include <functional>

namespace compiler {

    using namespace ast;

    /**
     * @brief 
     *   Module 的功能
     * 1. 用来初始化模块数据，有些数据初始化可能有依赖，所以需要手动确定初始化顺序 
     * 2. 用来热更新，程序在开发期间需要热更新模块内的变量，函数等，这时我们需要它快速定位
     */

    class Module {
    private:
        Value                                   _package;       // 模块所在包
        std::vector<std::pair<uint32_t, Node*>> _initliazeList; // 初始化列表
        std::vector<Node*>                      _functions;
        std::vector<Value>                      _classes;
        DebugInfoMap                            _debugInfos;

        struct IdLocateEnv {
            SymbolLayout*   functionLayout;     // local symbol layout
            SymbolLayout*   classLayout;        // class symbol layout  
            SymbolLayout*   packageLayout;      // current package symbol layout
            SymbolLayout*   globalLayout;       // global symbol layout
        };
    public:
        using TraverseCallBack = std::function<void(Node const*)>;
        Module()
            : _package()
            , _initliazeList()
        {}

        void setDebugInfo(DebugInfoMap && debugInfos) {
            _debugInfos = std::move(debugInfos);
        }
        DebugInfoMap const& debugInfo() const {
            return _debugInfos;
        }
        void setHostPackage(Value package);
        void addInitliaze(uint32_t loc, Node* node);
        void addFunction(Node* node);
        void addClass(Value cls);

        void traverseAST(Node const* ast, TraverseCallBack& callBack) ;

        std::vector<Token> postprocess(Env* env);
        std::vector<Token> postprocessFunction(Env* env, Node* ast, IdLocateEnv locateEnv);
        bool locateIdentifier(IdLocateEnv env, Identifier const* id);
        void initialize(Env* env);
        
    };

}
