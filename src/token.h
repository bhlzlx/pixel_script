#pragma once

#include <string>
#include "name_pool.h"
namespace compiler {

    enum class TokenType {
        // Single-character tokens.
        LeftParen/*(*/, RightParen, LeftBracket/*[]*/, RightBracket, LeftBrace/*{}*/, RightBrace,
        Comma, Dot, Minus, Plus, Semicolon, Slash, Star, Modulus,
        // One or two character tokens.
        Bang, BangEqual,
        Equal, EqualEqual,
        Greater, GreaterEqual,
        Less, LessEqual,
        // Literals.
        Identifier, String, Float, Integer,
        // Keywords
        And, Class, Else, False, Fun, For, If, Nil, Or,
        Print, Return, Super, This, True, Var, While,
        Eof,
        None
    };

    class Token {
    private:
        TokenType               _type;
        int                     _lineNumber;
        union { // literal value
            ksgw::Name          _string; // identifier, string
            double              _number;
            int64_t             _integer;
        };
    public:
        Token( TokenType type, ksgw::Name name, int lineNumber )
            : _type(type)
            , _string(name)
            , _lineNumber(lineNumber)
        {}
        Token(int64_t val, int lineNumber)
            : _type(TokenType::Integer)
            , _number(val)
            , _lineNumber(lineNumber)
        {}
        Token(double val, int lineNumber)
            : _type(TokenType::Float)
            , _number(val)
            , _lineNumber(lineNumber)
        {}
        TokenType type() const {
            return _type;
        }
        int lineNumber() const {
            return _lineNumber;
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