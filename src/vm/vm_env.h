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


    class Module;

    /**
     * @brief 
     * sizeof(Value) * 8 * 64 = 512
     */

    class Env {
        friend class Module;
    private:
    private:
        NamePool                                _namePool;
        Value                                   _package;
        StackFrames                             _stackFrames;
        std::vector<SymbolLayout*>              _symbolLayouts;
        std::map<Name, Module*, Name::FastLess> _modules;
    private:
        // utility functions
        Value preparePackage(Node* ast);
        void _executeBinaryOp(Opcode op);
        Value _valueInScope( ScopeType scope, uint32_t loc, bool readonly = false);
        // preprocess ast
        bool preprocessModuleAST(char const* module, Node* ast);
    public:

        Env();

        ~Env() {
        }

        Value root() { return _package; }

        StackFrames& stackFrames() {
            return _stackFrames;
        }
        StackFrames const& stackFrames() const{
            return _stackFrames;
        }

        SymbolLayout* newSymbolLayout(SymbolLayoutType type) {
            auto symLayout = new SymbolLayout(type);
            _symbolLayouts.push_back(symLayout);
            return symLayout;
        }

        Name createName(char const* str);

        std::string backtrace(char const* baseError) const ;

        Module* getModule(Name const& name);
        Module const* getModule(Name const& name) const;
        /**
         * @brief 
         *     只是生成简单的符号表，类，包，函数，全局变量
         * @return true 
         * @return false 
         */

        /**
         * @brief 
         *     代码预编译，生成基本符号，语法树，但是不解析语义正确性
         * @param moduleName 
         * @param code 
         * @return std::vector<Token> 代码预编译错误位置信息
         */
        bool precompileModule(char const* moduleName, char const* code);
        /**
         * @brief 
         *     解析，将语法树编译成字节码
         * @param module 
         * @return true 
         * @return false 
         */
        bool compileModule(char const* module);
        bool checkIdentifiers(char const* module);
        void initializeModule(char const* module);
        /**
         * @brief only for test
         * 
         * @param func 
         * @return Value 
         */

        Value callFuncWithPath(std::string const& func);
        
        /**
         * @brief 
         *   bridge utility function
         * 
         * @param argc 
         * @return int 
         */
        int call(int argc);

        // vm execution functions
        void execute();


    };
}