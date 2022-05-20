#pragma once
#include <vector>
#include "vm/vm_types.h"

namespace compiler {

    namespace ast {

        enum class SType : uint8_t {
            MultiExpr,
            If,
            // While,
            BinaryOp,
            Leaf,
            NegtiveOp,
            Pair,
            Function,
            StringList,
            Class,
            Return,
            MapItem,
            None,
        };

        enum class VType : uint8_t {
            None,
            Int,
            String,
            Float,
            Bool,
            Closure,
            Id,
            Variable,
            Block,
            Module,
            Args, // 实参，表达式列表
            DotAccess,
            FunctionCall,
            WhileStmt,
            Params, // 形参，id列表
            Package,
            Extends,
            ClassBody,
            NewOperator,
            Array,
            Map,
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

            StringList* asStringList() const {
                if(_stype == SType::StringList) {
                    return (StringList*)this;
                } else {
                    return nullptr;
                }
            }
            Identifier* asId() const {
                if(_vtype == VType::Id) {
                    return (Identifier*)this;
                } else {
                    return nullptr;
                }
            }
            IfStmt* asIf() const {
                if(_stype == SType::If) {
                    return (IfStmt*)this;
                } else {
                    return nullptr;
                }
            }
            WhileStmt* asWhile() const {
                if(_vtype == VType::WhileStmt) {
                    return (WhileStmt*)this;
                } else {
                    return nullptr;
                }
            }
            DotAccess* asDotAccess() const {
                if(_vtype == VType::DotAccess) {
                    return (DotAccess*)this;
                } else {
                    return nullptr;
                }
            }
            Function* asFunction() const {
                if(_stype == SType::Function) {
                    return (Function*)this;
                } else {
                    return nullptr;
                }
            }
            MultiExpr* asMultiExpr() const {
                if(_stype == SType::MultiExpr) {
                    return (MultiExpr*)this;
                } else {
                    return nullptr;
                }
            }
            NegativeExpr* asNegativeExpr() const {
                if(_stype == SType::NegtiveOp) {
                    return (NegativeExpr*)this;
                } else {
                    return nullptr;
                }
            }
            BinaryOpExpr* asBinaryExpr() const {
                if(_stype == SType::BinaryOp) {
                    return (BinaryOpExpr*)this;
                } else {
                    return nullptr;
                }
            }
            Class* asClass() const {
                if(_stype == SType::Class) {
                    return (Class*)this;
                } else {
                    return nullptr;
                }
            }
            Variable* asVar() const {
                return _vtype == VType::Variable ? (Variable*)this : nullptr;
            }
            Leaf* asLeaf() const {
                return _stype == SType::Leaf ? (Leaf*)this : nullptr;
            }
            FunctionCall* asFunctionCall() const {
                return _vtype == VType::FunctionCall ? (FunctionCall*)this : nullptr;
            }
            NewOperator* asNew() const {
                return _vtype == VType::NewOperator ? (NewOperator*)this : nullptr;
            }
            ReturnStmt* asReturn() const {
                return _stype == SType::Return ? (ReturnStmt*)this : nullptr;
            }
        };

        class StringList : public Node {
        private:
            std::vector<Token> _names;
        public:
            StringList(VType type)
                : Node(SType::StringList, type)
                , _names()
            {}
            StringList(std::vector<Token>& names, VType type)
                : Node(SType::StringList, type)
                , _names(std::move(names))
            {}
            void addName(Token name) {
                _names.push_back(name);
            }
            void pushFront(Token name) { // 类函数需要补一个this参数
                _names.insert(_names.begin(), name);
            }
            std::vector<Token> const& names() const {
                return _names;
            }
        };

        class ReturnStmt : public Node {
        private:
            Node* _expr;
        public:
            ReturnStmt(Node* expr)
                : Node(SType::Return, VType::None)
                , _expr(expr)
            {
                if(expr) {
                    expr->setParent(this);
                }
            }
            Node* expr() const {
                return _expr;
            }
        };

        class PairExpr : public Node {
        protected:
            Node* _first;
            Node* _second;
        public:
            PairExpr(VType vtype, Node* first, Node* second)
                : Node(SType::Pair, vtype)
                , _first(first)
                , _second(second) 
            {
                assert(first); first->setParent(this);
                if(second) {
                    second->setParent(this);
                }
            }
            Node* first() const {
                return _first;
            }
            Node* second() const {
                return _second;
            }
            ~PairExpr() {
                if(_first) {
                    delete _first;
                }
                if(_second) {
                    delete _second;
                }
            }
        };

        class FunctionCall : public PairExpr {
        public:
            FunctionCall(Node* methodExpr, MultiExpr* args)
                : PairExpr(VType::FunctionCall, methodExpr, (Node*)args)
            {}
            Node* methodExpr() const { return _first; }
            MultiExpr* args() const { return _second ? _second->asMultiExpr() : nullptr; }
        };

