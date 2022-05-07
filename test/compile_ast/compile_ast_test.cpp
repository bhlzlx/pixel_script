#include <cstdio>
#include <ast_builder.h>
#include <map>

#include <AST.h>
#include <vm/vm_object.h>
#include <vm/vm_env.h>


char const* code = R"(
    package game.test

    var global_var = 1

    var the_func = func (n) {
        n.f = 1
        var f = 1
        while n > 0 {
            f = f * n
            n = n - 1
        }
    } 

    func fact(n) {
        f = 1
        while n > 0 {
            f = f * n
            n = n - 1
        }
    }

    func compute(n) {
        fact(9)
        var even = 0
        var odd = 0
        var i = 1
        while i < 10 {
            if i % 2 == 0 {
                even = even + 1
            } else {
                odd = odd + i
            }
            i = i + 1
        }
        even + odd
    }
)";


std::function<void(compiler::ASTNode const*)> callback = [](compiler::ASTNode const* node){
    if(node->type() == compiler::ASTNodeType::Variable) {
        auto var = static_cast<compiler::ASTVariable const*>(node);
        auto token = var->name();
        printf("def %s line:%d col:%d\n", token.stringLiteral().text(), token.line(), token.column());
        // printf("def %s\n", var->name().text());
    } else if( node->type() == compiler::ASTNodeType::BinaryOp) {
        auto binExpr = static_cast<compiler::ASTBinaryOpExpr const*>(node);
        if(binExpr->left()->type() == compiler::ASTNodeType::Identifier) {
            auto id = static_cast<compiler::ASTLeaf const*>(binExpr->left());
            auto token = id->token();
            if(token.type() == compiler::TokenType::Identifier) {
                printf("ref %s line:%d col:%d\n", token.stringLiteral().text(), token.line(), token.column());
            }
        } 
        if(binExpr->right()->type() == compiler::ASTNodeType::Identifier) {
            if(binExpr->op() != compiler::TokenType::Dot) {
                auto id = static_cast<compiler::ASTLeaf const*>(binExpr->right());
                auto token = id->token();
                if(token.type() == compiler::TokenType::Identifier) {
                    printf("ref %s line:%d col:%d\n", token.stringLiteral().text(), token.line(), token.column());
                }
            }
        }
    } else if( node->type() == compiler::ASTNodeType::Primary) {
        auto primary = static_cast<compiler::ASTPrimary const*>(node);
        if(primary->operand()->type() == compiler::ASTNodeType::Identifier) {
            auto id = static_cast<compiler::ASTLeaf const*>(primary->operand());
            auto token = id->token();
            if(token.type() == compiler::TokenType::Identifier) {
                printf("ref %s line:%d col:%d\n", token.stringLiteral().text(), token.line(), token.column());
            }
        }
    }
};

int main() {
    compiler::Env env;
    compiler::ASTBuilder builder;
    auto prog = builder.buildAST(&env, code);
    assert(prog);
    compiler::ASTMultiExpr* multiExpr = dynamic_cast<compiler::ASTMultiExpr*>(prog.node);
    auto rst = env.compileCodeChunk("game.test", prog.node);
    return 0;
}