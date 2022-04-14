#include "ast_builder.h"
#include "token.h"
#include "token_parser.h"
#include "AST.h"
#include <map>
#include <stack>

namespace compiler {

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
        _tokenParser->peekCommaEol();
        auto token = _tokenParser->nextToken();
        auto startToken = *token;
        ASTNode* operand = nullptr;
        switch(token->type()) {
            case TokenType::Integer: {
                _tokenParser->peek();
                operand = new ASTLeaf(ASTNodeType::Integer, *token);
                return { operand, ASTParseError::None, token->line(), token->column() };
            }
            case TokenType::String: {
                _tokenParser->peek();
                operand = new ASTLeaf(ASTNodeType::String, *token);
                return { operand, ASTParseError::None, token->line(), token->column() };
            }
            case TokenType::Keyword: { // 目前关键字只有func
                auto rst = matchClosure();
                if(rst) {
                    return rst;
                }
                return { nullptr, ASTParseError::FunctionMismatch, token->line(), token->column() };
            }
            case TokenType::LeftParen: {
                _tokenParser->peek();
                auto expr = matchExpression();
                if(!expr) {
                    return expr; // match expression failed
                }
                auto nextToken = _tokenParser->nextToken();
                if(nextToken->type() == TokenType::RightParen) {
                    _tokenParser->peek();
                    operand = expr.node;
                } else {
                    delete expr.node;
                    MatchResult rst = { nullptr, ASTParseError::RightParenExpected, nextToken->line(), nextToken->column() };
                    return rst;
                }
                break;
            }
            case TokenType::Identifier: {
                _tokenParser->peek();
                operand = new ASTLeaf(ASTNodeType::Identifier, *token);
                break;
            }
            default: {
                MatchResult rst = { nullptr, ASTParseError::PrimaryMismatch, token->line(), token->column() };
                return rst;
            }
        }
        if(!operand) {
            return { nullptr, ASTParseError::PrimaryMismatch, startToken.line(), startToken.column() };
        }
        ASTNode* postfix = nullptr;
        auto expr = matchPostfix();
        // if postfix is not nullptr, it means there is only a operand avail, so we just return the operand
        if(!expr) {
            return { operand, ASTParseError::None, startToken.line(), startToken.column()};
        } else {
            postfix = expr.node;
            ASTPrimary* rst = new ASTPrimary(operand, postfix);
            return { rst, ASTParseError::None, startToken.line(), startToken.column() };
        }
    }

    MatchResult ASTBuilder::matchFactor() {
        auto token = _tokenParser->nextToken();
        auto startToken = *token;
        if (token->type() == TokenType::Minus) {
            _tokenParser->peek();
            auto primNode = matchPrimary();
            if(primNode) {
                MatchResult rst =  { 
                    new ASTNegativeExpression(primNode.node), 
                    ASTParseError::None,
                    startToken.line(),
                    startToken.column()
                };
                return rst;
            } else {
                return primNode; // failed
            }
        }
        return matchPrimary();
    }

    MatchResult ASTBuilder::matchExpression() {
        auto startToken = *_tokenParser->nextToken();
        std::stack<ASTNode*> factorStack;
        std::stack<TokenType> opStack;
        auto factor = matchFactor();
        if(factor) {
            factorStack.push(factor.node);
            while(true) {
                Token const* nextToken = _tokenParser->nextToken();
                switch(nextToken->type()) {
                    case TokenType::Minus:
                    case TokenType::Slash:
                    case TokenType::Plus:
                    case TokenType::Star:
                    case TokenType::Equal:
                    case TokenType::Modulus:
                    case TokenType::Less:
                    case TokenType::LessEqual:
                    case TokenType::Greater:
                    case TokenType::GreaterEqual:
                    case TokenType::EqualEqual:
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
                _tokenParser->peek();
                TokenType op = nextToken->type();
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
                    auto lastToken = _tokenParser->nextToken();
                    MatchResult rst = { nullptr, ASTParseError::NeedRightFactor, lastToken->line(), lastToken->column() };
                    return rst;
                }
            }
        }
        return factor;
    }

    MatchResult ASTBuilder::matchBlock() {
        auto token = _tokenParser->nextToken();
        if(token->type() == TokenType::LeftBrace) {
            _tokenParser->peek();
            auto rst = new ASTMultiExpr(ASTNodeType::Block);
            auto statement = matchStatement();
            if(statement) {
                rst->addExpr(statement.node);
            }
            while(true) {
                token = _tokenParser->nextToken();
                if(token->type() == TokenType::Semicolon || token->type() == TokenType::Eol) {
                    _tokenParser->peek();
                } else if(token->type() == TokenType::RightBrace) {
                    _tokenParser->peek();
                    return { rst, ASTParseError::None, token->line(), token->column() };
                } else {
                    statement = matchStatement();
                    if(statement) {
                        rst->addExpr(statement.node);
                    } else {
                        delete rst;
                        return statement;
                    }
                }
            }
            return { rst, ASTParseError::None, token->line(), token->column() };
        }
        return { nullptr, ASTParseError::BlockMismatch, token->line(), token->column() };
    }

    MatchResult ASTBuilder::matchStatement() { 
        auto& keywords = _tokenParser->keywords();
        auto token = _tokenParser->nextToken();
        if(token->stringLiteral() == keywords._if) {
            auto IfToken = *token;
            ASTIfStatement* if_stmt = new ASTIfStatement();
            _tokenParser->peek();
            auto expr = matchExpression();
            if(!expr) {
                return expr;
            }
            if_stmt->setCondition(expr.node);
            auto block = matchBlock();
            if(!block) {
                delete if_stmt;
                return block;
            }
            if_stmt->setThenBranch(block.node);
            token = _tokenParser->nextToken();
            if(token->stringLiteral() == keywords._else) {
                _tokenParser->peek();
                auto block = matchBlock();
                if(!block) {
                    delete if_stmt;
                    return block;
                }
                if_stmt->setElseBranch(block.node);
            }
            return { if_stmt, ASTParseError::None, IfToken.line(), IfToken.column() };
        } else if(token->stringLiteral() == keywords._while) {
            auto WhileToken = *token;
            _tokenParser->peek();
            auto expr = matchExpression();
            if(!expr) {
                return expr;
            }
            auto block = matchBlock();
            if(!block) {
                delete expr.node;
                return block;
            }
            auto while_stmt = new ASTWhileStatement();
            while_stmt->setBody(block.node);
            while_stmt->setCondition(expr.node);
            return { while_stmt, ASTParseError::None, WhileToken.line(), WhileToken.column() };
        } else {
            auto expr = matchExpression();
            return expr;
        }
    }

    MatchResult ASTBuilder::matchProgram() {
        auto nextToken = _tokenParser->nextToken();
        while(nextToken->type() == TokenType::Eol || nextToken->type() == TokenType::Comma) {
            _tokenParser->peek();
            nextToken = _tokenParser->nextToken();
        }
        auto program = new ASTMultiExpr(ASTNodeType::Program);
        while(nextToken) {
            auto pos = _tokenParser->pos();
            auto stmt = matchFunctionDef();
            if(!stmt) {
                stmt = matchStatement();
            }
            if(stmt) {
                program->addExpr(stmt.node);
            } else {
                if(nextToken->type() == TokenType::Eof) {
                    break;
                }
            }
            _tokenParser->peekCommaEol();
            nextToken = _tokenParser->nextToken();
        }
        return { program, ASTParseError::None, 0, 0 };
    }

    /**
     * @brief 
     *  bnf : params : param {, param}
     * @param _tokenParser 
     * @return MatchResult 
     */
    MatchResult matchParams(_tokenParser* _tokenParser) {
        auto token = _tokenParser->nextToken();
        Token startToken = *token;
        if(token->type() != TokenType::Identifier) {
            return MatchResult { nullptr, ASTParseError::ParamsListMismatch, token->line(), token->column() };
        }
        _tokenParser->peek();
        auto params = new ASTMultiExpr(ASTNodeType::Params);
        auto param = new ASTLeaf(ASTNodeType::Identifier, *token);
        params->addExpr(param);
        while(true) {
            token = _tokenParser->nextToken();
            if(token->type() == TokenType::Comma) {
                _tokenParser->peek();
                token = _tokenParser->nextToken();
                if(token->type() != TokenType::Identifier) {
                    delete params;
                    return MatchResult { nullptr, ASTParseError::ShouldFollowIdentifier, token->line(), token->column() };
                }
                param = new ASTLeaf(ASTNodeType::Identifier, *token);
                _tokenParser->peek();
                params->addExpr(param);
            } else {
                break;
            }
        }
        return { params, ASTParseError::None, startToken.line(), startToken.column() };
    }

    /**
     * @brief 
     *  bnf : paramlist : "(" params ")"
     * @param _tokenParser 
     * @return MatchResult 
     *  if no params, return nullptr
     */
    MatchResult ASTBuilder::matchParamList() {
        auto currToken = _tokenParser->nextToken();
        auto startToken = *currToken;
        if(currToken->type() == TokenType::LeftParen) {
            _tokenParser->peek();
            auto params = matchParams(); // maybe no params
            currToken = _tokenParser->nextToken();
            if(currToken->type() == TokenType::RightParen) {
                _tokenParser->peek();
                params.error = ASTParseError::None; // if has error, set as no error!
                return params; // success, maybe no params
            } else {
                delete params.node; // clean up
                return { nullptr, ASTParseError::RightParenExpected, currToken->line(), currToken->column() }; // failed
            }
        }
        return {nullptr, ASTParseError::ParamsListMismatch, startToken.line(), startToken.column()}; // failed
    }

    /**
     * @brief 
     * bnf : function : "func" ident "(" paramlist ")" block
     * 
     * @param _tokenParser 
     * @return MatchResult 
     */
    MatchResult ASTBuilder::matchFunctionDef() {
        auto const& keywords = _tokenParser->keywords();
        auto token = _tokenParser->nextToken();
        Token startToken = *token;
        if(token->stringLiteral() != keywords._func) { // "func"
            return MatchResult { nullptr, ASTParseError::FunctionMismatch, token->line(), token->column() };
        }
        _tokenParser->peek();
        token = _tokenParser->nextToken();
        if(token->type() != TokenType::Identifier) { // ident
            return MatchResult { nullptr, ASTParseError::MissFunctionName, token->line(), token->column() };
        }
        _tokenParser->peek();
        auto func = new ASTFunction();
        func->setName(token->stringLiteral());
        auto paramList = matchParamList(); // paramlist
        if(!paramList) {
            delete func;
            return paramList; // failed
        }
        func->setParams(paramList.node);
        auto block = matchBlock(); // body
        if(!block) {
            delete func;
            return block; // failed
        }
        func->setBody(block.node);
        return { func, ASTParseError::None, startToken.line(), startToken.column() };
    }


    /**
     * @brief 
     * bnf : expr {, expr}
     * 
     * @param _tokenParser 
     * @return MatchResult 
     */
    MatchResult ASTBuilder::matchArgs() {
        auto beginToken = *_tokenParser->nextToken();
        auto multiExpr = new ASTMultiExpr(ASTNodeType::Args);
        auto expr = matchExpression();
        if(!expr) {
            return expr;
        }
        multiExpr->addExpr(expr.node);
        while(true) {
            auto token = _tokenParser->nextToken();
            if(token->type() == TokenType::Comma) {
                _tokenParser->peek();
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
    
    /**
     * @brief 
     *  bnf : "(" [args] ")"
     * 
     * @param _tokenParser 
     * @return MatchResult 
     */
    MatchResult ASTBuilder::matchPostfix() {
        auto token = _tokenParser->nextToken();
        Token startToken = *token;
        if(token->type() != TokenType::LeftParen) {
            return MatchResult { nullptr, ASTParseError::LeftParenExpected, token->line(), token->column() };
        }
        _tokenParser->peek();
        auto args = matchArgs();
        token = _tokenParser->nextToken();
        if(token->type() != TokenType::RightParen) {
            delete args.node;
            return MatchResult { nullptr, ASTParseError::RightParenExpected, token->line(), token->column() };
        }
        _tokenParser->peek();
        return { args.node, ASTParseError::None, startToken.line(), startToken.column() };
    }

    MatchResult ASTBuilder::matchClosure() {
        auto token = _tokenParser->nextToken();
        Token startToken = *token;
        ASTNode* args = nullptr;
        ASTNode* body = nullptr;
        if(token->type() == TokenType::Keyword) {
            if(token->stringLiteral() == _tokenParser->keywords()._func) {
                _tokenParser->peek();
                args = matchPostfix().node;
                body = matchBlock().node;
                if(!body) {
                    delete args;
                }
                return { new ASTDoubleStructure(ASTNodeType::Closure, args, body), ASTParseError::None, startToken.line(), startToken.column() };
            }
        }
        return MatchResult { nullptr, ASTParseError::ClosureMismatch, token->line(), token->column() };
    }
}