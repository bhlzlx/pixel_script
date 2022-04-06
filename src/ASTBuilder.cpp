#include "ASTBuilder.h"
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
            ASTNode* node = matchExpression(token, tokenParser);
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
            exprNode = new ASTBinaryExpression(factor1, factor2);
        } else {
            delete factor1;
        }
        return exprNode; 
    }

}