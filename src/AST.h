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
        Primary,
        Keyword,
        // Statements
        Block,
        Program,
        Param,
        Params,
        Args,
        Class,
        If,
        Return,
        While,
        // Expressions
        BinaryOp,
        NagtiveOp,
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

    class ASTPrimary : public ASTNode {
    private:
        ASTNode* _operand;
        ASTNode* _postfix;
    public:
        ASTPrimary(ASTNode* operand, ASTNode* postfix)
            : ASTNode(ASTNodeType::Primary)
            , _operand(operand)
            , _postfix(postfix)
        {}
        ASTNode* operand() const {
            return _operand;
        } 
        ASTNode* postfix() const {
            return _postfix;
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
        ~ASTBinaryOpExpr() {
            if(_left) {
                delete _left;
            }
            if(_right) {
                delete _right;
            }
        }
    };

    class ASTNegativeExpression : public ASTNode {
    private:
        ASTNode* _value;
    public:
        ASTNegativeExpression(ASTNode* value) : ASTNode(ASTNodeType::NagtiveOp)
            , _value(value)
        {
            _value->setParent(this);
        }
        ASTNode const* value() const {
            return _value;
        }
        ~ASTNegativeExpression() {
            if(_value) {
                delete _value;
            }
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
        ~ASTIfStatement() {
            if(_condition) {
                delete _condition;
            }
            if(_thenBranch) {
                delete _thenBranch;
            }
            if(_elseBranch) {
                delete _elseBranch;
            }
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
        ~ASTWhileStatement() {
            if(_condition) {
                delete _condition;
            }
            if(_body) {
                delete _body;
            }
        }
    };

    class ASTFunction : public ASTNode {
    private:
        ksgw::Name  _name;
        ASTNode*    _params;
        ASTNode*    _body;
    public:
        ASTFunction() : ASTNode(ASTNodeType::Function)
            , _params(nullptr)
            , _body(nullptr)
        {}
        void setName(ksgw::Name name) {
            _name = name;
        }
        void setParams(ASTNode* params) {
            _params = params;
            _params->setParent(this);
        }
        void setBody(ASTNode* body) {
            _body = body;
            _body->setParent(this);
        }
        ~ASTFunction() {
            if(_params) {
                delete _params;
            }
            if(_body) {
                delete _body;
            }
        }
    };

    class ASTMultiExpr : public ASTNode {
    private:
        std::vector<ASTNode*> _expressions;
    public:
        ASTMultiExpr(ASTNodeType type)
            : ASTNode(type)
            , _expressions()
        {}
        void addExpr(ASTNode* expr) {
            _expressions.push_back(expr);
            expr->setParent(this);
        }
        ~ASTMultiExpr() {
            for(auto expr : _expressions) {
                delete expr;
            }
        }
    };

}