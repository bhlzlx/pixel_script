#include "token.h"
#include "vm/vm_env.h"

namespace compiler {
    namespace lang_keywords {
    #ifdef KEYWORD
    #undef KEYWORD
    #endif
    #define KEYWORD(kw) Name _##kw;
            #include "lang_keywords.h"
            std::set<Name>    _all;
        void init(Env* env) {
            // TokenBuf buf;
            #undef KEYWORD
            #define KEYWORD( keyword ) _##keyword = env->createName(#keyword); _all.insert(_##keyword);
            #include "lang_keywords.h"
        }
    }

    namespace lib_keywords {
    #ifdef KEYWORD
    #undef KEYWORD
    #endif
    #define KEYWORD(kw) Name _##kw;
            #include "lib_keywords.h"
            std::set<Name>    _all;
        void init(Env* env) {
            #undef KEYWORD
            #define KEYWORD( keyword ) _##keyword = env->createName(#keyword); _all.insert(_##keyword);
            #include "lib_keywords.h"
        }
    }
}