#include <cstdio>
#include <token_parser.h>
#include <ast_builder.h>
#include <map>

char const* code = R"(
    func fact(n) {
        f = 1
        while n > 0 {
            f = f * n
            n = n - 1
        }
    }
    the_func = func (value) {
        f = 1
        while n > 0 {
            f = f * n
            n = n - 1
        }
    } 
    fact(9)
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
    compiler::ASTBuilder builder;
    auto prog = builder.buildAST(code);
    assert(prog);
    return 0;
}