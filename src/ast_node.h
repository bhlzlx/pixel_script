#pragma once
#include <vector>
#include "vm/vm_types.h"

namespace compiler {

    namespace ast {

        enum class SType : uint8_t {
            MultiExpr,
            If,
            While,
            BinaryOp,
            Variable,
            Leaf,
            Primary,
            NegtiveOp,
            // Pair,
            Function,
            StringList,
            None,
        };

        enum class VType : uint8_t {
            None,
            Int,
            String,
            Float,
            Closure,
            Id,
            Block,
            Module,
            Args, // 实参，表达式列表
            Params, // 形参，id列表
            Package,
        };


        class Node {
        protected:
            SType           _stype;
            VType           _vtype;
            Node*           _parent;
        public:
            Node(SType stype, VType vtype, Node* parent = nullptr)
                : _stype(stype)
                , _vtype(vtype)
                , _parent(parent)
            {}
            void setParent(Node* parent) {
                _parent = parent;
            }
            SType structType() const {
                return _stype;
            }
            VType valueType() const {
                return _vtype;
            }
            Node* parent() const {
                return _parent;
            }
            virtual ~Node() {}

            ASTStringList* asStringList() const {
                if(_stype == SType::StringList) {
                    return (ASTStringList*)this;
                } else {
                    return nullptr;
                }
            }
            ASTIdentifier* asId() const {
                if(_vtype == VType::Id) {
                    return (ASTIdentifier*)this;
                } else {
                    return nullptr;
                }
            }
            ASTIfStatement* asIf() const {
                if(_stype == SType::If) {
                    return (ASTIfStatement*)this;
                } else {
                    return nullptr;
                }
            }
            ASTWhileStatement* asWhile() const {
                if(_stype == SType::While) {
                    return (ASTWhileStatement*)this;
                } else {
                    return nullptr;
                }
            }
            ASTFunction* asFunction() const {
                if(_stype == SType::Function) {
                    return (ASTFunction*)this;
                } else {
                    return nullptr;
                }
            }
            ASTMultiExpr* asMultiExpr() const {
                if(_stype == SType::MultiExpr) {
                    return (ASTMultiExpr*)this;
                } else {
                    return nullptr;
                }
            }
            ASTNegativeExpression* asNegativeExpr() const {
                if(_stype == SType::NegtiveOp) {
                    return (ASTNegativeExpression*)this;
                } else {
                    return nullptr;
                }
            }
            ASTBinaryOpExpr* asBinaryExpr() const {
                if(_stype == SType::BinaryOp) {
                    return (ASTBinaryOpExpr*)this;
                } else {
                    return nullptr;
                }
            }
            ASTVariable* asVar() const {
                return _stype == SType::Variable ? (ASTVariable*)this : nullptr;
            }
            ASTLeaf* asLeaf() const {
                return _stype == SType::Leaf ? (ASTLeaf*)this : nullptr;
            }
            ASTPrimary* asPrimary() const {
                return _stype == SType::Primary ? (ASTPrimary*)this : nullptr;
            }
        };

        class ASTStringList : public Node {
        private:
            std::vector<Token> _names;
        public:
            ASTStringList(VType type)
                : Node(SType::StringList, type)
                , _names()
            {}
            ASTStringList(std::vector<Token>& names, VType type)
                : Node(SType::StringList, type)
                , _names(std::move(names))
            {}
            void addName(Token name) {
                _names.push_back(name);
            }
            std::vector<Token> const& names() const {
                return _names;
            }
        };

        class ASTLeaf : public Node {
        protected:
            Token           _token;
        public:
            ASTLeaf(VType vtype, Token token)
                : Node(SType::Leaf, vtype, nullptr)
                , _token(token)
            {}
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
            ASTIdentifier(Token token)
                : ASTLeaf(VType::Id, token)
            {
                _valid = false;
                _processed = false;
                _idType = (uint64_t)IdentifierType::Null;
                _value = 0;
            }

            void setASTNode(Node* node) const {
                _idType = uint8_t(IdentifierType::Global);
                _value = (uint64_t)node;
            }

            Node* astNode() const {
                if(_valid) {
                    return (Node*)_value;
                }
                return nullptr;
            }

            bool processed() const {
                return _processed;
            }

