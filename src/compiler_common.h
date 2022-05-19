#pragma once
#include "name_pool.h"
#include "token.h"
#include <vector>
#include <map>

namespace compiler {

    using Name = ksgw::Name;

    class Token;
    class SymbolLayout;
    class Value;
    class Object;
    class UserdataObject;
    class Env;
    class Module;

    // debug 时候给调试器提供执行的位置信息
    // 另外，并不是所有的节点都有这个信息，只有那些算值的节点也有，即expr及更小的节点会有，
    // 像 statement 因为本向不参与执行，所以没有
    struct ExprDebugInfo {
        Name    name;
        int     line;
        int     column;
        // 为什么没有文件信息？函数里可以取到！
    };


    using BridgeFunc = int(*)(Env* env);

    struct BridgeFuncPair {
        BridgeFunc func;
        char const* name;
    };

    namespace ast {
        class Node;
        class Leaf;
        class Identifier;
        class StringList;
        class BinaryOpExpr;
        class IfStmt;
        class WhileStmt;
        class Function;
        class MultiExpr;
        class NegativeExpr;
        class Variable;
        class FunctionCall;
        class DotAccess;
        class Class;
        class NewOperator;
        class ReturnStmt;
    }

    using DebugInfoMap = std::map<ast::Node const*, ExprDebugInfo>;

    enum class IdentifierType : uint8_t {
        Null,
        FunctionLocal,
        Global,
        CurrentPackage,
        ClassMember, // package/namespace/object oriented
    };

    enum class SymbolType : uint8_t {
        Variable,
        Package,
        Function,
        Class,
        NewOperator,
    };

    enum class SymbolLayoutType : uint8_t {
        None,
        Package,
        Class,
        // ClassInstance,
        Function,
    };

}
