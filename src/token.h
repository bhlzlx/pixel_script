#pragma once

#include <string>
#include "ast_common.h"
namespace compiler {

    using Name = ksgw::Name;

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
            , _string(name)
            , _line(0)
            , _column(0)
        {}
        Token(int64_t val)
            : _type(TokenType::Integer)
            , _integer(val)
            , _line(0)
            , _column(0)
        {}
        Token(double val)
            : _type(TokenType::Float)
            , _number(val)
            , _line(0)
            , _column(0)
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