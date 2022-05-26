#pragma once
#include <cstdint>
#include <vector>

namespace compiler {

    enum class Opcode : uint8_t{
        Move,
        Call,
        Return,
        //
        Add,
        Sub,
        Mul,
        Assign,
        Less,
        Greater,
        LessEqual,
        GreaterEqual,
        NotEqual,
        Equal,
        Div,
        Mod,
        // Shl,
        // Shr,
        // And,
        // Or,
        // Xor,
        Push,
        Pop,
        PushFrame,
        PopFrame,
        GetField,
        GetIndexed,
        JumpZero,
        Jump,
        Swap,
        Nop,
    };
    constexpr uint32_t OpcodeBit = 5;
    static_assert((uint8_t)Opcode::Nop <=  (1<<OpcodeBit), "OpcodeBit is too small");

    // move到寄存器，意味着这个寄存器没有被使用，即栈顶的寄存器！
    enum class ScopeType {
        Global,
        Package,
        Member,
        Local,
        Register,
        Constant,
        Self,
        None
    };

    struct JumpInstruction {
        union {
            uint32_t opaque;
            struct {
                uint32_t opcode: OpcodeBit;
                uint32_t pos: 20;
                uint32_t pop:1;
            };
        };
    };

    struct Instruction {
        union {
            struct {
                uint32_t opcode: OpcodeBit;
                uint32_t srcType: 3; // value type
                uint32_t dstType: 3;
                uint32_t src: 10; // 最多1024个成员
                uint32_t dst: 9; // 存的目的数很小, 512
                uint32_t pop:2; // pop count? 1,2,3 
            };
            JumpInstruction jump;
        };

    };

    class Bytecode {
    private:
        std::vector<Instruction>        _instr;
        std::vector<Value>              _constants;
    public:
        Bytecode()
            : _instr()
            , _constants()
        {
        }
        uint32_t getConstant(Value const& val) {
            auto it = std::find(_constants.begin(), _constants.end(), val);
            if(it == _constants.end()) {
                _constants.push_back(val);
                return _constants.size()-1;
            }
            return it - _constants.begin();
        }
        void pushInstr(Instruction const& instr) {
            _instr.push_back(instr);
        }
        void pushInstr(Instruction&& instr) {
            _instr.push_back(std::move(instr));
        }
        Instruction& getInstr(size_t index) {
            return _instr[index];
        }
        size_t size() const {
            return _instr.size();
        }
        void exportInstr(std::vector<Instruction>& instrs) {
            instrs = std::move(_instr);
        }
        void exportConstants(std::vector<Value>& constans) {
            constans = std::move(_constants);
        }
    };

}