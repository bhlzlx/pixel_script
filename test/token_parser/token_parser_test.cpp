#include <cstdio>
#include <token_parser.h>

char const* code = R"(
    even = 0
    odd = 0
    i = 1
    while i < 10 {
        if i % 2 == 0 {
            even = even + 1
        } else {
            odd = odd + i
        }
        i = i + 1
    }
    even + odd
)";

int main() {
    compiler::TokenParser parser;
    parser.init(code);
    compiler::Token const* token = nullptr;
    do {
        token = parser.nextToken();
        // print token
        switch(token->type()) {
            case compiler::TokenType::None: {
                printf("None\n");
                break;
            }
            case compiler::TokenType::Identifier: {
                printf("%s\n", token->stringLiteral().text());
                break;
            }
            case compiler::TokenType::Float: {
                printf("%F\n", token->floatLiteral());
                break;
            }
            case compiler::TokenType::String: {
                printf("\"%s\"", token->stringLiteral().text());
                break;
            }
            case compiler::TokenType::LeftBrace: {
                printf("{");
                break;
            }
            case compiler::TokenType::RightBrace: {
                printf("}");
                break;
            }
            case compiler::TokenType::LeftParen: {
                printf("(");
                break;
            }
            case compiler::TokenType::RightParen: {
                printf(")");
                break;
            }
            case compiler::TokenType::LeftBracket: {
                printf("[");
                break;
            }
            case compiler::TokenType::RightBracket: {
                printf("]");
                break;
            }
        }
    } while(token);
    return 0;
}