#pragma once
#include <cstdint>

namespace compiler {


    class ASTNode;
    class TokenParser;
    class Token;

    enum class ASTParseError {
        None,
        ExprExpected,
        RightParenExpected,
        NeedRightFactor,
        PrimaryMismatch,
        ExprMismatch,
        BlockMismatch,
        ShouldFollowIdentifier,
        ParamsDefMismatch,
    };
    struct MatchResult {
        ASTNode*        node;
        ASTParseError   error;
        int             line;
        int             column;
        operator bool () const {
            return node != nullptr;
        }
    };

    MatchResult matchPrimary(TokenParser* tokenParser);
    MatchResult matchFactor(TokenParser* tokenParser);
    MatchResult matchExpression(TokenParser* tokenParser);
    MatchResult matchBlock(TokenParser* tokenParser);
    MatchResult matchStatement(TokenParser* tokenParser);
    MatchResult matchProgram(TokenParser* tokenParser);

    MatchResult matchParamsDef(TokenParser* tokenParser);
    MatchResult matchFunction(TokenParser* tokenParser);

    class ASTBuilder {
    private:
    };

}