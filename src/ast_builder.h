#pragma once
#include <cstdint>
#include <vector>
#include <deque>
#include "token.h"

namespace compiler {

    namespace ast {
        class Node;
    }

    class Env;
    class TokenParser;

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
        AssignExpected,
        MissVariableName,
        VarMismatch,
        PackageMismatch,
        CodeChunkMismatch,
    };
    struct MatchResult {
        ast::Node*      node;
        ASTParseError   error;
        int             line;
        int             column;
        operator bool () const {
            return node != nullptr;
        }
    };


    class ConsumeStateHelper;
    class ASTBuilder {
        // struct TokenHelper {
        //     size_t cacheSize;
        // };
        friend class ConsumeStateHelper;
    private:
        TokenParser*                    _tokenParser;
        std::vector<Token>              _consumedTokens;
        std::vector<uint32_t>           _consumedPositions;
        std::deque<Token>               _cachedTokens;
        // Token const*                    _token;
    private: // functions
        MatchResult matchPrimary();
        MatchResult matchFactor();
        MatchResult matchExpression();
        MatchResult matchBlock();
        MatchResult matchStatement();
        MatchResult matchCodeChunk();
        MatchResult matchParams();
        MatchResult matchParamList();
        MatchResult matchFunctionDef();
        MatchResult matchClosure();
        MatchResult matchPackage();

        MatchResult matchArgs();
        MatchResult matchPostfix();

        MatchResult matchDefVariable();

        Token const* nextToken();

        Token const*  consumeAndGetNext();

        void consumeCurrentToken();

        void consumeCommaEol();

        void pushConsumeState();
        void popConsumeState();
        void resumeConsumeState();
        void discardConsumeState();
    public:
        ASTBuilder()
            : _tokenParser(nullptr)
            , _consumedTokens()
            , _consumedPositions()
            , _cachedTokens()
        {
        }

        MatchResult buildAST(Env* env, char const* code);

    };

}