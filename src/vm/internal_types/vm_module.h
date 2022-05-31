#pragma once
#include <compiler_common.h>
#include "vm_object.h"
#include <vm/vm_bytecode.h>
// #include "../token.h"
#include <functional>

namespace compiler {

    using namespace ast;

    class DebugInfoNode {
    public:
        struct Info {
            uint32_t beg;
            uint32_t end;
            AstDebugInfo info;
        };
        Info _info;
    private:
        std::vector<Info*> _subInfos;
    public:
        DebugInfoNode* addSubInfo(AstDebugInfo info);
        ~DebugInfoNode() {
            for(auto info :_subInfos) {
                delete info;
            }
        }
    };

    /**
     * @brief 
     *   Module 的功能
     * 1. 用来初始化模块数据，有些数据初始化可能有依赖，所以需要手动确定初始化顺序 
     * 2. 用来热更新，程序在开发期间需要热更新模块内的变量，函数等，这时我们需要它快速定位
     */
    class Module {
        friend class Env;
    private:
        Value                                   _package;       // 模块所在包
        Node*                                   _ast;
        std::vector<std::pair<uint32_t,ast::Node const*>>           
                                                _initializeExprs;
        Bytecode                                _bytecode;
        BytecodeFunction*                       _initializeFunc;
        DebugInfoMap                            _debugInfos;
        std::vector<DebugInfo>                  _runtimeDebugInfo;

        struct IdLocateEnv {
            SymbolLayout*   functionLayout;     // local symbol layout
            SymbolLayout*   classLayout;        // class symbol layout  
            SymbolLayout*   packageLayout;      // current package symbol layout
            SymbolLayout*   globalLayout;       // global symbol layout
        };

    private:
        std::vector<Token> _checkVars(Env* env, Node const* ast, IdLocateEnv locateEnv);
        std::vector<Token> _checkFunctions(Env* env, Node const* ast, IdLocateEnv locateEnv);
        void _compileNode(ast::Node const* node, Bytecode* bytecode);
        bool _locateIdentifier(IdLocateEnv env, Identifier const* id);
        void _traverseAST(Node const* ast, TraverseCallBack& callBack) ;

        void setHostPackage(Value package);
        void addInitialize(uint32_t loc, Node* node);
        void setAst(Node* ast) {
            _ast = ast;
        }
        void setDebugInfo(DebugInfoMap && debugInfos) {
            _debugInfos = std::move(debugInfos);
        }
    public:
        using TraverseCallBack = std::function<void(Node const*)>;
        Module()
            : _package()
            , _ast(nullptr)
            , _initializeExprs()
        {}
        DebugInfoMap const& debugInfo() const {
            return _debugInfos;
        }
        std::vector<Token> checkIdentifiers(Env* env);

        // 编译成字节码
        void compileBytecode(Env* env);

        void initialize(Env* env);

        Bytecode const* bytecode() const { return &_bytecode; }
        
    };

}
