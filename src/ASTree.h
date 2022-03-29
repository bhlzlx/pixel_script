#pragma once

namespace compiler {
    class Token;
    class Lexer;

    enum class ASTNodeType {
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
        Literal,
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
        String,
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
        virtual ~ASTNode() {}
    };

    class ASTLeaf : public ASTNode {
    protected:
        Token* _token;
    public:
        ASTLeaf(Token* token)
            : _token(token)
        {}
        Token const* token() const {
            return _token;
        }
    };

    class NumberLiteral : public ASTLeaf {
    private:
        double _value;
    public:
        NumberLiteral( double value ) 
            : ASTLeaf( nullptr)
            , _value(value) 
        {}
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

    class ASTMultiExpression : public ASTNode {
    private:
        vector<ASTNode*> _expressions;
    };


}