#pragma once

namespace compiler {

    class ASTNode;
    class TokenParser;
    class Token;

    ASTNode* matchPrimary(Token const* token, TokenParser* tokenParser);
    ASTNode* matchFactor(Token const* token, TokenParser* tokenParser);
    ASTNode* matchExpression( Token const* token, TokenParser* tokenParser);

    class ASTBuilder {
    private:
    };

}