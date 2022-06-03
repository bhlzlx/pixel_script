#pragma once
#include <cstdint>
#include <vector>
#include <deque>
#include "token.h"
#include "compiler_common.h"

namespace compiler {

    namespace ast {
        class Node;
    }

    class Env;
    class TokenParser;


    class ConsumeStateHelper;
    class ASTBuilder {
        friend class ConsumeStateHelper;
    private:
        TokenParser*                    _tokenParser;
        std::vector<CodeDebugInfo>      _debugInfos;
        // 
        Token const*                    _cachedToken;
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
        MatchResult matchArgList();
        MatchResult matchDotAccess();

        MatchResult matchDefVariable();

        MatchResult matchClassDef();
        MatchResult matchExtends();
        MatchResult matchClassBody();

        MatchResult matchArray();
        MatchResult matchIndexAccess();

        MatchResult matchMapItem();
        MatchResult matchMap();
        // MatchResult matchMemberDef();

        Token const* nextToken();

        Token const*  consumeAndGetNext();

        void consumeCurrentToken();

        void consumeSemicolonEol();

        void pushConsumeState();
        void popConsumeState();
        void resumeConsumeState();
        void discardConsumeState();

        void _addDebugInfo(ast::Node* node, CodeDebugInfo const& info);
    public:
        ASTBuilder()
            : _tokenParser(nullptr)
            , _cachedToken()
        {
        }

        MatchResult buildAST(Env* env, char const* code);

        std::vector<CodeDebugInfo>& dbgInfo() { return _debugInfos; }

    };

}