#pragma once
#include <cstdint>

namespace compiler {


    class ASTNode;
    class TokenParser;
    class Token;

    enum class ASTParseError {
        None,
        ExprExpected,
        LeftParenExpected,
        RightParenExpected,
        NeedRightFactor,
        PrimaryMismatch,
        ExprMismatch,
        BlockMismatch,
        ShouldFollowIdentifier,
        ParamsListMismatch,
        FunctionMismatch,
        MissFunctionName,
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

    MatchResult matchParams(TokenParser* tokenParser);
    MatchResult matchParamList(TokenParser* tokenParser);
    MatchResult matchFunctionDef(TokenParser* tokenParser);

    MatchResult matchArgs(TokenParser* tokenParser);
    MatchResult matchPostfix(TokenParser* tokenParser);

    class ASTBuilder {
    private:
    };

}