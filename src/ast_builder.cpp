#include "ast_builder.h"
#include "token.h"
#include "token_parser.h"
#include "vm/vm_env.h"
#include "ast_node.h"
#include <map>
#include <stack>
#include <cassert>

using namespace compiler::ast;

namespace compiler {

    class ConsumeStateHelper {
    private:
        ASTBuilder*     _builder;
        bool            _resumable;
    public:
        ConsumeStateHelper(ASTBuilder* builder, bool resumable)
            : _builder(builder)
            , _resumable(resumable)
        {
            _builder->pushConsumeState();
        }
        ~ConsumeStateHelper() {
            if(_resumable) {
                _builder->discardConsumeState();   
            } else {
                _builder->popConsumeState();
            }
        }
    };

    /**
     * @brief 
     * bnf : ("{"expr"}"|NUMBER|IDENTIFIER|STRING) {postfix}
     * bnf : ("{"expr"}"|IDENTIFIER) {postfix}
     *  value
     *  1234
     * "hello"
     * 1+2+value
     * func(a,b,c)
     * 
     * @param _tokenParser 
     * @return MatchResult 
     */
    MatchResult ASTBuilder::matchPrimary() {
        ConsumeStateHelper(this, true);
        MatchResult rst = {};
        consumeSemicolonEol();
        auto token = nextToken();
        auto startToken = *token;
        Node* operand = nullptr;
        switch(token->type()) {
            case TokenType::False:
            case TokenType::True: {
                consumeCurrentToken();
                operand = new Leaf(VType::Bool, *token);
                return { operand, ASTParseError::None, token->line(), token->column() };
            }
            case TokenType::Integer: {
                consumeCurrentToken();
                operand = new Leaf(VType::Int, *token);
                return { operand, ASTParseError::None, token->line(), token->column() };
            }
            case TokenType::String: {
                consumeCurrentToken();
                operand = new Leaf(VType::String, *token);
                return { operand, ASTParseError::None, token->line(), token->column() };
            }
            case TokenType::Keyword: { // 目前关键字只有func
                if(token->stringLiteral() == lang_keywords::_func) {
                    auto closure = matchClosure();
                    if(closure) {
                        return closure;
                    }
                    return { nullptr, ASTParseError::FunctionMismatch, token->line(), token->column() };
                }
                if(token->stringLiteral() == lang_keywords::_self) {
                    consumeCurrentToken();
                    return { new Leaf(VType::Id, *token), ASTParseError::None, token->line(), token->column() };
                }
                return { nullptr, ASTParseError::PrimaryMismatch, token->line(), token->column() };
            }
            case TokenType::LeftParen: {
                consumeCurrentToken();
                auto expr = matchExpression();
                if(!expr) {
                    rst = expr; // match expression failed
                } else {
                    auto token = nextToken();
                    if(token->type() == TokenType::RightParen) {
                        consumeCurrentToken();
                        operand = expr.node;
                    } else {
                        delete expr.node;
                        return { nullptr, ASTParseError::RightParenExpected, token->line(), token->column() };
                    }
                }
                break;
            }
            case TokenType::Identifier: {
                consumeCurrentToken();
                Identifier* id = new Identifier(*token);
                operand = (Node*)id;
                break;
            }
            default: {
                return { nullptr, ASTParseError::PrimaryMismatch, token->line(), token->column() };
            }
        }
        if(!operand) {
            return { nullptr, ASTParseError::PrimaryMismatch, startToken.line(), startToken.column() };
        } else { // 尝试匹配.
            return { operand, ASTParseError::None, startToken.line(), startToken.column() };
        }
    }

    MatchResult ASTBuilder::matchIndexAccess() {
        ConsumeStateHelper(this, true);
        MatchResult rst = {};
        auto token = nextToken();
        if(token->type() == TokenType::LeftBracket) {
            consumeCurrentToken();
            auto expr = matchExpression();
            if(!expr) {
                return expr;
            } else {
                auto token = nextToken();
                if(token->type() == TokenType::RightBracket) {
                    consumeCurrentToken();
                    return { expr.node, ASTParseError::None, token->line(), token->column() };
                } else {
                    delete expr.node;
                    return { nullptr, ASTParseError::RightBracketExpected, token->line(), token->column() };
                }
            }
        }
        return { nullptr, ASTParseError::None, token->line(), token->column() };
    }

