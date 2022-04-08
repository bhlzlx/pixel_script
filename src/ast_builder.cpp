#include "ast_builder.h"
#include "token.h"
#include "token_parser.h"
#include "AST.h"
#include <map>
#include <stack>

namespace compiler {

    MatchResult matchPrimary(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        if (token->type() == TokenType::Integer) {
            tokenParser->peek();
            auto leaf = new ASTLeaf(ASTNodeType::Integer, *token);
            MatchResult rst = { leaf, ASTParseError::None, token->line(), token->column() };
            return rst;
        } else if (token->type() == TokenType::Identifier) {
            tokenParser->peek();
            auto leaf = new ASTLeaf(ASTNodeType::Identifier, *token);
            MatchResult rst = { leaf, ASTParseError::None, token->line(), token->column() };
            return rst;
        }
        else if (token->type() == TokenType::LeftParen) {
            tokenParser->peek();
            auto expr = matchExpression(tokenParser);
            if(!expr) {
                return expr; // match expression failed
            }
            auto nextToken = tokenParser->nextToken();
            if(nextToken->type() == TokenType::RightParen) {
                tokenParser->peek();
                return expr;
            } else {
                MatchResult rst = { nullptr, ASTParseError::RightParenExpected, nextToken->line(), nextToken->column() };
                return rst;
            }
        }
        return {nullptr, ASTParseError::PrimaryMismatch, token->line(), token->column()};
    }

    MatchResult matchFactor(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        if (token->type() == TokenType::Minus) {
            tokenParser->peek();
            auto nextToken = tokenParser->nextToken();
            auto primNode = matchPrimary(tokenParser);
            if(primNode) {
                MatchResult rst =  { 
                    new ASTNegativeExpression(primNode.node), 
                    ASTParseError::None,
                    token->line(),
                    token->column()
                };
                return rst;
            } else {
                return primNode;
            }
        }
        return matchPrimary(tokenParser);
    }

    MatchResult matchExpression(TokenParser* tokenParser) {
        std::stack<ASTNode*> factorStack;
        std::stack<TokenType> opStack;
        auto factor = matchFactor(tokenParser);
        if(factor) {
            factorStack.push(factor.node);
            while(true) {
                Token const* nextToken = tokenParser->nextToken();
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
                        return { rst, ASTParseError::None, nextToken->line(), nextToken->column() };
                }
                tokenParser->peek();
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
                factor = matchFactor(tokenParser);
                if(factor) {
                    factorStack.push(factor.node);
                } else { 
                    // clean up status & throw the exception
                    while(!factorStack.empty()) {
                        auto factor = factorStack.top();
                        delete factor;
                        factorStack.pop();
                    }
                    auto lastToken = tokenParser->nextToken();
                    MatchResult rst = { nullptr, ASTParseError::NeedRightFactor, lastToken->line(), lastToken->column() };
                    return rst;
                }
            }
        }
        return factor;
    }

    MatchResult matchBlock(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        if(token->type() == TokenType::LeftBrace) {
            tokenParser->peek();
            auto rst = new ASTMultiExpr(ASTNodeType::Block);
            auto statement = matchStatement(tokenParser);
            if(statement) {
                rst->addExpr(statement.node);
            }
            while(true) {
                token = tokenParser->nextToken();
                if(token->type() == TokenType::Semicolon || token->type() == TokenType::Eol) {
                    tokenParser->peek();
                } else if(token->type() == TokenType::RightBrace) {
                    tokenParser->peek();
                    return { rst, ASTParseError::None, token->line(), token->column() };
                } else {
                    statement = matchStatement(tokenParser);
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

    MatchResult matchStatement(TokenParser* tokenParser) { 
        auto& keywords = tokenParser->keywords();
        auto token = tokenParser->nextToken();
        if(token->stringLiteral() == keywords._if) {
            auto IfToken = *token;
            ASTIfStatement* if_stmt = new ASTIfStatement();
            tokenParser->peek();
            auto expr = matchExpression(tokenParser);
            if(!expr) {
                return expr;
            }
            if_stmt->setCondition(expr.node);
            auto block = matchBlock(tokenParser);
            if(!block) {
                delete if_stmt;
                return block;
            }
            if_stmt->setThenBranch(block.node);
            token = tokenParser->nextToken();
            if(token->stringLiteral() == keywords._else) {
                tokenParser->peek();
                auto block = matchBlock(tokenParser);
                if(!block) {
                    delete if_stmt;
                    return block;
                }
                if_stmt->setElseBranch(block.node);
            }
            return { if_stmt, ASTParseError::None, IfToken.line(), IfToken.column() };
        } else if(token->stringLiteral() == keywords._while) {
            auto WhileToken = *token;
            tokenParser->peek();
            auto expr = matchExpression(tokenParser);
            if(!expr) {
                return expr;
            }
            auto block = matchBlock(tokenParser);
            if(!block) {
                delete expr.node;
                return block;
            }
            auto while_stmt = new ASTWhileStatement();
            while_stmt->setBody(block.node);
            while_stmt->setCondition(expr.node);
            return { while_stmt, ASTParseError::None, WhileToken.line(), WhileToken.column() };
        } else {
            auto expr = matchExpression(tokenParser);
            return expr;
        }
    }

    MatchResult matchProgram(TokenParser* tokenParser) {
        auto program = new ASTMultiExpr(ASTNodeType::Program);
        while(true) {
            auto pos = tokenParser->pos();
            auto stmt = matchStatement(tokenParser);
            if(stmt) {
                program->addExpr(stmt.node);
            } else {
                tokenParser->peek();
                tokenParser->nextToken();
            }
            if(pos != tokenParser->pos()) {
                continue;
            } else {
                break;
            }
        }
        return { program, ASTParseError::None, 0, 0 };
    }

    MatchResult matchParamsDef(TokenParser* tokenParser) {
        auto params = new ASTMultiExpr(ASTNodeType::Params);
        auto token = tokenParser->nextToken();
        if(token->type() == TokenType::LeftParen) {
            tokenParser->peek();
            while(true) {
                token = tokenParser->nextToken();
                switch(token->type()) {
                    case TokenType::RightParen: {
                        tokenParser->peek();
                        return { params, ASTParseError::None, token->line(), token->column() };
                    }
                    case TokenType::Identifier: {
                        auto param = new ASTLeaf(ASTNodeType::Param, *token);
                        tokenParser->peek();
                        params->addExpr(param);
                        while(true) {
                            token = tokenParser->nextToken();
                            if(token->type() == TokenType::Comma) {
                                tokenParser->peek();
                                token = tokenParser->nextToken();
                                if(token->type() == TokenType::Identifier) {
                                    auto param = new ASTLeaf(ASTNodeType::Param, *token);
                                    tokenParser->peek();
                                    params->addExpr(param);
                                    break;
                                } else {
                                    delete params;
                                    return { nullptr, ASTParseError::ShouldFollowIdentifier, token->line(), token->column() };
                                }
                            } else {
                                break;
                            }
                        }
                        token = tokenParser->nextToken();
                        if(token->type() != TokenType::RightParen) {
                            delete params;
                            return { nullptr, ASTParseError::RightParenExpected, token->line(), token->column() };
                        }
                        return { params, ASTParseError::None, token->line(), token->column() };
                    }
                }
            }
        }
        return {nullptr, ASTParseError::ParamsDefMismatch, token->line(), token->column()};
    }
}