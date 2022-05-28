#pragma once
#include "internal_types/vm_object.h"
#include "internal_types/vm_module.h"
#include <name_pool.h>
#include <functional>
#include <map>
#include <type_traits>
#include "vm_bytecode.h"
#include "vm_register.h"

namespace compiler {

    using namespace ast;

    using TraverseCallBack = std::function<void(Node const*)>;

    class Module;

    /**
     * @brief 
     * sizeof(Value) * 8 * 64 = 512
     */

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
        void initializeModule(char const* module);
        int callBytecodeFunc();

        /**
         * @brief only for test
         * 
         * @param func 
         * @return Value 
         */
        int call(int argc);
        Value callFuncWithPath(std::string const& func);

        void _executeBinaryOp(Opcode op);
        void execute();

        Value _valueInScope( ScopeType scope, uint32_t loc, bool readonly = false);
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