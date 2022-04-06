#include "ast_builder.h"
#include "token.h"
#include "token_parser.h"
#include "AST.h"

namespace compiler {

    ASTNode* matchPrimary(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        if (token->type() == TokenType::Integer || token->type() == TokenType::Identifier) {
            tokenParser->peek();
            return new ASTLeaf(token);
        }
        else if (token->type() == TokenType::LeftParen) {
            tokenParser->peek();
            ASTNode* node = matchExpression(tokenParser);
            auto nextToken = tokenParser->nextToken();
            if(nextToken->type() == TokenType::RightParen) {
                tokenParser->peek();
                return node;
            } else {
                return nullptr;
            }
        }
        assert(false && "match primary failed!");
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
        ASTNode* factor1 = matchFactor(tokenParser);
        ASTNode* factor2 = nullptr;
        ASTNode* exprNode = nullptr;
        Token const* nextToken = tokenParser->nextToken();
        switch(nextToken->type()) {
            case TokenType::Minus:
            case TokenType::Plus:
            case TokenType::Slash:
            case TokenType::Star:
            case TokenType::Modulus:
            default:
                return factor1;
        }
        tokenParser->peek();
        factor2 = matchFactor(tokenParser);
        if(factor2) {
            exprNode = new ASTBinaryOpExpr(factor1, factor2);
        } else {
            delete factor1;
        }
        return exprNode; 
    }

    ASTNode* matchBlock(TokenParser* tokenParser) {
        auto token = tokenParser->nextToken();
        if(token->type() == TokenType::LeftBrace) {
            tokenParser->peek();
            auto rst = new ASTBlock();
            auto statement = matchStatement(tokenParser);
            if(statement) {
                rst->addSubNode(statement);
            }
            while(true) {
                token = tokenParser->nextToken();
                if(token->type() == TokenType::Semicolon) {
                    tokenParser->peek();
                } else if(token->type() == TokenType::RightBrace) {
                    tokenParser->peek();
                    return rst;
                } else {
                    statement = matchStatement(tokenParser);
                    if(statement) {
                        rst->addSubNode(statement);
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
        auto stmt = matchStatement(tokenParser);
        if(stmt) {
            auto program = new ASTProgram(stmt);
            return program;
        }
        return nullptr;
    }
}