#include <queue>
#include <string>
#include <deque>
#include "name_pool.h"
#include "token.h"

namespace compiler {

    struct Keywords {
        ksgw::Name              _none;
        ksgw::Name              _for;
        ksgw::Name              _if;
        ksgw::Name              _while;
        ksgw::Name              _else;
        ksgw::Name              _func;
        ksgw::Name              _closure;
        std::set<ksgw::Name>    _all;
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
            _buf[0] = _buf[1] = 0;
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

    #define AddKeyword( keyword ) buf.assign(#keyword); _keywords._##keyword = _namePool.getName(buf.asName()); _keywords._all.insert(_keywords._##keyword);
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
        // bool                _tokenCached;
        std::vector<Token>  _queuedToken; 
        State               _state;
        int32_t             _line;
        int32_t             _column;
        int32_t             _tokenColumn;
    private:
        Keywords            _keywords;

        void fallback() {
            if(_column)
                --_column;
            if(_tokenColumn)
                --_tokenColumn;
            --_pos;
        }
        void updateTokenLocation() {
            _token.setLocation(_line, _tokenColumn);
            _tokenColumn = -1;
        }
    public:
        TokenParser()
            : _namePool()
            // , _tokens()
            , _text()
            , _pos(0)
            , _tokenBuf()
            , _token(TokenType::None, nullptr)
            // , _tokenCached(false)
            , _state(State::None)
            , _line(0)
            , _column(0)
            , _tokenColumn(0)
        {
            TokenBuf buf;
            AddKeyword(if);
            AddKeyword(else);
            AddKeyword(while);
            AddKeyword(for);
            AddKeyword(func);
            AddKeyword(closure);
            AddKeyword(none);
        }

        void init(char const* text) {
            _text = text;
            _textLen = strlen(_text);
        }

        Keywords const& keywords() const {
            return _keywords;
        }

        Token const* nextToken();
        // void peek();
        // void peekCommaEol();

        size_t pos() const {
            return _pos;
        }
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