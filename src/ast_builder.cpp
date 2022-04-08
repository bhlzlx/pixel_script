#include "ast_builder.h"
#include "token.h"
#include "token_parser.h"
#include "AST.h"
#include <map>
#include <stack>

namespace compiler {

    ASTNode* matchPrimary(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        if (token->type() == TokenType::Integer) {
            tokenParser->peek();
            auto leaf = new ASTLeaf(ASTNodeType::Integer, *token);
            return leaf;
        } else if (token->type() == TokenType::Identifier) {
            tokenParser->peek();
            auto leaf = new ASTLeaf(ASTNodeType::Identifier, *token);
            return leaf;
        }
        else if (token->type() == TokenType::LeftParen) {
            tokenParser->peek();
            ASTNode* node = matchExpression(tokenParser);
            if(node == nullptr) {
                assert(false && "not permitted");
                return nullptr;
            }
            auto nextToken = tokenParser->nextToken();
            if(nextToken->type() == TokenType::RightParen) {
                tokenParser->peek();
                return node;
            } else {
                return nullptr;
            }
        }
        return nullptr;
    }

    ASTNode* matchFactor(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        if (token->type() == TokenType::Minus) {
            tokenParser->peek();
            auto nextToken = tokenParser->nextToken();
            auto primNode = matchPrimary(tokenParser);
            if(primNode) {
                return new ASTNagativeExpression(primNode);
            }
            return false;
        }
        return matchPrimary(tokenParser);
    }

    ASTNode* matchExpression(TokenParser* tokenParser) {
        std::stack<ASTNode*> factorStack;
        std::stack<TokenType> opStack;
        auto factor = matchFactor(tokenParser);
        if(factor) {
            factorStack.push(factor);
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
                        return rst;
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
                    factorStack.push(factor);
                } else {
                    assert( false && "need a factor");
                    return nullptr;
                }
            }
        }
        return nullptr; // not an expression
    }

    ASTNode* matchBlock(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        if(token->type() == TokenType::LeftBrace) {
            tokenParser->peek();
            auto rst = new ASTMultiExpr(ASTNodeType::Block);
            auto statement = matchStatement(tokenParser);
            if(statement) {
                rst->addExpr(statement);
            }
            while(true) {
                token = tokenParser->nextToken();
                if(token->type() == TokenType::Semicolon || token->type() == TokenType::Eol) {
                    tokenParser->peek();
                } else if(token->type() == TokenType::RightBrace) {
                    tokenParser->peek();
                    return rst;
                } else {
                    statement = matchStatement(tokenParser);
                    if(statement) {
                        rst->addExpr(statement);
                    } else {
                        delete rst;
                        delete statement;
                        assert(false);
                        return nullptr;
                    }
                }
            }
            return rst;
        }
        return nullptr;
    }

    ASTNode* matchStatement(TokenParser* tokenParser) { 
        auto& keywords = tokenParser->keywords();
        auto token = tokenParser->nextToken();
        if(token->stringLiteral() == keywords._if) {
            ASTIfStatement* if_stmt = new ASTIfStatement();
            tokenParser->peek();
            auto expr = matchExpression(tokenParser);
            if(!expr) {
                delete expr;
                return nullptr;
            }
            if_stmt->setCondition(expr);
            auto block = matchBlock(tokenParser);
            if(!block) {
                delete expr;
                delete block;
                return nullptr;
            }
            if_stmt->setThenBranch(block);
            token = tokenParser->nextToken();
            if(token->stringLiteral() == keywords._else) {
                tokenParser->peek();
                auto block = matchBlock(tokenParser);
                if(!block) {
                    delete expr;
                    delete block;
                    return nullptr;
                }
                if_stmt->setElseBranch(block);
            }
            return if_stmt;
        } else if(token->stringLiteral() == keywords._while) {
            tokenParser->peek();
            auto expr = matchExpression(tokenParser);
            if(!expr) {
                delete expr;
                return nullptr;
            }
            auto block = matchBlock(tokenParser);
            if(!block) {
                delete expr;
                delete block;
                return nullptr;
            }
            auto while_stmt = new ASTWhileStatement();
            while_stmt->setBody(block);
            while_stmt->setCondition(expr);
            return while_stmt;
        } else {
            auto expr = matchExpression(tokenParser);
            return expr;
        }
    }

    ASTNode* matchProgram(TokenParser* tokenParser) {
        auto program = new ASTMultiExpr(ASTNodeType::Program);
        while(true) {
            ASTNode* stmt = nullptr;
            auto pos = tokenParser->pos();
            stmt = matchStatement(tokenParser);
            if(stmt) {
                program->addExpr(stmt);
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
        return program;
    }

    ASTNode* matchParamsDef(TokenParser* tokenParser) {
        auto params = new ASTMultiExpr(ASTNodeType::Params);
        auto token = tokenParser->nextToken();
        if(token->type() == TokenType::LeftParen) {
            tokenParser->peek();
            while(true) {
                token = tokenParser->nextToken();
                switch(token->type()) {
                    case TokenType::RightParen: {
                        tokenParser->peek();
                        return params;
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
                                    delete param;
                                    return nullptr;
                                }
                            } else {
                                break;
                            }
                        }
                        token = tokenParser->nextToken();
                        if(token->type() != TokenType::RightParen) {
                            delete param;
                            return nullptr;
                        }
                        return params;
                    }
                }
            }
        }
        return nullptr;
    }
}