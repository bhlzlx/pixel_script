#include <cstdint>
#include <cstdio>
#include <unordered_map>

namespace compiler {

    enum class ValueType : uint8_t {
        Int64,
        Float64,
        String,
    };

    class TypeInfo {
    };

    class Value {
    private:
        ValueType _type;
        union {
            int64_t     i64;
            double      f64;
            char const* cstr;
            void*       userdefined;
        };
    };

} // namespace name
