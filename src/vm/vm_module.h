#pragma once
#include "../ast_common.h"
#include "vm_object.h"

namespace compiler {

    /**
     * @brief 
     *   Module 的功能
     * 1. 用来初始化模块数据，有些数据初始化可能有依赖，所以需要手动确定初始化顺序 
     * 2. 用来热更新，程序在开发期间需要热更新模块内的变量，函数等，这时我们需要它快速定位
     */

    class Module {
    private:
        Value                                   _package;       // 模块所在包
        std::vector<std::pair<uint32_t, Node*>> _initliazeList; // 初始化列表
    public:
        Module()
            : _package()
            , _initliazeList()
        {}

        void setHostPackage(Value package);
        void addInitliaze(uint32_t loc, Node* node);

        void initialize(Env* env);
        
    };

}