    MatchResult ASTBuilder::matchFactor() {
        ConsumeStateHelper helper(this, true);
        MatchResult rst = {};
        auto token = nextToken();
        auto startToken = *token;
        auto array = matchArray();
        // 添加array map支持
        if(array) {
            return array;
        }
        auto map = matchMap();
        if(map) {
            return map;
        }
        if (token->type() == TokenType::Minus) {
            consumeCurrentToken();
            auto primNode = matchPrimary();
            if(primNode) {
                rst = { 
                    new NegativeExpr(primNode.node, *token), 
                    ASTParseError::None,
                    startToken.line(),
                    startToken.column()
                };
                _addDebugInfo(rst.node, {token->stringLiteral(), token->line(), token->column()});
            } else {
                rst = primNode; // failed
            }
        } else {
            rst =  matchPrimary();
            if(!rst) {
                return { nullptr, ASTParseError::PrimaryMismatch, startToken.line(), startToken.column() };
            }
            Node* node = rst.node;
            while(true)  {
                rst = matchArgList(); // 参见 args 的匹配实现，实际是multiexpr
                // 补充一下，虽然匹配args会返回空，并不意味着失败，参数量为0时也会返回空，所以再判断下error code，
                // 如果error code没有错误，则说明匹配成功，只是没有参数
                if(rst || rst.error == ASTParseError::None) {
                    // global_func(e,f,g)
                    // a.b.c.d(e,f,g)，现在匹配括号部分了，看上级，也就是c.d
                    // 实际上，这个全局的函数调用看似没有点语法，实际是有一个隐匿的全局包/this对象
                    // function object expr
                    /*
                    1. 点语法
                    2. 函数返回值
                    3. 作用域内（全局，包内，类内）
                    */
                    // 所以我们不妨在这里判断下这个节点的类型，并作断言
                    auto args = rst;
                    FunctionCall* funcCall = nullptr;
                    // 现在我拿到的是这个factor的语法节点的顶节点，然后我们就可以针对不同情况做特殊处理，
                    switch(node->valueType()) {
                        case VType::DotAccess: { 
                            // 这个是可以处理得非常直接的，原本是一个dot access，现在我们要把这个节点删除掉
                            // 改成函数调用，而不是作为新节点的子节点
                            auto dotAccess = node->asDotAccess();
                            funcCall = FunctionCall::fromDotAccess(dotAccess, args.node);
                            delete dotAccess;
                            break;
                        }
                        case VType::FunctionCall: { // 基操，这种函数调用，没有self，它就是个自由函数
                            funcCall = new FunctionCall(nullptr, node, args.node); // no function name break;
                            break;
                        }
                        case VType::Id: { 
                            // 这种还是有可能不是包内全局函数的，如果是成员函数
                            // 前边是不用加self的，但是语法树这里还是判断不了的，需要做后处理
                            funcCall = new FunctionCall(nullptr, node, args.node); // no function name break;
                            break;
                        }
                        default: {
                            assert(false);
                        }
                    }
                    // 出于教学目的的话，这里帮助大家理解一些，实际上，这里完全没必要处理，因为在后处理时会处理这些东西
                    _addDebugInfo(funcCall, { token->stringLiteral(), token->line(), token->column() });
                    node = funcCall;
                    break;
                } else {
                    token = nextToken();
                    rst = matchIndexAccess(); // 索引表达式
                    if(rst) {
                        auto indexAccess = new IndexAccess(node, rst.node);
                        node = indexAccess;
                        _addDebugInfo(indexAccess, {token->stringLiteral(), token->line(), token->column() });
                    } else {
                        token = nextToken();
                        rst = matchDotAccess(); // 返回的是一个Leaf String
                        if(rst) {
                            auto dotAccess = new DotAccess(node, rst.node);
                            node = dotAccess;
                            _addDebugInfo(dotAccess, {token->stringLiteral(), token->line(), token->column() });
                        } else {
                            break;
                        }
                    }
                }
            }
            return { node, ASTParseError::None, startToken.line(), startToken.column() };
        }
        return rst;
    }

