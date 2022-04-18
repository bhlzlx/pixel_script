#include <cstdio>
#include <vector>
#include <unordered_map>
#include "vm_types.h"

namespace compiler {

    // 脚本调用栈帧对应的局部变量表模型（都是固定的）
    class ValueTableTemplate {
        using ValueMap = std::unordered_map<Name, ValueIndex>;
    private: 
        size_t                  _size;
        std::vector<Name>       _values;
        ValueMap                _valueMap; // name -> index
    public:
        ValueTableTemplate()
            : _size(0), _values(), _valueMap() {}

        ValueTableTemplate(size_t size)
            : _size(size), _values(size), _valueMap() {}

        ValueIndex getLocation(Name name) {
            auto it = _valueMap.find(name);
            if (it == _valueMap.end()) {
                return ~0UL;
            }
            return it->second;
        }

        void addValue(Name name, Value value) {
            auto it = _valueMap.find(name);
            if (it != _valueMap.end()) {
                return;
            }
            _valueMap[name] = _values.size();
            _values.push_back(name);
        }

    };

    class ValueTable {
    private:
        uint32_t                                _version;
        std::vector<Value>                      _values;
        ValueTableTemplate*                     _template;
    public:
        Value* getValue(ValueAccessor& accessor) {
            if(accessor._version == _version) {
                return &_values[accessor._index];
            }
            auto loc = _template->getLocation(accessor._name);
            accessor._version = _version;
            accessor._index = loc;
            return &_values[loc];
        }

        void setValue(Name name, Value value) {
            auto loc = _template->getLocation(name);
            if (loc == ~0UL) {
                _template->addValue(name, value);
                _values.push_back(value);
            } else {
                _values[loc] = value;
            }
        }
    };

    /**
     * @brief Script stack frame
     * 
     */
    class StackFrame {
    private:
        StackFrame*       _parent;
        ValueTable*       _vt;
    public:
        StackFrame() {
        }
        Value* getValue(ValueAccessor value) {
            return _vt->getValue(value);
        }
    };
}


