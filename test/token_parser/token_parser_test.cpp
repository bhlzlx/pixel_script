#include <cstdio>
#include <token_parser.h>
#include <map>

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
    {compiler::TokenType::Fun, "fun"},
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

int main() {
    compiler::TokenParser parser;
    parser.init(code);
    compiler::Token const* token = nullptr;
    while(token = parser.nextToken()) {
        // print token
        printToken(token);
        parser.peek();
    }
    system("pause");
    return 0;
}