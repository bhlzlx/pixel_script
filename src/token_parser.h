#include <queue>
#include <string>
#include "name_pool.h"
#include "token.h"

namespace compiler {

    struct Keywords {
        ksgw::Name          _none;
        ksgw::Name          _for;
        ksgw::Name          _if;
        ksgw::Name          _while;
        ksgw::Name          _else;
    };

    class TokenBuf {
    private:
        std::string _buf;
    public:
        TokenBuf() : _buf() {
        }
        char const* str() const {
            if(_buf.length()>2) {
                return &_buf[2];
            }
            return nullptr;
        }
        uint16_t len() const {
            return *(uint16_t*)_buf.data();
        }
        void clear() {
            _buf.resize(2, '\0');
        }
        void push_back( char ch ) {
            _buf.push_back(ch);
            ++(*(uint16_t*)_buf.data());
        }
        void assign( std::string const& str ) {
            this->clear();
            for(auto ch : str) {
                this->push_back(ch);
            }
        }
        ksgw::Name asName() const {
            return ksgw::Name((uint8_t const*)_buf.data());
        }
    };

    #define AddKeyword( keyword ) buf.assign(#keyword); _keywords._##keyword = _namePool.getName(buf.asName());
    class TokenParser {
        enum class State {
            None, // no state
            Identifier, // keyword/var
            Integer, // pure number
            Float, //
            // Brackets, // { } [ ] ()
            Op, // + - * / += *= /= ++ --
            // Semicolon, // ;
        };
    private:
        ksgw::NamePool      _namePool;
        // std::queue<Token*>  _tokens;
        char const*         _text;
        size_t              _textLen;
        size_t              _pos;
        TokenBuf            _tokenBuf;
        Token               _token;
        State               _state;
        size_t              _lineNumber;
    private:
        Keywords            _keywords;
    public:
        TokenParser()
            : _namePool()
            // , _tokens()
            , _text()
            , _pos(0)
            , _tokenBuf()
            , _token(TokenType::Nil, nullptr, 0)
            , _state(State::None)
            , _lineNumber(0)
        {
            TokenBuf buf;
            AddKeyword(if);
            AddKeyword(else);
            AddKeyword(while);
            AddKeyword(for);
            AddKeyword(none);
        }

        void init(char const* text) {
            _text = text;
            _textLen = strlen(_text);
        }

        Token const* nextToken();
        Token const* currToken() const;
    private:
        bool matchBrackets(char ch);
        bool matchOperator(char ch);

        bool dealNone(char ch);
        bool dealIdentifier(char ch);
        bool dealFloat(char ch);
        bool dealInteger(char ch);
        bool dealOp(char ch);
        bool dealEOF(char ch);
    };

}