    MatchResult ASTBuilder::matchExpression() {
        ConsumeStateHelper helper(this, true);
        MatchResult rst = {};
        auto startToken = *nextToken();
        std::stack<Node*> factorStack;
        std::stack<Token> opStack;
        auto factor = matchFactor();
        if(factor) {
            factorStack.push(factor.node);
            while(true) {
                Token const* token = nextToken();
                switch(token->type()) {
                    case TokenType::Minus:
                    case TokenType::Slash:
                    case TokenType::Plus:
                    case TokenType::Star:
                    case TokenType::Assign:
                    case TokenType::Modulus:
                    case TokenType::Less:
                    case TokenType::LessEqual:
                    case TokenType::Greater:
                    case TokenType::GreaterEqual:
                    case TokenType::Equal:
                    case TokenType::Dot:
                        break;
                    default:
                        // expr pattern end
                        while(!opStack.empty()) {
                            auto op = opStack.top();
                            opStack.pop();
                            auto right = factorStack.top();
                            factorStack.pop();
                            auto left = factorStack.top();
                            factorStack.pop();
                            auto binExpr = new BinaryOpExpr(left, right, op.type());
                            _addDebugInfo(binExpr, {op.stringLiteral(), op.line(), op.column()});
                            factorStack.push(binExpr);
                        }
                        auto rst = factorStack.top();
                        assert(factorStack.size() == 1);
                        return { rst, ASTParseError::None, startToken.line(), startToken.column() };
                }
                consumeCurrentToken();
                // TokenType op = token->type();
                if(opStack.size()) {
                    auto top = opStack.top();
                    if(token->type() >= top.type()) {
                        auto right = factorStack.top(); factorStack.pop();
                        auto left = factorStack.top(); factorStack.pop();
                        auto binExpr = new BinaryOpExpr(left, right, top.type());
                        _addDebugInfo(binExpr, {top.stringLiteral(), top.line(), top.column()});
                        opStack.pop();
                        factorStack.push(binExpr);
                    }
                }
                opStack.push(*token);
                factor = matchFactor();
                if(factor) {
                    factorStack.push(factor.node);
                } else { 
                    // clean up status & throw the exception
                    while(!factorStack.empty()) {
                        auto factor = factorStack.top();
                        delete factor;
                        factorStack.pop();
                    }
                    auto lastToken = nextToken();
                    rst = { nullptr, ASTParseError::NeedRightFactor, lastToken->line(), lastToken->column() };
                    return rst;
                }
            }
        } else {
            rst = factor;
        }
        return rst;
    }

    MatchResult ASTBuilder::matchBlock() {
        ConsumeStateHelper helper(this, true);
        MatchResult rst = {};
        auto token = nextToken();
        if(token->type() == TokenType::LeftBrace) {
            consumeCurrentToken();
            auto exprs = new MultiExpr(VType::Block);
            auto statement = matchStatement();
            if(statement) {
                exprs->addExpr(statement.node);
            }
            while(true) {
                token = nextToken();
                if(token->type() == TokenType::Semicolon || token->type() == TokenType::Eol) {
                    consumeCurrentToken();
                } else if(token->type() == TokenType::RightBrace) {
                    consumeCurrentToken();
                    return { exprs, ASTParseError::None, token->line(), token->column() };
                } else {
                    statement = matchStatement();
                    if(statement) {
                        exprs->addExpr(statement.node);
                    } else {
                        delete exprs;
                        return statement;
                    }
                }
            }
        } else {
            return { nullptr, ASTParseError::BlockMismatch, token->line(), token->column() };
        }
        return rst;
    }

