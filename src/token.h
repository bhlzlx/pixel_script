#pragma once

#include <string>

namespace compiler {

    enum class TokenType {
        // Single-character tokens.
        LeftParen/*(*/, RightParen, LeftBracket/*[]*/, RightBracket, LeftBrace/*{}*/, RightBrace,
        Comma, Dot, Minus, Plus, Semicolon, Slash, Star,
        // One or two character tokens.
        Bang, BangEqual,
        Equal, EqualEqual,
        Greater, GreaterEqual,
        Less, LessEqual,
        // Literals.
        Identifier, String, Number,
        // Keywords
        And, Class, Else, False, Fun, For, If, Nil, Or,
        Print, Return, Super, This, True, Var, While,
        Eof
    };

    using string = std::string;

    string createString ( char const* str );

    class Token {
    private:
        TokenType               _type;
        int                     _lineNumber;
        union { // literal value
            string*             _string; // identifier, string
            double              _number;
        };
    public:
        Token( TokenType type, char const* str, int lineNumber ) {
        }
        TokenType type() const {
            return _type;
        }
        int lineNumber() const {
            return _lineNumber;
        }
        string const* stringLiteral() const {
            return _string;
        }
    };

}