#pragma once
#include <cstdint>
#include <vector>

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
        ClosureMismatch,
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


    class ASTBuilder {
    private:
        TokenParser*        _tokenParser;
        std::vector<Token>  _cachedTokens;
    private: // functions
        MatchResult matchPrimary();
        MatchResult matchFactor();
        MatchResult matchExpression();
        MatchResult matchBlock();
        MatchResult matchStatement();
        MatchResult matchProgram();
        MatchResult matchParams();
        MatchResult matchParamList();
        MatchResult matchFunctionDef();
        MatchResult matchClosure();

        MatchResult matchArgs(TokenParser* tokenParser);
        MatchResult matchPostfix(TokenParser* tokenParser);
    public:
        ASTBuilder()
            : _tokenParser(nullptr)
        {
        }

        MatchResult buildAST( char const* code);
    };

}