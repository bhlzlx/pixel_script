#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <vector>
#include "../name_pool.h"

/** 
 * 我打算让一个包里的方法只能访问包内的变量
 *  
 * 
*/

namespace compiler {

    /**
     * @brief Global script execution context
     * 
     */
    class Env {
    private:
    public:
    };

    using Name = ksgw::Name;

    enum class ValueType : uint8_t {
        Int64,
        Float64,
        String,
    };

    class Value {
    private:
        Name      _name;    // name of the value
        ValueType _type;    // type of the value
        union {
            int64_t     i64;
            double      f64;
            char const* cstr;
            void*       userdefined;
        };
    };


    using ValueIndex = uint32_t;
    using ValueVersion = uint32_t;
    class ValueAccessor {
        friend class ValueTable;
    private:
        ValueVersion    _version;
        ValueIndex      _index;
        Name            _name;
        ValueTable*     _table;
    public:
        ValueAccessor()
            : _name(0), _version(0), _index(0), _table(nullptr) {}

        Value getValue() {
            _table->getValue(*this);
        }
    };

} // namespace name