    MatchResult ASTBuilder::matchStatement() { 
        ConsumeStateHelper helper(this, false);
        MatchResult rst = {};
        auto token = nextToken();
        if(token->stringLiteral() == lang_keywords::_return) {
            auto retToken = *token;
            consumeCurrentToken();
            auto expr = matchExpression();
            if(expr) {
                auto retStmt = new ReturnStmt(expr.node);
                return { retStmt, ASTParseError::None, retToken.line(), retToken.column() };
            } else {
                auto retStmt = new ReturnStmt(nullptr);
                return { retStmt, ASTParseError::None, retToken.line(), retToken.column() };
            }
        } else if(token->stringLiteral() == lang_keywords::_if) {
            auto IfToken = *token;
            IfStmt* if_stmt = new IfStmt();
            consumeCurrentToken();
            auto expr = matchExpression();
            if(!expr) {
                rst = expr;
            } else {
                if_stmt->setCondition(expr.node);
                auto block = matchBlock();
                if(!block) {
                    delete if_stmt;
                    rst =  block;
                } else {
                    if_stmt->setThenBranch(block.node);
                    token = nextToken();
                    if(token->stringLiteral() == lang_keywords::_else) {
                        consumeCurrentToken();
                        auto block = matchBlock();
                        if(!block) {
                            delete if_stmt;
                            if_stmt = nullptr;
                            rst = block;
                        } else {
                            if_stmt->setElseBranch(block.node);
                        }
                    } 
                }
                return { if_stmt, ASTParseError::None, IfToken.line(), IfToken.column() };
            }
        } else if(token->stringLiteral() == lang_keywords::_while) {
            auto WhileToken = *token;
            consumeCurrentToken();
            auto expr = matchExpression();
            if(!expr) {
                rst = expr;
            } else {
                auto block = matchBlock();
                if(!block) {
                    delete expr.node;
                    rst = block;
                } else {
                    auto while_stmt = new WhileStmt(expr.node, block.node);
                    return { while_stmt, ASTParseError::None, WhileToken.line(), WhileToken.column() };
                }
            }
        } else {
            rst = matchDefVariable();
            if(rst) {
                return rst;
            } else {
                return matchExpression();
            }
        }
        return rst;
    }

    MatchResult ASTBuilder::matchCodeChunk() {
        ConsumeStateHelper helper(this, false);
        auto chunk = new MultiExpr(VType::Module);
        auto pack = matchPackage();
        if(pack) {
            chunk->addExpr(pack.node);
            auto token = nextToken();
            while(token->type() != TokenType::Eof) {
                auto func = matchFunctionDef();
                if(func) {
                    chunk->addExpr(func.node);
                } else {
                    auto var = matchDefVariable();
                    if(var) {
                        chunk->addExpr(var.node);
                    } else {
                        auto clazz = matchClassDef();
                        if(clazz) {
                            chunk->addExpr(clazz.node);
                        } else {
                            token = nextToken();
                            if(token->type() == TokenType::Eof) {
                                return { chunk, ASTParseError::None, token->line(), token->column() };
                            } else {
                                delete chunk;
                                return { nullptr, ASTParseError::VarMismatch, token->line(), token->column() };
                            }
                        }
                    }
                }
            }
            return { chunk, ASTParseError::None, token->line(), token->column() };
        } else {
            return pack;
        }
        return { chunk, ASTParseError::None, 0, 0 };
    }

    /**
     * @brief 
     *  bnf : params : param {, param}
     * @param _tokenParser 
     * @return MatchResult StringList
     */
    MatchResult ASTBuilder::matchParams() {
        ConsumeStateHelper helper(this, true);
        MatchResult rst = {};
        auto token = nextToken();
        Token startToken = *token;
        if(token->type() != TokenType::Identifier) {
            rst = MatchResult { nullptr, ASTParseError::ParamsListMismatch, token->line(), token->column() };
        } else {
            std::vector<Token> tokens;
            tokens.push_back(*token);
            consumeCurrentToken();
            auto param = new Leaf(VType::Id, *token);
            while(true) {
                token = nextToken();
                if(token->type() == TokenType::Comma) {
                    consumeCurrentToken();
                    token = nextToken();
                    if(token->type() != TokenType::Identifier) {
                        return MatchResult { nullptr, ASTParseError::IdentifierExpected, token->line(), token->column() };
                    } else {
                        tokens.push_back(*token);
                        consumeCurrentToken();
                    }
                } else {
                    break;
                }
            }
            return { new StringList(tokens, VType::Params), ASTParseError::None, startToken.line(), startToken.column() };
        }
        return rst;
    }

