#pragma once
#include <queue>
#include <string>
#include "name_pool.h"

namespace compiler {

    class Token;

    class TokenParser {
        enum class State {
            None, // no state
            Identifier, // keyword/var
            Number, // pure number
            Brackets, // { } [ ] ()
            Op, // + - * / += *= /= ++ --
            Semicolon, // ;
        };
    private:
        ksgw::NamePool      _namePool;
        std::queue<Token*>  _tokens;
        std::string         _text;
        size_t              _pos;
        std::string         _tokenBuf;
        State               _state;
    public:
        TokenParser()
            : _namePool()
            , _tokens()
            , _text()
            , _pos(0)
            , _tokenBuf()
            , _state(State::None)
        {}

        Token* nextToken();
    private:
        void dealNone( char ch);
    };

}