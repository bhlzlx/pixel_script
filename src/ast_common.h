#pragma once
#include "name_pool.h"
#include "token.h"

namespace compiler {
    using Name = ksgw::Name;

    enum class IdentifierType {
        Null,
        Local,
        Global,
        Package,
        Member, // package/namespace/object oriented
    };
}