    /**
     * @brief 
     *  bnf : paramlist : "(" params ")"
     * @param _tokenParser 
     * @return MatchResult 
     *  if no params, return nullptr
     */
    MatchResult ASTBuilder::matchParamList() {
        ConsumeStateHelper helper(this, true);
        auto currToken = nextToken();
        auto startToken = *currToken;
        if(currToken->type() == TokenType::LeftParen) {
            consumeCurrentToken();
            auto params = matchParams(); // maybe no params
            currToken = nextToken();
            if(currToken->type() == TokenType::RightParen) {
                consumeCurrentToken();
                params.error = ASTParseError::None; // if has error, set as no error!
                return params;
            } else {
                delete params.node; // clean up
                return { nullptr, ASTParseError::RightParenExpected, currToken->line(), currToken->column() }; // failed
            }
        } else {
            return {nullptr, ASTParseError::ParamsListMismatch, startToken.line(), startToken.column()}; // failed
        }
    }

    /**
     * @brief 
     * bnf : function : "func" ident "(" paramlist ")" block
     * 
     * @param _tokenParser 
     * @return MatchResult 
     */
    MatchResult ASTBuilder::matchFunctionDef() {
        consumeSemicolonEol();
        ConsumeStateHelper helper(this, true);
        Function* func = nullptr;
        auto token = nextToken();
        Token startToken = *token;
        if(token->stringLiteral() != lang_keywords::_func) { // "func"
            return MatchResult { nullptr, ASTParseError::FunctionMismatch, token->line(), token->column() };
        } else {
            consumeCurrentToken();
            token = nextToken();
            if(token->type() != TokenType::Identifier) { // ident
                return MatchResult { nullptr, ASTParseError::MissFunctionName, token->line(), token->column() };
            } else {
                consumeCurrentToken();
                auto func = new Function(VType::None);
                func->setName(*token);
                auto paramList = matchParamList(); // paramlist
                if(paramList) {
                    func->setParams(paramList.node->asStringList());
                } else {
                    func->setParams(new StringList(VType::Params));
                }
                auto block = matchBlock(); // body
                if(!block) {
                    delete func;
                    return block; // failed
                } else {
                    func->setBody(block.node);
                    return { func, ASTParseError::None, startToken.line(), startToken.column() };
                }
            }
        }
    }


    /**
     * @brief 
     * bnf : expr {, expr}
     * 
     * @param _tokenParser 
     * @return MatchResult multiple expressions
     */
    MatchResult ASTBuilder::matchArgs() {
        ConsumeStateHelper helper(this, true);
        MatchResult rst = {};
        auto beginToken = *nextToken();
        auto expr = matchExpression();
        if(!expr) {
            return expr; // no expr, empty args
        } else {
            auto multiExpr = new MultiExpr(VType::Args);
            multiExpr->addExpr(expr.node);
            while(true) {
                auto token = nextToken();
                if(token->type() == TokenType::Comma) {
                    consumeCurrentToken();
                    expr = matchExpression();
                    multiExpr->addExpr(expr.node);
                    if(!expr) {
                        delete multiExpr;
                        return expr;
                    }
                } else {
                    break;
                }
            }
            return { multiExpr, ASTParseError::None, beginToken.line(), beginToken.column() };
        }
    }
    
    /**
     * @brief 
     *  bnf : "(" [args] ")"
     * 
     * @param _tokenParser 
     * @return MatchResult multi expressions
     */
    MatchResult ASTBuilder::matchArgList() {
        ConsumeStateHelper helper(this, true);
        auto token = nextToken();
        MatchResult rst = {};
        Token startToken = *token;
        if(token->type() != TokenType::LeftParen) {
            return MatchResult { nullptr, ASTParseError::LeftParenExpected, token->line(), token->column() };
        } else {
            consumeCurrentToken();
            auto args = matchArgs();
            token = nextToken();
            if(token->type() != TokenType::RightParen) {
                delete args.node;
                return MatchResult { nullptr, ASTParseError::RightParenExpected, token->line(), token->column() };
            } else {
                consumeCurrentToken();
                return { args.node, ASTParseError::None, startToken.line(), startToken.column() };
            }
        }
        return rst;
    }

