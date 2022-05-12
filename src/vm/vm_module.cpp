#include "vm_module.h"
#include "vm_env.h"
#include "vm_object.h"


namespace compiler {

    void Module::addInitliaze(uint32_t loc, Node* node) {
        if(node) {
            _initliazeList.push_back(std::make_pair(loc, node));
        }
    }

    void Module::initialize(Env* env) {
        for (auto& pair : _initliazeList) {
            auto loc = pair.first;
            auto node = pair.second;
            Value* valRef = _package[loc];
            *valRef = env->callFunction(Value(node), {});
        }
    }


    void Module::setHostPackage(Value package) {
        _package = package;
    }

}