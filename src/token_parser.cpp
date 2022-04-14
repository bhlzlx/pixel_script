#include "token_parser.h"
#include <cctype>
#include <regex>
#include <map>

namespace compiler {

    std::map<compiler::TokenType, char const*> tokenTraceMap = {
        {compiler::TokenType::None, "None"},
        {compiler::TokenType::LeftParen, "("},
        {compiler::TokenType::RightParen, ")"},
        {compiler::TokenType::LeftBracket, "["},
        {compiler::TokenType::RightBracket, "]"},
        {compiler::TokenType::LeftBrace, "{"},
        {compiler::TokenType::RightBrace, "}"},
        {compiler::TokenType::Plus, "+"},
        {compiler::TokenType::Minus, "-"},
        {compiler::TokenType::Slash, "/"},
        {compiler::TokenType::Star, "*"},
        {compiler::TokenType::Modulus, "%"},
        {compiler::TokenType::Equal, "="},
        {compiler::TokenType::EqualEqual, "=="},
        {compiler::TokenType::Less, "<"},
        {compiler::TokenType::LessEqual, "<="},
        {compiler::TokenType::Greater, ">"},
        {compiler::TokenType::GreaterEqual, ">="},
        {compiler::TokenType::NotEqual, "!="},
        {compiler::TokenType::Not, "!"},
        {compiler::TokenType::And, "&&"},
        {compiler::TokenType::Or, "||"},
        {compiler::TokenType::Equal, "="},
        {compiler::TokenType::Comma, ","},
        {compiler::TokenType::Semicolon, ";"},
        {compiler::TokenType::Dot, "."},
        {compiler::TokenType::And, "and"},
        {compiler::TokenType::Class, "class"},
        {compiler::TokenType::Else, "else"},
        {compiler::TokenType::False, "false"},
        {compiler::TokenType::Func, "func"},
        {compiler::TokenType::For, "for"},
        {compiler::TokenType::If, "if"},
        {compiler::TokenType::Nil, "nil"},
        {compiler::TokenType::Or, "or"},
        {compiler::TokenType::Print, "print"},
        {compiler::TokenType::Return, "return"},
        {compiler::TokenType::Super, "super"},
        {compiler::TokenType::This, "this"},
        {compiler::TokenType::True, "true"},
        {compiler::TokenType::Var, "var"},
        {compiler::TokenType::While, "while"},
        {compiler::TokenType::Eof, "$eof"},
        {compiler::TokenType::Eol, "$eol\n"},
    };

    void printToken(compiler::Token const* token) {
        auto iter = tokenTraceMap.find(token->type());
        // printf("$%d, %d$", token->line(), token->column());
        if(iter != tokenTraceMap.end()) {
            printf("%s", iter->second);
        } else {
            switch(token->type()) {
                case compiler::TokenType::Identifier:
                    printf("%s", token->stringLiteral().text());
                    break;
                case compiler::TokenType::String:
                    printf("\"%s\"", token->stringLiteral().text());
                    break;
                case compiler::TokenType::Float:
                    printf("%f", token->floatLiteral());
                    break;
                case compiler::TokenType::Integer:
                    printf("%lld", token->integerLiteral());
                    break;
                default:
                    printf("%d", token->type());
                    break;
            }
        }
    }


    bool TokenParser::matchBrackets(char ch) {
        switch(ch) {
            case '(': _token = Token(TokenType::LeftParen, _keywords._none); break;
            case ')': _token = Token(TokenType::RightParen, _keywords._none); break;
            case '[': _token = Token(TokenType::LeftBracket, _keywords._none); break;
            case ']': _token = Token(TokenType::RightBracket, _keywords._none); break;
            case '{': _token = Token(TokenType::LeftBrace, _keywords._none); break;
            case '}': _token = Token(TokenType::RightBrace, _keywords._none); break;
            default:
            return false;
        }
        return true;
    }

    bool TokenParser::matchOperator(char ch) {
        if(_token.type() == TokenType::None) {
            switch(ch) {
                case '+': _token = Token(TokenType::Plus, _keywords._none); break;
                case '-': _token = Token(TokenType::Minus, _keywords._none); break;
                case '/': _token = Token(TokenType::Slash, _keywords._none); break;
                case '*': _token = Token(TokenType::Star, _keywords._none); break;
                case '%': _token = Token(TokenType::Modulus, _keywords._none); break;
                case '=': _token = Token(TokenType::Equal, _keywords._none); break;
                case '>': _token = Token(TokenType::Greater, _keywords._none); break;
                case '<': _token = Token(TokenType::Less, _keywords._none); break;
            }
            return false;
        } else {
            switch(_token.type()) {
                case TokenType::Equal: {
                    switch(ch) {
                        case '=': _token = Token(TokenType::EqualEqual, _keywords._none); break;
                        case '>': _token = Token(TokenType::EqualEqual, _keywords._none); break;
                    }
                    break;
                }
                case TokenType::Less: {
                    switch(ch) {
                        case '=': _token = Token(TokenType::LessEqual, _keywords._none); break;
                    }
                    break;
                }
                default: {
                    fallback();
                }
            }
            updateTokenLocation();
            return true;
        }
        return true;
    }

