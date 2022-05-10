#include "vm_object.h"
#include "vm_types.h"
#include <exception>

namespace compiler {

    Value* Value::operator[](Name name)  {
        if(this->_type == ValueType::Object) {
            Object* obj = (Object*)_ud;
            return obj->operator[](name);
        }
        else {
            return nullptr;
        }
    }

    Value const* Value::operator[](Name name) const {
        if(this->_type == ValueType::Object) {
            Object const* obj = (Object*)_ud;
            return &obj->operator[](name);
        }
        else {
            return nullptr;
        }
    }

    Value* Value::operator[](uint32_t loc) {
        if(this->_type == ValueType::Object) {
            return _obj->at(loc);
        }
        return nullptr;
    }

    Value* Value::operator[](uint32_t loc) const {
        if(this->_type == ValueType::Object) {
            return _obj->at(loc);
        }
        return nullptr;
    }


    Value::Value(SymbolLayout* symbolLayout) {
        _type = ValueType::Object;
        _obj = new Object(symbolLayout);
    }

    Value::Value(Value const& other) {
        _obj = other._obj;
        _type = other._type;
        if(_type == ValueType::Object) {
            _obj->ref();
        }
    }

    Value::Value(Node const* node)
        : _node(node) 
        , _type(ValueType::ASTNode)
    {
    }

    Value::Value( Value&& other) {
        _obj = other._obj;
        _type = other._type;
        other._type = ValueType::Nil;
        other._obj = nullptr;
    }

    Value& Value::operator = (Value const& other) {
        deref(); // deref the old value
        _obj = other._obj;
        _type = other._type;
        if(_type == ValueType::Object) {
            _obj->ref();
        }
        return *this;
    }

    Value& Value::operator = (Value&& other) {
        deref();
        _obj = other._obj;
        _type = other._type;
        other._obj = nullptr;
        other._type = ValueType::Nil;
        return *this;
    }

    Node const* Value::node() const {
        return _node;
    }

    Value::operator bool () const {
        if(_type == ValueType::Nil) {
            return false;
        }
        return _i64 != 0;
    }

    Object* Value::asObject() const {
        if(_type != ValueType::Object) {
            return nullptr;
        }
        return _obj;
    }

    ASTFunction* Value::asFunc() const {
        if(_type != ValueType::ASTNode) {
            return nullptr;
        }
        if(_node->structType() != SType::Function) {
            return nullptr;
        }
        return (ASTFunction*)_node;
    }
    
    ASTVariable* Value::astVar() const {
        if(_type != ValueType::ASTNode) {
            return nullptr;
        }
        if(_node->structType() != SType::Variable) {
            return nullptr;
        }
        return (ASTVariable*)_node;
    }

    void Value::deref() {
        if(_type == ValueType::Object) {
            _obj->deref();
        }
    }

    Value::~Value() {
        deref();
    }

    void Value::setInt64(int64_t i64) {
        _type = ValueType::Int64;
        _i64 = i64;
    }

    void Value::setFloat64(double f64) {
        _type = ValueType::Float64;
        _f64 = f64;
    }

    void Value::setString(Name name) {
        _type = ValueType::String;
        _str = name;
    }

    ValueType Value::type() const {
        return _type;
    }

    void Value::setUd(void * u) {
        _ud = u;
    }

    void* Value::ud() const {
        return _ud;
    }

    int64_t Value::intValue() const {
        return _i64;
    }

    double Value::floatValue() const {
        return _f64;
    }

    Name Value::stringValue() const {
        return _str;
    }

}