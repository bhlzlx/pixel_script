#pragma once
#include "name_pool.h"
#include "token.h"
#include <vector>
#include <map>

namespace compiler {

    using Name = ksgw::Name;

    class Token;
    // class Lexer;
    class SymbolLayout;
    class Value;
    class Object;
    class UserdataObject;
    class Env;
    class Module;
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
    }

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
