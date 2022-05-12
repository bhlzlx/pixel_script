#include "token.h"
#include "vm/vm_env.h"

namespace compiler {
    namespace keywords {
    #ifdef KEYWORD
    #undef KEYWORD
    #endif
    #define KEYWORD(kw) Name _##kw;
            #include "keywords.h"
            std::set<Name>    _all;
        void init(Env* env) {
            // TokenBuf buf;
            #undef KEYWORD
            #define KEYWORD( keyword ) _##keyword = env->createName(#keyword); _all.insert(_##keyword);
            #include "keywords.h"
        }
    }
}