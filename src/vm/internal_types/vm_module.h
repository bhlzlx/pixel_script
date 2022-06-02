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
        void setInfo(CodeDebugInfo inf);
        bool find(uint32_t ip, std::pair<int,int>& out);
        ~DebugInfoNode();
    };

    class DebugInfo {
    private:
        Bytecode*                       _bytecode;
        std::vector<DebugInfoNode*>     _nodes;
        std::vector<DebugInfoNode*>     _buildStack;
    public:
        class Handle {
        private:
            DebugInfo    * _info;
        public:
            Handle(DebugInfo* info)
                : _info(info) {
            }
            void release() {
                if(_info) {
                    _info->_buildStack.back()->setEnd(_info->_bytecode->size());
                    _info->_buildStack.pop_back();
                }
            }
        };
        DebugInfo(Bytecode* bytecode)
            : _bytecode(bytecode)
            , _nodes()
            , _buildStack()
        {}
        Handle newDbgInfo(CodeDebugInfo info);
        std::pair<int,int> locateIp(uint32_t ip);
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
        Value                                               _package;       // 模块所在包
        Bytecode                                            _bytecode;
        BytecodeFunction*                                   _initializeFunc;
        Name                                                _name;
        DebugInfo                                           _debugInfo;
        // ast infos，数据成员是可以删除的，不过这里暂时不作优化了，暂时先做清空处理
        Node*                                               _ast;
        std::vector<std::pair<uint32_t,ast::Node const*>>   _initializeExprs;
        std::vector<CodeDebugInfo>                          _astDebugInfos;

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
        void setAst(Node* ast) { _ast = ast; }
        void setCodeDbgInfo(std::vector<CodeDebugInfo>&& dbgInfo);
    public:
        using TraverseCallBack = std::function<void(Node const*)>;
        Module(Name name)
            : _package()
            , _ast(nullptr)
            , _initializeExprs()
            , _debugInfo(&_bytecode)
            , _name(name)
        {}
        std::vector<Token> checkIdentifiers(Env* env);

        // 编译成字节码
        void compileBytecode(Env* env);

        void initialize(Env* env);

        Bytecode const* bytecode() const { return &_bytecode; }

        std::pair<int, int> getIpDbgLoc(uint32_t ip);

        Name name() const {
            return _name;
        }
        
    };

}