        class DotAccess : public PairExpr {
        public:
            DotAccess(Node* obj, Node* field)
                : PairExpr(VType::DotAccess, obj, field)
            {}
            Node* obj() const { return _first; }
            Node* field() const { return _second; }
        };

        class Variable : public PairExpr {
        public:
            Variable(Identifier* id, Node* value)
                : PairExpr(VType::Variable, (Node*)id, value)
            {
            }
            Token name() const;
            Identifier* id() const { return _first->asId(); }
            Node* valueExpr() const { return _second; }
        };

        class WhileStmt : public PairExpr {
        public:
            WhileStmt(Node* cond, Node* body)
                : PairExpr(VType::WhileStmt, cond, body)
            {}
            Node* condition() const { return _first; }
            Node* body() const { return _second; }
        };


        class Leaf : public Node {
        protected:
            Token           _token;
        public:
            Leaf(VType vtype, Token token)
                : Node(SType::Leaf, vtype, nullptr)
                , _token(token)
            {}
            Token token() const {
                return _token;
            }
        };

        class ClassExtends : public Leaf {
        public:
            ClassExtends(Token token)
                : Leaf(VType::Extends, token)
            {}
        };

        class Identifier: public Leaf {
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
            Identifier(Token token)
                : Leaf(VType::Id, token)
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

        class MultiExpr : public Node {
        private:
            std::vector<Node*> _expressions;
        public:
            MultiExpr(VType vtype)
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
            ~MultiExpr() {
                for(auto expr : _expressions) {
                    delete expr;
                }
            }
        };

        class BinaryOpExpr : public Node {
        protected:
            TokenType       _op;
            Node*           _left;
            Node*           _right;
        public:
            BinaryOpExpr(Node* left = nullptr, Node* right = nullptr, TokenType op = TokenType::None)
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
            ~BinaryOpExpr() {
                if(_left) {
                    delete _left;
                }
                if(_right) {
                    delete _right;
                }
            }
        };

        class NegativeExpr : public Node {
        private:
            Token _op;
            Node* _value;
        public:
            NegativeExpr(Node* value, Token op) : Node(SType::NegtiveOp, VType::None)
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
            ~NegativeExpr() {
                if(_value) {
                    delete _value;
                }
            }
        };

        class IfStmt : public Node {
        private:
            Node* _condition;
            Node* _thenBranch;
            Node* _elseBranch;
        public:
            IfStmt() : Node(SType::If, VType::None) 
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
            ~IfStmt() {
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

        /**
         * @brief Function
         *   函数的AST描述，每个函数都有唯一的变量表
         * 脚本加载这后不会更新这些变量表，而是在首次执行时更新变量表，如果脚本有重载行为，则会设置为脏
         * 强制下次执行时间更新变量表
         */
        class Function : public Node {
            friend class ::compiler::Env;
            friend class ::compiler::Module;
        protected:
            struct {
                uint64_t        _valid: 1;    // 编译过而且没问题
                uint64_t        _compiled: 1;   // 编译过了
            };
            Token               _name;
            Name                _module;
            Value               _hostPackage;
            StringList*         _params;
            Node*               _body;
            SymbolLayout*       _symbolLayout;
        public:
            Function(VType type)
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

            void setParams(StringList* params) {
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

            // get params
            std::vector<Token> const& params() const {
                static std::vector<Token> empty;
                if(_params) {
                    return _params->names();
                } else {
                    return empty;
                }
            }

            void addSelfParam() {
                if(!_params) {
                    _params = new StringList(VType::Params);
                }
                _params->pushFront(Token(TokenType::Identifier, lang_keywords::_self));
            }

            ~Function() {
                if(_body) {
                    delete _body;
                }
            }
        };

        class Class : public Node {
        private:
            Token           _name;
            ClassExtends*   _extends;
            MultiExpr*      _body;
        public:
            Class(Token name, ClassExtends* extends, MultiExpr* body)
                : Node(SType::Class, VType::None)
                , _name(name)
                , _extends(extends)
                , _body(body)
            {}

            Name name() {
                return _name.stringLiteral();
            }

            ClassExtends* extends() {
                return _extends;
            }

            MultiExpr* body() {
                return _body;
            }
        };

        class NewOperator : public Function {
        private:
            SymbolLayout*   _classSymbol;
        public:
            NewOperator(SymbolLayout* layout)
                : Function(VType::NewOperator)
                , _classSymbol(layout)
            {
                _compiled = 1;
                _valid = 1;
            }

            void setSymbolLayout(SymbolLayout* layout) {
                _classSymbol = layout;
            }

            SymbolLayout* symbolLayout() {
                return _classSymbol;
            }
        };

        class Array : public MultiExpr {
        public:
            Array()
                : MultiExpr(VType::Array)
            {}
        };

        class MapItem : public Node {
        private:
            Name            _key;
            Node*           _value;
        public:
            MapItem(Name key, Node* _value)
                : Node(SType::MapItem, VType::None)
            {}
            Name key() const {
                return _key;
            }
            Node* value() const {
                return _value;
            }
        };

    }


}