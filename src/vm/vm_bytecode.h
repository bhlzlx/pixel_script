#pragma once
#include <cstdint>

namespace compiler {

    enum class Opcode {
        CopyTR, // constexpr/local/package/member -> reg
        packageToReg,
        globalToReg,
        regToLocal,
        regToPackage,
        regToGlobal,
        pushReg,
        popReg,
    };

    namespace insgram {

        enum class MoveRType {

        };

        struct MoveR {
            Opcode opcode;

        };
    };


    struct bytecode {

    };


}