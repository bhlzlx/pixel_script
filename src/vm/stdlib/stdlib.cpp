#include "stdlib.h"
#include "std_vec.h"
#include "std_io.h"
#include "std_map.h"

namespace compiler {

    namespace stdlib {

        void init(Env* env) {
            std_vec_impl::init(env);
            std_io_impl::init(env);
            std_map_impl::init(env);
        }

    }


}