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
        class ASTLeaf;
        class ASTIdentifier;
        class ASTStringList;
        class ASTBinaryOpExpr;
        class ASTIfStatement;
        class ASTWhileStatement;
        class ASTFunction;
        class ASTMultiExpr;
        class ASTPrimary;
        class ASTNegativeExpression;
        class ASTVariable;
    }
}