            void valid() const {
                _valid = true;
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

        class ASTMultiExpr : public Node {
        private:
            std::vector<Node*> _expressions;
        public:
            ASTMultiExpr(VType vtype)
                : Node(SType::MultiExpr, vtype)
                , _expressions()
            {}
            void addExpr(Node* expr) {
                _expressions.push_back(expr);
                expr->setParent(this);
            }
            std::vector<Node*> const& expressions() const {
                return _expressions;
            }
            ~ASTMultiExpr() {
                for(auto expr : _expressions) {
                    delete expr;
                }
            }
        };

        /** 
         * 目前是个函数调用
        */
        class ASTPrimary : public Node {
        private:
            Node*               _operand;
            ASTMultiExpr*       _params;
        public:
            ASTPrimary(Node* operand, ASTMultiExpr* postfix)
                : Node(SType::Primary, VType::None, nullptr)
                , _operand(operand)
                , _params(postfix)
            {
                operand->setParent(this);
                postfix->setParent(this);
            }
            Node* operand() const {
                return _operand;
            } 
            ASTMultiExpr* args() const {
                return _params;
            }
        };

        class ASTBinaryOpExpr : public Node {
        protected:
            TokenType       _op;
            Node*        _left;
            Node*        _right;
        public:
            ASTBinaryOpExpr(Node* left = nullptr, Node* right = nullptr, TokenType op = TokenType::None)
                : Node(SType::BinaryOp, VType::None, nullptr)
                , _op(op)
                , _left(left)
                , _right(right)
            {
                if(_left) {
                    _left->setParent(this);
                }
                if(_right) {
                    _right->setParent(this);
                }
            }
            Node const* left() const {
                return _left;
            }
            Node const* right() const {
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

        class ASTVariable : public Node {
        private:
            // Token           _name;
            ASTIdentifier*      _id;
            Node*               _value;
        public:
            ASTVariable(ASTIdentifier* id, Node* value)
                : Node(SType::Variable, VType::None)
                , _id(id)
                , _value(value)
            {
                id->setParent(this);
                if(value) {
                    value->setParent(this);
                }
            }

            Token name() const {
                return _id->token();
            }

            ASTIdentifier* id() const {
                return _id;
            }

            Node* valueExpr() const {
                return _value;
            }
        };

        // class ASTPair : public Node {
        // protected:
        //     Node* _first;
        //     Node* _second;
        // public:
        //     ASTPair(VType type, Node* first, Node* second) 
        //         : Node(SType::Pair, type) 
        //         , _first(first)
        //         , _second(second) {}
        //     Node* first() const {
        //         return _first;
        //     }
        //     Node* second() const {
        //         return _second;
        //     }
        // };

        class ASTNegativeExpression : public Node {
        private:
            Token _op;
            Node* _value;
        public:
            ASTNegativeExpression(Node* value, Token op) : Node(SType::NegtiveOp, VType::None)
                , _op(op)
                , _value(value)
            {
                _value->setParent(this);
            }
            Token op() const {
                return _op;
            }
            Node const* value() const {
                return _value;
            }
            ~ASTNegativeExpression() {
                if(_value) {
                    delete _value;
                }
            }
        };

        class ASTIfStatement : public Node {
        private:
            Node* _condition;
            Node* _thenBranch;
            Node* _elseBranch;
        public:
            ASTIfStatement() : Node(SType::If, VType::None) 
                , _condition(nullptr)
                , _thenBranch(nullptr)
                , _elseBranch(nullptr)
            {}
            void setCondition(Node* condition) {
                _condition = condition;
                _condition->setParent(this);
            }
            void setThenBranch(Node* thenBranch) {
                _thenBranch = thenBranch;
                _thenBranch->setParent(this);
            }
            void setElseBranch(Node* elseBranch) {
                _elseBranch = elseBranch;
                _elseBranch->setParent(this);
            }
            Node* condition() const {
                return _condition;
            }
            Node* thenBranch() const {
                return _thenBranch;
            }
            Node* elseBranch() const {
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

        class ASTWhileStatement : public Node {
        private:
            Node* _condition;
            Node* _body;
        public:
            ASTWhileStatement()
                : Node(SType::While, VType::None)
                , _condition(nullptr)
                , _body(nullptr)
            {}
            void setCondition(Node* condition) {
                _condition = condition;
                _condition->setParent(this);
            }
            void setBody(Node* body) {
                _body = body;
                _body->setParent(this);
            }
            Node* condition() const {
                return _condition;
            }
            Node* body() const {
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
        class ASTFunction : public Node {
            friend class ::compiler::Env;
        private:
            struct {
                uint64_t        _valid: 1;    // 编译过而且没问题
                uint64_t        _compiled: 1;   // 编译过了
            };
            Token               _name;
            Name                _module;
            Value               _hostPackage;
            ASTStringList*      _params;
            Node*               _body;
            SymbolLayout*       _symbolLayout;
        public:
            ASTFunction(VType type)
                : Node(SType::Function, type)
                , _valid(0)
                , _compiled(0)
                , _name()
                , _module()
                , _hostPackage()
                , _params()
                , _body(nullptr)
                , _symbolLayout(nullptr)
            {}

            void setSymbolLayout(SymbolLayout* layout) {
                _symbolLayout = layout;
            }

            void setHostPackage(Value pack) {
                _hostPackage = pack;
            }

            Value hostPackage() const {
                return _hostPackage;
            }

            SymbolLayout* symbolLayout() const {
                return _symbolLayout;
            }

            bool valid() const {
                return _valid;
            }

            bool compiled() const {
                return _compiled;
            }

            void setName(Token name) {
                _name = name;
            }

            void setModule(Name module) {
                _module = module;
            }

            Name module() const {
                return _module;
            }

            void setParams(ASTStringList* params) {
                assert(params);
                _params = params;
                _params->setParent(this);
            }
            
            void setBody(Node* body) {
                _body = body;
                _body->setParent(this);
            }

            Token name() const {
                return _name;
            }

            //get body
            Node* body() const {
                return _body;
            }

            // global var and local var are different impl
            // void convertToGlobal() {
            //     if(_body) {
            //         ASTFunction* valueExprFunc = new ASTFunction(VType::Closure);
            //         ASTMultiExpr* funcBody = new ASTMultiExpr(VType::Block);
            //         funcBody->addExpr(_body);
            //         _body = valueExprFunc;
            //     }
            // }

            // get params
            std::vector<Token> const& params() const {
                static std::vector<Token> empty;
                if(_params) {
                    return _params->names();
                } else {
                    return empty;
                }
                // return _params->names();
            }

            ~ASTFunction() {
                if(_body) {
                    delete _body;
                }
            }
        };

    }


}