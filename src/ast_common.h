#pragma once
#include "name_pool.h"
#include "token.h"

namespace compiler {
    using Name = ksgw::Name;

    enum class IdentifierType {
        Null,
        FunctionLocal,
        Global,
        CurrentPackage,
        Member, // package/namespace/object oriented
    };

    class Token;
    // class Lexer;
    class SymbolLayout;
    class Value;
    class Object;
    class Env;
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
    }
}