    MatchResult ASTBuilder::matchDotAccess() {
        ConsumeStateHelper helper(this, true);
        auto token = nextToken();
        Token startToken = *token;
        if(token->type() != TokenType::Dot) {
            return MatchResult { nullptr, ASTParseError::DotExpected, token->line(), token->column() };
        } else {
            consumeCurrentToken();
            token = nextToken();
            // 这里强制为Field
            if(token->type() == TokenType::Identifier || token->type() == TokenType::Keyword) {
                consumeCurrentToken();
                return { new Leaf(VType::Field, *token), ASTParseError::None, startToken.line(), startToken.column() };
            } else {
                return MatchResult { nullptr, ASTParseError::IdentifierExpected, token->line(), token->column() };
            }
        }
    }

    MatchResult ASTBuilder::matchClosure() {
        ConsumeStateHelper helper(this, true);
        auto token = nextToken();
        MatchResult rst = MatchResult { nullptr, ASTParseError::ClosureMismatch, token->line(), token->column() };
        Token startToken = *token;
        Node* params = nullptr;
        Node* body = nullptr;
        if(token->type() == TokenType::Keyword) {
            if(token->stringLiteral() == lang_keywords::_func) {
                consumeCurrentToken();
                params = matchParamList().node;
                body = matchBlock().node;
                if(!body) {
                    delete params;
                } else {
                    auto closure = new Function(VType::Closure);
                    closure->setParams(params->asStringList());
                    closure->setBody(body);
                    return { closure, ASTParseError::None, startToken.line(), startToken.column() };
                }
            }
        }
        return rst;
    }

    MatchResult ASTBuilder::matchDefVariable() {
        consumeSemicolonEol();
        ConsumeStateHelper helper(this, true);
        auto token = nextToken();
        if(token->type() == TokenType::Keyword) {
            if(token->stringLiteral() == lang_keywords::_var) {
                consumeCurrentToken();
                token = nextToken();
                if(token->type() == TokenType::Identifier) {
                    Token name = *token;
                    Identifier* id = new Identifier(name);
                    consumeCurrentToken();
                    token = nextToken();
                    if(token->type() != TokenType::Assign) {
                        if(token->type() == TokenType::Semicolon || token->type() == TokenType::Eol) {
                            consumeCurrentToken();
                            return { new Variable(id, nullptr), ASTParseError::None, name.line(), name.column() };
                        }
                        return { nullptr, ASTParseError::AssignExpected, name.line(), name.column() };
                    } else {
                        consumeCurrentToken();
                        auto expr = matchExpression();
                        if(!expr) {
                            return expr;
                        } else { 
                            return { new Variable(id, expr.node), ASTParseError::None, name.line(), name.column() };
                        }
                    }
                } else {
                    return { nullptr, ASTParseError::MissVariableName, token->line(), token->column() };
                }
            }
        }
        return { nullptr, ASTParseError::VarMismatch, token->line(), token->column() };
    }

    MatchResult ASTBuilder::matchPackage() {
        ConsumeStateHelper helper(this, true);
        consumeSemicolonEol();
        auto token = nextToken();
        if(token->type() == TokenType::Keyword) {
            if(token->stringLiteral() == lang_keywords::_package) {
                consumeCurrentToken();
                token = nextToken();
                std::vector<Token> names;
                if(token->type() == TokenType::Identifier) {
                    names.push_back(*token);
                    consumeCurrentToken();
                    while(true) {
                        token = nextToken();
                        if(token->type() == TokenType::Eol) {
                            consumeCurrentToken();
                            return { new StringList(names, VType::Package), ASTParseError::None, token->line(), token->column() };
                        } else {
                            if(token->type() != TokenType::Dot) {
                                return { nullptr, ASTParseError::PackageMismatch, token->line(), token->column() };
                            } else {
                                consumeCurrentToken();
                                token = nextToken();
                                if(token->type() != TokenType::Identifier) {
                                    return { nullptr, ASTParseError::PackageMismatch, token->line(), token->column() };
                                } else {
                                    names.push_back(*token);
                                    consumeCurrentToken();
                                }
                            }
                        }
                    }
                } else {
                    return { nullptr, ASTParseError::PackageMismatch, token->line(), token->column() };
                }
            }
        }
        return { nullptr, ASTParseError::PackageMismatch, token->line(), token->column() };
    }

