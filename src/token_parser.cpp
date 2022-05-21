#include "token_parser.h"
#include "token.h"
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
        {compiler::TokenType::Assign, "="},
        {compiler::TokenType::Equal, "=="},
        {compiler::TokenType::Less, "<"},
        {compiler::TokenType::LessEqual, "<="},
        {compiler::TokenType::Greater, ">"},
        {compiler::TokenType::GreaterEqual, ">="},
        {compiler::TokenType::NotEqual, "!="},
        {compiler::TokenType::Not, "!"},
        {compiler::TokenType::And, "&&"},
        {compiler::TokenType::Or, "||"},
        {compiler::TokenType::Assign, "="},
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
            case '(': _token = Token(TokenType::LeftParen, lang_keywords::_none); break;
            case ')': _token = Token(TokenType::RightParen, lang_keywords::_none); break;
            case '[': _token = Token(TokenType::LeftBracket, lang_keywords::_none); break;
            case ']': _token = Token(TokenType::RightBracket, lang_keywords::_none); break;
            case '{': _token = Token(TokenType::LeftBrace, lang_keywords::_none); break;
            case '}': _token = Token(TokenType::RightBrace, lang_keywords::_none); break;
            default:
            return false;
        }
        updateTokenLocation();
        return true;
    }

    bool TokenParser::matchOperator(char ch) {
        if(_token.type() == TokenType::None) {
            switch(ch) {
                case '+': _token = Token(TokenType::Plus, lang_keywords::_none); break;
                case '-': _token = Token(TokenType::Minus, lang_keywords::_none); break;
                case '/': _token = Token(TokenType::Slash, lang_keywords::_none); break;
                case '*': _token = Token(TokenType::Star, lang_keywords::_none); break;
                case '%': _token = Token(TokenType::Modulus, lang_keywords::_none); break;
                case '=': _token = Token(TokenType::Assign, lang_keywords::_none); break;
                case '>': _token = Token(TokenType::Greater, lang_keywords::_none); break;
                case '<': _token = Token(TokenType::Less, lang_keywords::_none); break;
                case '.': _token = Token(TokenType::Dot, lang_keywords::_none); break;
            }
            return false;
        } else {
            switch(_token.type()) {
                case TokenType::Assign: {
                    switch(ch) {
                        case '=': _token = Token(TokenType::Equal, lang_keywords::_none); break;
                        case '>': _token = Token(TokenType::Equal, lang_keywords::_none); break;
                        default: {
                            fallback();
                        }
                    }
                    break;
                }
                case TokenType::Less: {
                    switch(ch) {
                        case '=': _token = Token(TokenType::LessEqual, lang_keywords::_none); break;
                        default: {
                            fallback();
                        }
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
        _token = Token(TokenType::None, lang_keywords::_none);
        bool rst = false;
        if(std::isalpha(ch) || ch == '_') {
            _state = State::Identifier; _tokenBuf.push_back(ch);
        } else if (std::isdigit(ch)) {
            _state = State::Integer; _tokenBuf.push_back(ch);
        } else if(matchBrackets(ch)) {
            rst = true;
        } else if(matchOperator(ch) || _token.type() != TokenType::None) {
            _state = State::Op;
        } else if(ch == '"') {
            _state = State::String;
        } else if(':' == ch) {
            _token = Token(TokenType::Colon, lang_keywords::_none);
            updateTokenLocation();
            rst = true;
        } else if('.' == ch) {
            _token = Token(TokenType::Dot, lang_keywords::_none);
            updateTokenLocation();
            rst = true;
        } else if(',' == ch) {
            _token = Token(TokenType::Comma, lang_keywords::_none);
            updateTokenLocation();
            rst = true;
        } else if(';' == ch) {
            _token = Token(TokenType::Semicolon, lang_keywords::_none);
            updateTokenLocation();
            rst = true;
        } else if('\n' == ch) {
            _token = Token(TokenType::Eol, lang_keywords::_none);
            _tokenColumn = _column;
            updateTokenLocation();
            ++_line;
            _column = 1;
            rst = true;
        }
        return rst;
    }

    bool TokenParser::dealString(char ch) {
        if(_tokenColumn == -1) {
            _tokenColumn = _column;
        }
        if(_state == State::StringEscape) { 
            if(ch == 'n') {
                _tokenBuf.push_back('\n');
            } else if(ch == 't') {
                _tokenBuf.push_back('\t');
            } else if(ch == 'r') {
                _tokenBuf.push_back('\r');
            } else if(ch == '"') {
                _tokenBuf.push_back('"');
            } else if(ch == '\\') {
                _tokenBuf.push_back('\\');
            } else {
                _tokenBuf.push_back(ch);
            }
            _state = State::String;
        } else if(_state == State::String) {
            if(ch == '"') {
                _state = State::None;
                _token = Token(TokenType::String, _env->createName(_tokenBuf.str()));
                updateTokenLocation();
                return true;
            } else if(ch == '\\') {
                _state = State::StringEscape;
            } else {
                _tokenBuf.push_back(ch);
            }
        }
        return false;
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
            _token = Token(TokenType::Identifier, _env->createName(_tokenBuf.str()));
            if(_token.stringLiteral() == lang_keywords::_true) {
                _token.setType(TokenType::True);
            } else if(_token.stringLiteral() == lang_keywords::_false) {
                _token.setType(TokenType::False);
            } else if(lang_keywords::_all.find(_token.stringLiteral()) != lang_keywords::_all.end()) {
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
                _token = Token(TokenType::Eof, lang_keywords::_none);
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
                case State::String: 
                case State::StringEscape: {
                    rst = dealString(ch); break;
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