#pragma once
#include <vector>
#include "name_pool.h"

namespace compiler {
    class Token;
    class Lexer;

    enum class ASTNodeType {
        Identifier,
        Integer,
        Float,
        String,
        // Statements
        Block,
        Class,
        ExpressionStatement,
        Function,
        If,
        Print,
        Return,
        Super,
        This,
        Variable,
        While,
        // Expressions
        Array,
        Binary,
        Call,
        Get,
        Grouping,
        Logical,
        Set,
        Super,
        This,
        Unary,
        Variable,
        // Types
        Array,
        Function,
        Literal,
        // Misc
        Break,
        Continue,
        Null,
        True,
        False,
        Eof
    };

    class ASTNode {
    protected:
        ASTNode*        _parent;
        ASTNodeType     _type;
    public:
        ASTNode()
            : _parent(nullptr)
            , _type(ASTNodeType::Eof)
        {}
        void setParent(ASTNode* parent) {
            _parent = parent;
        }
        void setType( ASTNodeType type ) {
            _type = type;
        }
        virtual ~ASTNode() {}
    };

    class ASTLeaf : public ASTNode {
    protected:
        Token const* _token;
    public:
        ASTLeaf(Token const* token)
            : _token(token)
        {}
        Token const* token() const {
            return _token;
        }
    };

    class ASTBinaryExpression : public ASTNode {
    protected:
        ASTNode*        _left;
        ASTNode*        _right;
    public:
        ASTBinaryExpression(ASTNode* left = nullptr, ASTNode* right = nullptr)
            : _left(left)
            , _right(right)
        {
        }
        ASTNode const* left() const {
            return _left;
        }
        ASTNode const* right() const {
            return _right;
        }
    };

    class ASTNagativeExpression : public ASTNode {
    private:
        ASTNode* _value;
    public:
        ASTNagativeExpression(ASTNode* value)
            : _value(value) {
                _value->setParent(this);
        }
        ASTNode const* value() const {
            return _value;
        }
    };

    class ASTMultiExpression : public ASTNode {
    private:
        std::vector<ASTNode*> _expressions;
    public:
        void addSubNode(ASTNode* node) {
            _expressions.push_back(node);
            node->setParent(this);
        }
    };


}