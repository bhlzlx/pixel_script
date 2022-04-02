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
                printf("Identifier: %s\n", token->stringLiteral().text();
                break;
            }
            case compiler::TokenType::Float: {
                printf("Number: %s\n", token->value().c_str());
                break;
            }
            case compiler::TokenType::String: {
                printf("String: %s\n", token->value().c_str());
                break;
            }
            case compiler::TokenType::Keyword: {
                printf("Keyword: %s\n", token->value().c_str());
                break;
            }
            case compiler::TokenType::Operator: {
                printf("Operator: %s\n", token->value().c_str());
                break;
            }
            case compiler::TokenType::Bracket: {
                printf("Bracket: %s\n", token->value().c_str());
                break;
            }
            case compiler::TokenType::Comment: {
                printf("Comment: %s\n", token->value().c_str());
                break;
            }
            case compiler::TokenType::Error: {
                printf("Error: %s\n", token->value().c_str());
                break;
            }
        }
    } while(token);
    return 0;
}