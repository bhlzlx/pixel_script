#pragma once

namespace compiler {

    class ASTNode;
    class TokenParser;
    class Token;

    ASTNode* matchPrimary(TokenParser* tokenParser);
    ASTNode* matchFactor(TokenParser* tokenParser);
    ASTNode* matchExpression(TokenParser* tokenParser);
    ASTNode* matchBlock(TokenParser* tokenParser);
    ASTNode* matchStatement(TokenParser* tokenParser);
    ASTNode* matchProgram(TokenParser* tokenParser);

    ASTNode* matchParamsDef(TokenParser* tokenParser);
    ASTNode* matchFunction(TokenParser* tokenParser);

    class ASTBuilder {
    private:
    };

}