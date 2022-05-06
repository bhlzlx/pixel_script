#pragma once
#include <vector>
#include "ast_common.h"

namespace compiler {
    class Token;
    class Lexer;
    class SymbolLayout;
    class Value;

    enum class ASTNodeType {
        Package,
        Identifier,
        Integer,
        Float,
        String,
        Primary,
        Keyword,
        // Statements
        Block,
        CodeChunk,
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
        Variable, // define var
        // Types
        Function,
        Closure,
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
        ASTNodeType type() const {
            return _type;
        }
        ASTNode* parent() const {
            return _parent;
        }
        virtual ~ASTNode() {}
    };

    class ASTLeaf : public ASTNode {
    protected:
        Token           _token;
    public:
        ASTLeaf(ASTNodeType type, Token token) : ASTNode(type), _token(token) {}
        Token token() const {
            return _token;
        }
    };

    class ASTIdentifier: public ASTLeaf {
    private:
        union {
            struct {
                mutable uint64_t    _processed : 1;
                mutable uint64_t    _valid : 1;
                mutable uint64_t    _idType: 8;
                mutable uint64_t    _value: 48;
            };
            uint64_t   _raw;
        };
    public:
        ASTIdentifier(Token token) : ASTLeaf(ASTNodeType::Identifier, token) {
            _valid = false;
            _processed = false;
            _idType = (uint64_t)IdentifierType::Null;
            _value = 0;
        }

        void setASTNode(ASTNode* node) const {
            _idType = uint8_t(IdentifierType::Global);
            _value = (uint64_t)node;
        }

        ASTNode* astNode() const {
            if(_valid) {
                return (ASTNode*)_value;
            }
            return nullptr;
        }

        // 局部变量，变量也是动态生成的，所以只存局部变量的一个表中的位置
        // 也可能是通过包访问的一个变量，主时候
        void setValue(IdentifierType type, uint32_t loc) const {
            _processed = true;
            _valid = true;
            _idType = (uint8_t)type;
            _value = loc;
        }

        void setValue(Value* value) const { // 编译，静态变量，所以也可以认为它是全局的
            _processed = true;
            _valid = true;
            _idType = (uint8_t)IdentifierType::Global;
            _value = (uint64_t)value;
        }

        IdentifierType type() const {
            return (IdentifierType)_idType;
        }

        uint32_t valueLoc() const {
            return _value;
        }
    };

    /** 
     * 目前是个函数调用
    */

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
        TokenType op() const {
            return _op;
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

    class ASTVariable : public ASTNode {
    private:
        // Token           _name;
        ASTIdentifier*  _id;
        ASTNode*        _value;
    public:
        ASTVariable(ASTIdentifier* id, ASTNode* value)
            : ASTNode(ASTNodeType::Variable)
            , _id(id)
            , _value(value)
        {}

        Token name() const {
            return _id->token();
        }

        ASTIdentifier* id() const {
            return _id;
        }

        ASTNode* valueExpr() const {
            return _value;
        }
    };

    class ASTDoubleStructure : public ASTNode {
    protected:
        ASTNode* _first;
        ASTNode* _second;
    public:
        ASTDoubleStructure(ASTNodeType type, ASTNode* first, ASTNode* second) 
            : ASTNode(type), 
            _first(first),
            _second(second) {}
        ASTNode* first() const {
            return _first;
        }
        ASTNode* second() const {
            return _second;
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
        ASTNode* condition() const {
            return _condition;
        }
        ASTNode* thenBranch() const {
            return _thenBranch;
        }
        ASTNode* elseBranch() const {
            return _elseBranch;
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
        ASTNode* condition() const {
            return _condition;
        }
        ASTNode* body() const {
            return _body;
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

    /**
     * @brief ASTFunction
     *   函数的AST描述，每个函数都有唯一的变量表
     * 脚本加载这后不会更新这些变量表，而是在首次执行时更新变量表，如果脚本有重载行为，则会设置为脏
     * 强制下次执行时间更新变量表
     */
    class ASTFunction : public ASTNode {
    private:
        struct {
            uint64_t        _valid: 1;    // 编译过而且没问题
            uint64_t        _compiled: 1;   // 编译过了
        };
        Token               _name;
        std::vector<Name>   _params;
        // ASTNode*            _params;
        ASTNode*            _body;
        SymbolLayout*       _symbolLayout;
    public:
        ASTFunction() : ASTNode(ASTNodeType::Function)
            , _valid(0)
            , _compiled(0)
            , _params()
            , _body(nullptr)
            , _symbolLayout(nullptr)
        {}

        bool valid() const {
            return _valid;
        }

        bool compiled() const {
            return _compiled;
        }

        void setName(Token name) {
            _name = name;
        }

        void setParams( std::vector<Name>& names ) {
            _params = std::move(names);
            // _params->setParent(this);
        }
        
        void setBody(ASTNode* body) {
            _body = body;
            _body->setParent(this);
        }

        Token name() const {
            return _name;
        }

        //get body
        ASTNode* body() const {
            return _body;
        }

        // get params
        std::vector<Name> const& params() const {
            return _params;
        }

        ~ASTFunction() {
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
        std::vector<ASTNode*> const& expressions() const {
            return _expressions;
        }
        ~ASTMultiExpr() {
            for(auto expr : _expressions) {
                delete expr;
            }
        }
    };

    class ASTPackage : public ASTNode {
    private:
        std::vector<Name> _names;
    public:
        ASTPackage(std::vector<Name>& names) : ASTNode(ASTNodeType::Package)
            , _names(std::move(names))
        {}
        void addName(Name name) {
            _names.push_back(name);
        }
        std::vector<Name> const& names() const {
            return _names;
        }
    };

}