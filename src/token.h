#pragma once

#include <string>
#include "compiler_common.h"
namespace compiler {

    class Env;

    using Name = ksgw::Name;

    namespace lang_keywords {
    #ifdef KEYWORD
    #undef KEYWORD
    #endif
    #define KEYWORD(kw) extern Name _##kw;
            #include "lang_keywords.h"
            extern std::set<Name>    _all;
        
        void init(Env* env);
    }

    namespace lib_keywords {
    #ifdef KEYWORD
    #undef KEYWORD
    #endif
    #define KEYWORD(kw) extern Name _##kw;
            #include "lib_keywords.h"
            extern std::set<Name>    _all;
        
        void init(Env* env);
    }

    enum class TokenType {
        // Single-character tokens.
        LeftParen = 0/*(*/, RightParen, LeftBracket/*[]*/, RightBracket, LeftBrace/*{}*/, RightBrace,
        Comma/*,*/, Colon/*:*/,
        Not,
        Dot/*.*/, 
        Slash/*/*/, Star/***/, 
        Minus, Plus, Modulus, Semicolon/*;*/, 
        // One or two character tokens.
        Less, LessEqual,
        Greater, GreaterEqual,
        Equal,
        NotEqual,
        Assign, 
        // Literals.
        Identifier, String, Float, Integer,
        // Keywords
        Keyword,
        And, Class, Else, False, Func, For, If, Nil, Or,
        Print, Return, Super, This, True, Var, While,
        Eol,
        Eof,
        None
    };


    class Token {
    private:
        TokenType               _type;
        int                     _line;
        int                     _column;
        union { // literal value
            double              _number;
            Name                _string; // identifier, string
            int64_t             _integer;
        };
    public:
        Token( TokenType type = TokenType::None, ksgw::Name name = ksgw::Name(nullptr))
            : _type(type)
            , _line(0)
            , _column(0)
            , _string(name)
        {}
        Token(int64_t val)
            : _type(TokenType::Integer)
            , _line(0)
            , _column(0)
            , _integer(val)
        {}
        Token(double val)
            : _type(TokenType::Float)
            , _line(0)
            , _column(0)
            , _number(val)
        {}
        void setType(TokenType type) {
            _type = type;
        }
        TokenType type() const {
            return _type;
        }
        void setLocation( int line, int column ) {
            _line = line;
            _column = column;
        }
        int line() const {
            return _line;
        }
        int column() const {
            return _column;
        }
        ksgw::Name stringLiteral() const {
            return _string;
        }
        int64_t integerLiteral() const {
            return _integer;
        }
        double floatLiteral() const {
            return _number;
        }
    };

}