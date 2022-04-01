#include "token_parser.h"
#include <cctype>
#include <regex>

namespace compiler {

    std::regex identifier_reg(R"(^[_a-zA-Z]\w*)");
    std::regex number_reg(R"(^)");


    void TokenParser::dealNone(char ch) {
        if(std::isalpha(ch) || ch == '_') {
            _state = State::Identifier; _tokenBuf.push_back(ch);
        } else if (std::isdigit(ch) || ch == '.') {
            _state = State::Number; _tokenBuf.push_back(ch);
        }
    }

    Token* TokenParser::nextToken() {
        while(_pos < _text.length()) {
            char ch = _text[_pos];
            switch(_state) {
                case State::None: {
                }
            }
        }
    }

}