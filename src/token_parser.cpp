#include "token_parser.h"
#include <cctype>
#include <regex>

namespace compiler {

    bool TokenParser::matchBrackets(char ch) {
        switch(ch) {
            case '(': _token = Token(TokenType::LeftParen, _keywords._none, _lineNumber); break;
            case ')': _token = Token(TokenType::RightParen, _keywords._none, _lineNumber); break;
            case '[': _token = Token(TokenType::LeftBracket, _keywords._none, _lineNumber); break;
            case ']': _token = Token(TokenType::RightBracket, _keywords._none, _lineNumber); break;
            case '{': _token = Token(TokenType::LeftBrace, _keywords._none, _lineNumber); break;
            case '}': _token = Token(TokenType::RightBrace, _keywords._none, _lineNumber); break;
            default:
            return false;
        }
        return true;
    }

    bool TokenParser::matchOperator(char ch) {
        if(_token.type() == TokenType::None) {
            switch(ch) {
                case '+': _token = Token(TokenType::Plus, _keywords._none, _lineNumber); break;
                case '-': _token = Token(TokenType::Minus, _keywords._none, _lineNumber); break;
                case '/': _token = Token(TokenType::Slash, _keywords._none, _lineNumber); break;
                case '*': _token = Token(TokenType::Star, _keywords._none, _lineNumber); break;
                case '%': _token = Token(TokenType::Modulus, _keywords._none, _lineNumber); break;
                case '=': _token = Token(TokenType::Equal, _keywords._none, _lineNumber); break;
                case '>': _token = Token(TokenType::Greater, _keywords._none, _lineNumber); break;
                case '<': _token = Token(TokenType::Less, _keywords._none, _lineNumber); break;
            }
            return false;
        } else {
            switch(_token.type()) {
                case TokenType::Equal: {
                    switch(ch) {
                        case '=': _token = Token(TokenType::EqualEqual, _keywords._none, _lineNumber); break;
                        case '>': _token = Token(TokenType::EqualEqual, _keywords._none, _lineNumber); break;
                    }
                    break;
                }
                case TokenType::Less: {
                    switch(ch) {
                        case '=': _token = Token(TokenType::LessEqual, _keywords._none, _lineNumber); break;
                    }
                    break;
                }
                default: {
                    --_pos; // fallback
                }
            }
            return true;
        }
        return true;
    }

    bool TokenParser::dealNone(char ch) {
        _tokenBuf.clear();
        _token = Token(TokenType::None, _keywords._none, 0);
        if(std::isalpha(ch) || ch == '_') {
            _state = State::Identifier; _tokenBuf.push_back(ch);
            return false;
        } else if (std::isdigit(ch)) {
            _state = State::Integer; _tokenBuf.push_back(ch);
            return false;
        } else if(matchBrackets(ch)) {
            return true;
        } else if(matchOperator(ch) || _token.type() != TokenType::None) {
            _state = State::Op;
        } else if(';' == ch) {
            _token = Token(TokenType::Semicolon, _keywords._none, _lineNumber);
            return true;
        } else {
            return false;
        }
        return false;
    }

    bool TokenParser::dealIdentifier(char ch) {
        if(std::isalpha(ch) || ch == '_' || std::isdigit(ch)) {
            _tokenBuf.push_back(ch);
            return false;
        } else {
            if(!std::isblank(ch)) {
                --_pos; // fallback
            }
            _token = Token(TokenType::Identifier, _tokenBuf.asName(), _lineNumber);
            return true;
        }
    }

    bool TokenParser::dealFloat(char ch) {
        if(std::isdigit(ch)) {
            _tokenBuf.push_back(ch);
            return false;
        } else {
            if(!std::isblank(ch)) {
                --_pos;
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
            _token = Token( (int64_t)atoi(_tokenBuf.str()), _lineNumber);
        }
        --_pos; // fallback
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
                _token = Token(TokenType::Identifier, _tokenBuf.asName(), _lineNumber);
                break;
            }
            case State::Integer: {
                _token = Token((int64_t)atoi(_tokenBuf.str()), _lineNumber);
                break;
            }
            case State::Float: {
                _token = Token((double)atof(_tokenBuf.str()), _lineNumber);
                break;
            }
            case State::Op: {
                break;
            }
            default: {
                _token = Token(TokenType::Eof, _keywords._none, _lineNumber);
            }
        }
        return true;
    }

    Token const* TokenParser::nextToken() {
        if(_tokenCached) {
            return &_token;
        }
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
            if(rst) {
                _state = State::None;
                return &_token;
            }
        }
        return nullptr;
    }

    void TokenParser::peek() {
        _tokenCached = false;
    }

}