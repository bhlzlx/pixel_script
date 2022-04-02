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
    while(parser.nextToken())
    return 0;
}