    MatchResult ASTBuilder::matchClassDef() {
        ConsumeStateHelper helper(this, true);
        consumeSemicolonEol();
        auto token = nextToken();
        if(token->type() == TokenType::Keyword) {
            if(token->stringLiteral() == lang_keywords::_class) {
                consumeCurrentToken();
                token = nextToken();
                if(token->type() == TokenType::Identifier) {
                    auto className = *token; // get class name
                    consumeCurrentToken();
                    ClassExtends* extends = nullptr; // get extends info
                    if(token->type() == TokenType::Keyword) {
                        if(token->stringLiteral() == lang_keywords::_extends) {
                            consumeCurrentToken();
                            token = nextToken();
                            if(token->type() == TokenType::Identifier) {
                                consumeCurrentToken();
                                extends = new ClassExtends(*token);
                            } else {
                                return { nullptr, ASTParseError::ClassExtendsMismatch, token->line(), token->column() };
                            }
                        }
                    }
                    // match clas body
                    auto body = matchClassBody();
                    if(body) {
                        auto clazz = new Class(className, extends, (MultiExpr*)body.node);
                        return { clazz, ASTParseError::None, className.line(), className.column() };
                    } else {
                        return body;
                    }

                }
            }
        }
        return { nullptr, ASTParseError::ClassMismatch, token->line(), token->column() };
    }

    MatchResult ASTBuilder::matchClassBody() {
        ConsumeStateHelper helper(this, true);
        auto token = nextToken();
        if(token->type() != TokenType::LeftBrace) {
            return { nullptr, ASTParseError::LeftParenExpected, token->line(), token->column() };
        }
        consumeCurrentToken();
        MultiExpr* body = new MultiExpr(VType::ClassBody);
        while(true) {
            consumeSemicolonEol();
            auto var = matchDefVariable();
            if(var) {
                body->addExpr(var.node);
            } else {
                auto fun = matchFunctionDef();
                if(fun) {
                    fun.node->asFunction()->addSelfParam(); // convert to member function
                    body->addExpr(fun.node);
                } else {
                    break;
                }
            }
        }
        token = nextToken();
        if(token->type() != TokenType::RightBrace) {
            return { nullptr, ASTParseError::RightParenExpected, token->line(), token->column() };
        }
        consumeCurrentToken();
        return { body, ASTParseError::None, token->line(), token->column() };
    }

    MatchResult ASTBuilder::matchArray() {
        ConsumeStateHelper helper(this, true);
        consumeSemicolonEol();
        auto token = nextToken();
        if(token->type() != TokenType::LeftBracket) {
            return { nullptr, ASTParseError::LeftParenExpected, token->line(), token->column() };
        }
        consumeCurrentToken();
        MultiExpr* body = new MultiExpr(VType::Vector);
        consumeSemicolonEol();
        auto arrItem = matchExpression();
        if(arrItem) {
            body->addExpr(arrItem.node);
            while(true) {
                consumeSemicolonEol();
                token = nextToken();
                if(token->type() != TokenType::Comma) {
                    break;
                }
                consumeCurrentToken();
                consumeSemicolonEol();
                arrItem = matchExpression();
                if(arrItem) {
                    body->addExpr(arrItem.node);
                } else {
                    break;
                }
            }
        } 
        token = nextToken();
        if(token->type() != TokenType::RightBracket) {
            return { nullptr, ASTParseError::RightParenExpected, token->line(), token->column() };
        }
        consumeCurrentToken();
        return { body, ASTParseError::None, token->line(), token->column() };
    }

