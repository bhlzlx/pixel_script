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
        consumeCommaEol();
        auto token = nextToken();
        auto startToken = *token;
        Node* operand = nullptr;
        switch(token->type()) {
            case TokenType::Integer: {
                consumeCurrentToken();
                operand = new ASTLeaf(VType::Int, *token);
                return { operand, ASTParseError::None, token->line(), token->column() };
            }
            case TokenType::String: {
                consumeCurrentToken();
                operand = new ASTLeaf(VType::String, *token);
                return { operand, ASTParseError::None, token->line(), token->column() };
            }
            case TokenType::Keyword: { // 目前关键字只有func
                auto closure = matchClosure();
                if(closure) {
                    return closure;
                } else {
                    return { nullptr, ASTParseError::FunctionMismatch, token->line(), token->column() };
                }
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
                ASTIdentifier* id = new ASTIdentifier(*token);
                operand = id;
                break;
            }
            default: {
                return { nullptr, ASTParseError::PrimaryMismatch, token->line(), token->column() };
            }
        }
        // 特定的匹配才有operand一说
        if(!operand) {
            return { nullptr, ASTParseError::PrimaryMismatch, startToken.line(), startToken.column() };
        } else {
            ASTMultiExpr* postfix = nullptr;
            // 这里添加了函数调用匹配
            // a + b(1) 这种
            auto expr = matchPostfix();
            // if postfix is not nullptr, it means there is only a operand avail, so we just return the operand
            if(!expr) {
                return { operand, ASTParseError::None, startToken.line(), startToken.column()};
            } else {
                if(expr.node) {
                    postfix = expr.node->asMultiExpr();
                }
                ASTPrimary* prim = new ASTPrimary(operand, postfix);
                return  { prim, ASTParseError::None, startToken.line(), startToken.column() };
            }
        }
    }

    MatchResult ASTBuilder::matchFactor() {
        ConsumeStateHelper helper(this, true);
        MatchResult rst = {};
        auto token = nextToken();
        auto startToken = *token;
        if (token->type() == TokenType::Minus) {
            consumeCurrentToken();
            auto primNode = matchPrimary();
            if(primNode) {
                rst = { 
                    new ASTNegativeExpression(primNode.node, *token), 
                    ASTParseError::None,
                    startToken.line(),
                    startToken.column()
                };
            } else {
                rst = primNode; // failed
            }
        } else {
            rst =  matchPrimary();
        }
        return rst;
    }

    MatchResult ASTBuilder::matchExpression() {
        ConsumeStateHelper helper(this, true);
        MatchResult rst = {};
        auto startToken = *nextToken();
        std::stack<Node*> factorStack;
        std::stack<TokenType> opStack;
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
                            auto binExpr = new ASTBinaryOpExpr(left, right, op);
                            factorStack.push(binExpr);
                        }
                        auto rst = factorStack.top();
                        assert(factorStack.size() == 1);
                        return { rst, ASTParseError::None, startToken.line(), startToken.column() };
                }
                consumeCurrentToken();
                TokenType op = token->type();
                if(opStack.size()) {
                    if(op>=opStack.top()) {
                        auto factor1 = factorStack.top(); factorStack.pop();
                        auto factor2 = factorStack.top(); factorStack.pop();
                        auto binExpr = new ASTBinaryOpExpr(factor1, factor2, opStack.top());
                        opStack.pop();
                        factorStack.push(binExpr);
                    }
                }
                opStack.push(op);
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
            auto exprs = new ASTMultiExpr(VType::Block);
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
        if(token->stringLiteral() == keywords::_if) {
            auto IfToken = *token;
            ASTIfStatement* if_stmt = new ASTIfStatement();
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
                    if(token->stringLiteral() == keywords::_else) {
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
        } else if(token->stringLiteral() == keywords::_while) {
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
                    auto while_stmt = new ASTWhileStatement();
                    while_stmt->setBody(block.node);
                    while_stmt->setCondition(expr.node);
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
        auto chunk = new ASTMultiExpr(VType::Module);
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
     * @return MatchResult ASTStringList
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
            // auto params = new ASTMultiExpr(VType::Params);
            // auto params = new ASTStringList()
            auto param = new ASTLeaf(VType::Id, *token);
            // params->addExpr(param);
            while(true) {
                token = nextToken();
                if(token->type() == TokenType::Comma) {
                    consumeCurrentToken();
                    token = nextToken();
                    if(token->type() != TokenType::Identifier) {
                        return MatchResult { nullptr, ASTParseError::ShouldFollowIdentifier, token->line(), token->column() };
                    } else {
                        tokens.push_back(*token);
                        consumeCurrentToken();
                    }
                } else {
                    break;
                }
            }
            return { new ASTStringList(tokens, VType::Params), ASTParseError::None, startToken.line(), startToken.column() };
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
        consumeCommaEol();
        ConsumeStateHelper helper(this, true);
        ASTFunction* func = nullptr;
        auto token = nextToken();
        Token startToken = *token;
        if(token->stringLiteral() != keywords::_func) { // "func"
            return MatchResult { nullptr, ASTParseError::FunctionMismatch, token->line(), token->column() };
        } else {
            consumeCurrentToken();
            token = nextToken();
            if(token->type() != TokenType::Identifier) { // ident
                return MatchResult { nullptr, ASTParseError::MissFunctionName, token->line(), token->column() };
            } else {
                consumeCurrentToken();
                auto func = new ASTFunction(VType::None);
                func->setName(*token);
                auto paramList = matchParamList(); // paramlist
                if(paramList) {
                    func->setParams(paramList.node->asStringList());
                } else {
                    func->setParams(new ASTStringList(VType::Params));
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
            auto multiExpr = new ASTMultiExpr(VType::Args);
            multiExpr->addExpr(expr.node);
            while(true) {
                auto token = nextToken();
                if(token->type() == TokenType::Comma) {
                    consumeCurrentToken();
                    expr = matchExpression();
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
    MatchResult ASTBuilder::matchPostfix() {
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

    MatchResult ASTBuilder::matchClosure() {
        ConsumeStateHelper helper(this, true);
        auto token = nextToken();
        MatchResult rst = MatchResult { nullptr, ASTParseError::ClosureMismatch, token->line(), token->column() };
        Token startToken = *token;
        Node* params = nullptr;
        Node* body = nullptr;
        if(token->type() == TokenType::Keyword) {
            if(token->stringLiteral() == keywords::_func) {
                consumeCurrentToken();
                params = matchParamList().node;
                body = matchBlock().node;
                if(!body) {
                    delete params;
                } else {
                    auto closure = new ASTFunction(VType::Closure);
                    closure->setParams(params->asStringList());
                    closure->setBody(body);
                    return { closure, ASTParseError::None, startToken.line(), startToken.column() };
                }
            }
        }
        return rst;
    }

    MatchResult ASTBuilder::matchDefVariable() {
        consumeCommaEol();
        ConsumeStateHelper helper(this, true);
        auto token = nextToken();
        if(token->type() == TokenType::Keyword) {
            if(token->stringLiteral() == keywords::_var) {
                consumeCurrentToken();
                token = nextToken();
                if(token->type() == TokenType::Identifier) {
                    Token name = *token;
                    ASTIdentifier* id = new ASTIdentifier(name);
                    consumeCurrentToken();
                    token = nextToken();
                    if(token->type() != TokenType::Assign) {
                        if(token->type() == TokenType::Semicolon || token->type() == TokenType::Eol) {
                            consumeCurrentToken();
                            return { new ASTVariable(id, nullptr), ASTParseError::None, name.line(), name.column() };
                        }
                        return { nullptr, ASTParseError::AssignExpected, name.line(), name.column() };
                    } else {
                        consumeCurrentToken();
                        auto expr = matchExpression();
                        if(!expr) {
                            return expr;
                        } else { // 初始化实际是创建的一个function，因为方便后续处理，代码重用，算是一个小trick
                            // ASTFunction* valueExprFunc = new ASTFunction(VType::Closure);
                            // ASTMultiExpr* funcBody = new ASTMultiExpr(VType::Block);
                            // funcBody->addExpr(expr.node);
                            // valueExprFunc->setBody(funcBody);
                            return { new ASTVariable(id, expr.node), ASTParseError::None, name.line(), name.column() };
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
        consumeCommaEol();
        auto token = nextToken();
        if(token->type() == TokenType::Keyword) {
            if(token->stringLiteral() == keywords::_package) {
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
                            return { new ASTStringList(names, VType::Package), ASTParseError::None, token->line(), token->column() };
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

    void ASTBuilder::consumeCommaEol() {
        auto token = nextToken();
        while(token->type() == TokenType::Comma ||token->type() == TokenType::Eol ) {
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

    MatchResult ASTBuilder::buildAST(Env* env, char const* code) {
        _tokenParser = new TokenParser(env);
        _tokenParser->init(code);
        auto rst = matchCodeChunk();
        delete _tokenParser;
        assert(rst);
        return rst;
    }
}