    bool TokenParser::dealNone(char ch) {
        if(_tokenColumn == -1) {
            _tokenColumn = _column;
        }
        _tokenBuf.clear();
        _token = Token(TokenType::None, _keywords._none);
        bool rst = false;
        if(std::isalpha(ch) || ch == '_') {
            _state = State::Identifier; _tokenBuf.push_back(ch);
        } else if (std::isdigit(ch)) {
            _state = State::Integer; _tokenBuf.push_back(ch);
        } else if(matchBrackets(ch)) {
            rst = true;
        } else if(matchOperator(ch) || _token.type() != TokenType::None) {
            _state = State::Op;
        } else if(';' == ch) {
            _token = Token(TokenType::Semicolon, _keywords._none);
            updateTokenLocation();
            rst = true;
        } else if('\n' == ch) {
            _token = Token(TokenType::Eol, _keywords._none);
            _tokenColumn = _column;
            updateTokenLocation();
            ++_line;
            _column = 0;
            rst = true;
        }
        return rst;
    }

    bool TokenParser::dealIdentifier(char ch) {
        if(_tokenColumn == -1) {
            _tokenColumn = _column;
        }
        if(std::isalpha(ch) || ch == '_' || std::isdigit(ch)) {
            _tokenBuf.push_back(ch);
            return false;
        } else {
            if(!std::isblank(ch)) {
                fallback();
            }
            _token = Token(TokenType::Identifier, _namePool.getName(_tokenBuf.asName()));
            if(_keywords._all.find(_token.stringLiteral()) != _keywords._all.end()) {
                _token.setType(TokenType::Keyword);
            }
            updateTokenLocation();
            return true;
        }
    }

    bool TokenParser::dealFloat(char ch) {
        if(std::isdigit(ch)) {
            _tokenBuf.push_back(ch);
            return false;
        } else {
            if(!std::isblank(ch)) {
                fallback();
                // --_pos;
            }
            return true;
        }
    }

    bool TokenParser::dealInteger(char ch) {
        if(std::isdigit(ch)) {
            _tokenBuf.push_back(ch);
            return false;
        } else if('.' == ch) {
            _state = State::Float;
            return false;
        } else {
            _token = Token( (int64_t)atoi(_tokenBuf.str()));
        }
        updateTokenLocation();
        fallback();
        // --_pos; // fallback
        return true;
    }

    bool TokenParser::dealOp(char ch) {
        auto rst = matchOperator(ch);
        assert(rst == true);
        return rst;
    }

    bool TokenParser::dealEOF(char ch) {
        switch(_state) {
            case State::Identifier: {
                _token = Token(TokenType::Identifier, _tokenBuf.asName());
                break;
            }
            case State::Integer: {
                _token = Token((int64_t)atoi(_tokenBuf.str()));
                break;
            }
            case State::Float: {
                _token = Token((double)atof(_tokenBuf.str()));
                break;
            }
            case State::Op: {
                break;
            }
            default: {
                _token = Token(TokenType::Eof, _keywords._none);
            }
        }
        updateTokenLocation();
        return true;
    }

    Token const* TokenParser::nextToken() {
        // if(_tokenCached) {
        //     return &_token;
        // }
        while(_pos < _textLen) {
            char ch = _text[_pos];
            bool rst = false;
            switch(_state) {
                case State::None: {
                    rst = dealNone(ch); break;
                }
                case State::Identifier: {
                    rst = dealIdentifier(ch); break;
                }
                case State::Integer: {
                    rst = dealInteger(ch); break;
                }
                case State::Float: {
                    rst = dealFloat(ch); break;
                }
                case State::Op: {
                    rst = dealOp(ch); break;
                }
            }
            ++_pos;
            ++_column;
            if(rst) {
                _state = State::None;
                if(_token.type() == TokenType::None) {
                    continue;
                }
                // _tokenCached =  true;
                return &_token;
            }
            if(_state == State::None) {
                _tokenColumn = -1;
            }
        }
        _token.setType(TokenType::Eof);
        updateTokenLocation();
        return &_token;
    }

    // void TokenParser::peek() {
    //     // printToken(&_token);
    //     _tokenCached = false;
    // }

    // void TokenParser::peekCommaEol() {
    //     Token const* t = nextToken();
    //     while(t->type() == TokenType::Comma || t->type() == TokenType::Eol) {
    //         peek();
    //         t = nextToken();
    //     }
    // }
}