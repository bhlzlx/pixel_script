#include "vm_object.h"
#include "vm_types.h"
#include "vm_userdata.h"
#include "vm_env.h"
#include <exception>

namespace compiler {

    Value Value::operator[](Name name) const {
        if(this->_type == PrimeVType::Object) {
            Object* obj = (Object*)_ud;
            return obj->operator[](name);
        } else if(this->_type == PrimeVType::Userdata) {
            UserdataObject* ud = (UserdataObject*)_ud;
            auto bridgeFunc = ud->getFunction(name);
            if(bridgeFunc) {
                return Value(bridgeFunc);
            } else {
                return Value();
        }
        } else {
            return Value();
        }
    }

    Value Value::operator[](uint32_t loc) const {
        if(this->_type == PrimeVType::Object) {
            return _obj->at(loc);
        }
        return Value();
    }

    Value::Value(SymbolLayout* symbolLayout) {
        _type = PrimeVType::Object;
        _obj = new Object(symbolLayout);
        _stype = symbolLayout->type();
    }

    Value::Value(Value const& other) {
        _obj = other._obj;
        _type = other._type;
        _stype = other._stype;
        incRef();
    }

    Value::Value(Value* ref) {
        _type = PrimeVType::ValueRef;
        _ref = ref;
    }
    
    Value::Value(BridgeFunc func) {
        _type = PrimeVType::BridgeFunc;
        _bridgeFunc = func;
    }

    Value::Value(Node const* node)
        : _node(node) 
        , _type(PrimeVType::FunctionNode)
        , _stype(SymbolLayoutType::Function)
    {
        assert(node->structType() == SType::Function);
    }

    Value::Value(UserdataObject* ud)
        : _type(PrimeVType::Userdata)
        , _ud(ud)
    {}

    Value::Value( Value&& other) {
        _obj = other._obj;
        _type = other._type;
        _stype = other._stype;
        other._type = PrimeVType::Nil;
        other._obj = nullptr;
    }

    Value::Value(Name name) {
        _str = name;
        _type = PrimeVType::String;
    }


    Value::Value(uint64_t val) {
        _i64 = val;
        _type = PrimeVType::Int64;
    }

    Value& Value::operator = (Value const& other) {
        if(_type == PrimeVType::ValueRef) { // 把other的值，赋值给自己在的ref
            *_ref = other;
        } else {
            decRef(); // decRef the old value
            _obj = other._obj;
            _type = other._type;
            _stype = other._stype;
            incRef(); // incRef the new value
        }
        return *this;
    }

    Value& Value::operator = (Value&& other) {
        if(_type == PrimeVType::ValueRef) {
            *_ref = other;
            other._ref = nullptr;
        } else {
            decRef();
            _obj = other._obj;
            _type = other._type;
            _stype = other._stype;
            other._obj = nullptr;
        }
        other._type = PrimeVType::Nil;
        other._stype = SymbolLayoutType::None;
        return *this;
    }

    void Value::incRef() {
        if(_type == PrimeVType::Object) {
            _obj->incRef();
        } else if(_type == PrimeVType::Userdata) {
            _ud->incRef();
        }
    }

    void Value::decRef() {
        if(_type == PrimeVType::Object) {
            _obj->decRef();
        } else if(_type == PrimeVType::Userdata) {
            _ud->decRef();
        }
    }

    Value* Value::ref() {
        if(_type == PrimeVType::ValueRef) {
            return _ref;
        }
        return this;
    }

    void Value::deref() {
        if(_type == PrimeVType::ValueRef) {
            auto ref = _ref;
            new(this)Value(*ref);
            incRef();
        }
    }

    Node const* Value::node() const {
        return _node;
    }

    Value::operator bool () const {
        if(_type == PrimeVType::Nil) {
            return false;
        }
        return _i64 != 0;
    }

    Object* Value::asObject() const {
        if(_type != PrimeVType::Object) {
            return nullptr;
        }
        return _obj;
    }

    Function* Value::asFunc() const {
        if(_type != PrimeVType::FunctionNode) {
            return nullptr;
        }
        if(_node->structType() != SType::Function) {
            return nullptr;
        }
        return (Function*)_node;
    }
    
    BridgeFunc Value::asBridgeFunc() const {
        if(_type != PrimeVType::BridgeFunc) {
            return nullptr;
        }
        return _bridgeFunc;
    }

    void Value::enumerateFunctions(std::function<void(ast::Function*)> const& func) const {
        if(_type == PrimeVType::Object) {
            auto symlayout = this->asObject()->symbolLayout();
            for(auto const& sym : symlayout->symbols()) {
                if(sym.symbol.type() == SymbolType::Function) {
                    func((ast::Function*)sym.value.asFunc());
                }
            }
        }
    }

    Value::~Value() {
        decRef();
    }

    void Value::setInt64(int64_t i64) {
        _type = PrimeVType::Int64;
        _i64 = i64;
    }

    void Value::setFloat64(double f64) {
        _type = PrimeVType::Float64;
        _f64 = f64;
    }

    void Value::setString(Name name) {
        _type = PrimeVType::String;
        _str = name;
    }

    void Value::setBool(bool val) {
        _type = PrimeVType::Boolean;
        _bool = val;
    }

    PrimeVType Value::type() const {
        return _type;
    }

    UserdataObject* Value::ud() const {
        return _ud;
    }

    int64_t Value::intValue() const {
        return _i64;
    }

    double Value::floatValue() const {
        return _f64;
    }

    Name Value::stringValue() const {
        if(_type == PrimeVType::String) {
            return _str;
        }
        return Name();
    }

    SymbolLayoutType Value::stype() const {
        return _stype;
    }

    bool Value::booleanValue() const {
        return _i64 != 0;
    }


    // Exceptions 
    DumpException::DumpException(Env const* env, ExecutionError error, char const* brifErr)
        : _error(error)
        , _message(env->backtrace(brifErr)) 
    {
    }

} // namespace compiler