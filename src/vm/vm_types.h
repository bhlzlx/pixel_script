#pragma once
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

    // this script is for 64 bit only
    class TagPointer {
    private:
        union {
            uint64_t    _value;
            void*       _pointer;
            struct {
                uint64_t   _tag : 16;
                uint64_t   _addr : 48;
            };
        };
        template<class T>
        T* getPointer() {
            return reinterpret_cast<T*>(_addr);
        };
        template<class T>
        T const* getPointer() const {
            return reinterpret_cast<T*>(_addr);
        };
        template<class T>
        operator T*() {
            return getPointer<T>();
        };
        template<class T>
        operator T const*() const{
            return getPointer<T>();
        };
        TagPointer(void* ptr = nullptr) {
            _pointer = ptr;
            _tag = 0;
        }
    };

    using Name = ksgw::Name;

    enum class ValueType : uint8_t {
        Nil,
        Int64,
        Float64,
        String,
        Object,
        Function,
        UserData
    };

    class Value {
    private:
        ValueType       _type;    // type of the value
        union {
            int64_t     _i64;
            double      _f64;
            Name        _str;
            void*       _ud;
        };
    public:
        Value() 
            : _type(ValueType::Nil)
            , _i64(0)
        {
        }

        void setInt64(int64_t i64) {
            _type = ValueType::Int64;
            _i64 = i64;
        }

        void setFloat64(double f64) {
            _type = ValueType::Float64;
            _f64 = f64;
        }

        void setString(Name name) {
            _type = ValueType::String;
            _str = name;
        }

        ValueType type() const {
            return _type;
        }

        void setUd(void * u) {
            _ud = u;
        }

        void* ud() const {
            return _ud;
        }

        int64_t intValue() const {
            return _i64;
        }

        double floatValue() const {
            return _f64;
        }

        Name stringValue() const {
            return _str;
        }

        Value* operator[](Name name) ;
        Value const* operator[](Name name) const;
    };

    using ValueIndex = uint32_t;
    using ValueVersion = uint32_t;

    // value accesor 用来访问变量表上的的变量
    // 引用变量的形式为 包内变量，栈内局部变量，成员变量
    // 包内变量给一个
    enum class ValueAccessorType : uint8_t {
        Local,
        Member,
        Package,
        Global,
        Const,
        Unknown
    };

    // 局部变量
    // 成员变量
    // 模块内变量
    class ValueAccessor {
        friend class ValueTable;
    private:
        Value*              _value;
        ValueAccessorType   _type;
        Name                _name;
        uint8_t             _tableIndex;
        // 局部变量 0 ~ 255，表示栈帧的索引
        // 成员变量 0 ~ 255，表示成员变量的索引
    public:
        ValueAccessor(Name name)
            : _type(ValueAccessorType::Unknown)
            , _name()
            , _value()
        {
        }
    };

} // namespace name
