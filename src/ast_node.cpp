#include "ast_node.h"

namespace compiler {

    Token Variable::name() const { return id()->token();}

}