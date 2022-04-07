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
        If,
        Return,
        While,
        // Expressions
        BinaryOp,
        NagtiveOp,
        Program,
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
        ASTNodeType     _type;
        ASTNode*        _parent;
    public:
        ASTNode(ASTNodeType type = ASTNodeType::Eof, ASTNode* parent = nullptr)
            : _type(type)
            , _parent(parent)
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
        Token _token;
    public:
        ASTLeaf(ASTNodeType type, Token token) : ASTNode(type), _token(token) {}
        Token token() const {
            return _token;
        }
    };

    class ASTBinaryOpExpr : public ASTNode {
    protected:
        TokenType       _op;
        ASTNode*        _left;
        ASTNode*        _right;
    public:
        ASTBinaryOpExpr(ASTNode* left = nullptr, ASTNode* right = nullptr, TokenType op = TokenType::None)
            : ASTNode(ASTNodeType::BinaryOp)
            , _op(op)
            , _left(left)
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
        ASTNagativeExpression(ASTNode* value) : ASTNode(ASTNodeType::NagtiveOp)
            , _value(value)
        {
            _value->setParent(this);
        }
        ASTNode const* value() const {
            return _value;
        }
    };

    class ASTIfStatement : public ASTNode {
    private:
        ASTNode* _condition;
        ASTNode* _thenBranch;
        ASTNode* _elseBranch;
    public:
        ASTIfStatement() : ASTNode(ASTNodeType::If) 
            , _condition(nullptr)
            , _thenBranch(nullptr)
            , _elseBranch(nullptr)
        {}
        void setCondition(ASTNode* condition) {
            _condition = condition;
            _condition->setParent(this);
        }
        void setThenBranch(ASTNode* thenBranch) {
            _thenBranch = thenBranch;
            _thenBranch->setParent(this);
        }
        void setElseBranch(ASTNode* elseBranch) {
            _elseBranch = elseBranch;
            _elseBranch->setParent(this);
        }
    };

    class ASTWhileStatement : public ASTNode {
    private:
        ASTNode* _condition;
        ASTNode* _body;
    public:
        ASTWhileStatement() : ASTNode(ASTNodeType::While)
            , _condition(nullptr)
            , _body(nullptr)
        {}
        void setCondition(ASTNode* condition) {
            _condition = condition;
            _condition->setParent(this);
        }
        void setBody(ASTNode* body) {
            _body = body;
            _body->setParent(this);
        }
    };

    class ASTBlock : public ASTNode {
    private:
        std::vector<ASTNode*> _expressions;
    public:
        ASTBlock() : ASTNode(ASTNodeType::Block) {}
        void addSubNode(ASTNode* node) {
            _expressions.push_back(node);
            node->setParent(this);
        }
    };

    class ASTProgram : public ASTNode {
    private:
        std::vector<ASTNode*> _expressions;
    public:
        ASTProgram()
            : ASTNode(ASTNodeType::Program)
            , _expressions()
        {}

        void addExpr(ASTNode* expr) {
            _expressions.push_back(expr);
            expr->setParent(this);
        }
    };

}