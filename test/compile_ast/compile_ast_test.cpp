#include <cstdio>
#include <ast_builder.h>
#include <map>

#include <ast_node.h>
#include <vm/vm_object.h>
#include <vm/vm_env.h>

#include <fstream>

int main(int argc, char** argv) {
    std::string path = argv[0];
    //get the dir of the executable
    path = path.substr(0, path.find_last_of("\\/"));
    path = path.substr(0, path.find_last_of("\\/"));
    path+="/scripts/oop_test.script";


    std::ifstream file(path, std::ios::in | std::ios::binary);
    // get ifstream size
    if(!file.is_open()) {
        return 0;
    }
    auto set = file.tellg();
    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    char *buffer = new char[size-set+1];
    buffer[size] = '\0';
    file.read(buffer, size);
    compiler::Env env;
    compiler::ASTBuilder builder;
    auto prog = builder.buildAST(&env, buffer);
    assert(prog);
    // compiler::MultiExpr* multiExpr = dynamic_cast<compiler::MultiExpr*>(prog.node);
    char const* module = "oop_test";
    auto rst = env.compileCodeChunk(module, prog.node);
    env.initializeModule(module);
    env.callFunction("game.test.entry");
    return 0;
}