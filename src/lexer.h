#pragma once
#include "token.h"

namespace compiler {

    class Lexer {   
    public:
        Lexer( char const* source );
        ~Lexer();
        Token* nextToken();
    };

}