    MatchResult ASTBuilder::matchMapItem() {
        ConsumeStateHelper helper(this, true);
        auto token = nextToken();
        if(token->type() != TokenType::Identifier) {
            return { nullptr, ASTParseError::IdentifierExpected, token->line(), token->column() };
        }
        Name name = token->stringLiteral();
        consumeCurrentToken();
        token = nextToken();
        if(token->type() != TokenType::Colon) {
            return { nullptr, ASTParseError::ColonExpected, token->line(), token->column() };
        }
        consumeCurrentToken();
        auto expr = matchExpression();
        if(expr) {
            return { new MapItem(name, expr.node), ASTParseError::None, token->line(), token->column() };
        } else {
            return expr;
        }
    }

    MatchResult ASTBuilder::matchMap() {
        ConsumeStateHelper helper(this, true);
        consumeSemicolonEol();
        auto token = nextToken();
        if(token->type() != TokenType::LeftBrace) {
            return { nullptr, ASTParseError::LeftParenExpected, token->line(), token->column() };
        }
        consumeCurrentToken();
        MultiExpr* body = new MultiExpr(VType::Map);
        consumeSemicolonEol();
        auto item = matchMapItem();
        if(item) {
            body->addExpr(item.node);
            while(true) {
                consumeSemicolonEol();
                token = nextToken();
                if(token->type() != TokenType::Comma) {
                    break;
                }
                consumeCurrentToken();
                consumeSemicolonEol();
                item = matchMapItem();
                if(item) {
                    body->addExpr(item.node);
                } else {
                    break;
                }
            }
        }
        token = nextToken();
        if(token->type() != TokenType::RightBrace) {
            delete body;
            return { nullptr, ASTParseError::RightParenExpected, token->line(), token->column() };
        }
        consumeCurrentToken();
        return { body, ASTParseError::None, token->line(), token->column() };
    }

    Token const* ASTBuilder::nextToken() {
        if(!_cachedTokens.size()) {
            _cachedTokens.push_back(*_tokenParser->nextToken());
        }
        return &_cachedTokens.front();
    }

    // void ASTBuilder::popTokenCache() {
    //     assert(_cachePositions.size());
    //     _consumedTokens.resize(_cachePositions.back());
    //     _cachePositions.pop_back();
    // }

    Token const*  ASTBuilder::consumeAndGetNext() {
        consumeCurrentToken();
        return nextToken();
    }

    void ASTBuilder::consumeCurrentToken() {
        _consumedTokens.push_back(_cachedTokens.front());
        _cachedTokens.pop_front();
    }

    void ASTBuilder::consumeSemicolonEol() {
        auto token = nextToken();
        while(token->type() == TokenType::Semicolon ||token->type() == TokenType::Eol ) {
            consumeAndGetNext();
        }
    }

    // 保存当前的token消费状态，以便匹配失败回溯
    void ASTBuilder::pushConsumeState() {
        _consumedPositions.push_back(_consumedTokens.size());
    }

    // 有些可回溯的BNF生成式，在匹配这些生成式的时候，如果失败，可以回溯，再尝试其它的匹配方式
    void ASTBuilder::resumeConsumeState() {
        auto consumePos = _consumedPositions.back();
        for( size_t i = consumePos; i< _consumedTokens.size(); ++i) {
            _cachedTokens.push_front(_consumedTokens[i]);
        }
        _consumedTokens.resize(consumePos);
        _consumedPositions.pop_back();
    }

    // BNF生成式匹配成功了，但其父生成式如果是可回溯的，则需要调用此方法
    void ASTBuilder::discardConsumeState() {
        _consumedPositions.pop_back();
    }

    // BNF生成式匹配成功了，但其父生成式如果是不可回溯的，则需要调用此方法
    void ASTBuilder::popConsumeState() {
        _consumedTokens.resize(_consumedPositions.back());
        _consumedPositions.pop_back();
    }


    void ASTBuilder::_addDebugInfo(ast::Node const* node, ExprDebugInfo const& info) {
        if(_debugInfoMap) {
            auto& ref = *_debugInfoMap;
            ref[node] = info;
        }
    }

    MatchResult ASTBuilder::buildAST(Env* env, char const* code, DebugInfoMap* debugInfoMap) {
        _debugInfoMap = debugInfoMap;
        _tokenParser = new TokenParser(env);
        _tokenParser->init(code);
        auto rst = matchCodeChunk();
        delete _tokenParser;
        assert(rst);
        return rst;
    }
}