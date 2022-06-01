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
            uint32_t        beg;
            uint32_t        end;
            CodeDebugInfo   info;
        };
    private:
        Info                            _info;
        std::vector<DebugInfoNode*>     _subInfos;
    public:
        DebugInfoNode* addSubInfo(CodeDebugInfo info);
        void setRange(uint32_t beg, uint32_t end);
        void setBegin(uint32_t beg);
        void setEnd(uint32_t end);
        ~DebugInfoNode();
    };

    class DebugInfo {
    private:
        Bytecode*       _bytecode;
        DebugInfoNode*  _root;
        std::vector<DebugInfoNode*>     _nodes;
    public:
        struct Handle {
            DebugInfo    *info;
            ~Handle() {
                info->_nodes.back()->setEnd(info->_bytecode->size());
                info->_nodes.pop_back();
            }
        };
        DebugInfo(Bytecode* bytecode)
            : _bytecode(bytecode)
        {}
        Handle newDbgInfo(CodeDebugInfo info);
        ~DebugInfo();
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
        //
        std::vector<CodeDebugInfo>              _debugInfos;
        DebugInfo                               _debugInfo;

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
    public:
        using TraverseCallBack = std::function<void(Node const*)>;
        Module()
            : _package()
            , _ast(nullptr)
            , _initializeExprs()
        {}
        std::vector<Token> checkIdentifiers(Env* env);

        // 编译成字节码
        void compileBytecode(Env* env);

        void initialize(Env* env);

        Bytecode const* bytecode() const { return &_bytecode; }
        
    };

}
