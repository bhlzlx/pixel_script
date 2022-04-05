#include "ASTBuilder.h"
#include "token.h"
#include "token_parser.h"
#include "AST.h"

namespace compiler {

    ASTNode* matchPrimary(Token const* token, TokenParser* tokenParser) {
        if (token->type() == TokenType::Integer || token->type() == TokenType::Identifier) {
            return new ASTLeaf(token);
        }
        else if (token->type() == TokenType::LeftParen) {
            ASTNode* node = matchExpression(token, tokenParser);
            auto nextToken = tokenParser->nextToken();
            if(nextToken->type() == TokenType::RightParen) {
                return node;
            } else {
                return nullptr;
            }
        }
        assert(false && "match primary failed!");
        return nullptr;
    }

    ASTNode* matchFactor(Token const* token, TokenParser* tokenParser) {
        if (token->type() == TokenType::Minus) {
            // auto rst = new ASTNagativeExpression()
            auto nextToken = tokenParser->nextToken();
            auto primNode = matchPrimary(nextToken, tokenParser);
            if(primNode) {
                return new ASTNagativeExpression(primNode);
            }
            return false;
        }
        return matchPrimary(token, tokenParser);
    }

}