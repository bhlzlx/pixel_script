#include <queue>
#include <string>
#include <deque>
#include "token.h"
#include "vm/vm_env.h"

namespace compiler {

#ifdef KEYWORD
#undef KEYWORD
#endif
#define KEYWORD(kw) Name _##kw;

    struct Keywords {
        #include "keywords.h"
        std::set<Name>    _all;
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
        Env*                _env;
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
        TokenParser(Env* env)
            : _env(env)
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
            // TokenBuf buf;
            #undef KEYWORD
            #define KEYWORD( keyword ) _keywords._##keyword = _env->getName(#keyword); _keywords._all.insert(_keywords._##keyword);
            #include "keywords.h"
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
        bool dealString(char ch);
        bool dealIdentifier(char ch);
        bool dealFloat(char ch);
        bool dealInteger(char ch);
        bool dealOp(char ch);
        bool dealEOF(char ch);
    };

}