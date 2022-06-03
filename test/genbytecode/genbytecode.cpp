#include <cstdio>
#include <ast_builder.h>
#include <map>

#include <ast_node.h>
#include <vm/vm_env.h>

#include <fstream>

class Buffer {
private:
    uint8_t*    _buffer;
    size_t      _length;
public:
    Buffer()
        : _buffer(nullptr)
        , _length(0)
    {}
    uint8_t const* buffer() const {
        return _buffer;
    }
    size_t length() const {
        return _length;
    }
    ~Buffer() {
        if(_buffer) {
            free(_buffer);
        }
    }
    static Buffer* createFileBuffer(char const* filepath) {
        auto file = fopen(filepath, "rb");
        if(!file) {
            return nullptr;
        }
        auto set = ftell(file);
        fseek(file, 0, SEEK_END);
        auto length = ftell(file) - set;
        Buffer* buffer = new Buffer();
        buffer->_buffer = (uint8_t*)malloc(length + 1);
        buffer->_length = length;
        fseek(file, 0, SEEK_SET);
        fread(buffer->_buffer, 1, buffer->_length, file);
        buffer->_buffer[buffer->_length] = 0;
        fclose(file);
        return buffer;
    }
};

struct LoadItem {
    char const* moduleName;
    char const* filepath;
};

int main(int argc, char** argv) {
    std::string path = argv[0];
    //get the dir of the executable
    path = path.substr(0, path.find_last_of("\\/"));
    path = path.substr(0, path.find_last_of("\\/"));
    path+="/scripts/";

    LoadItem items[] = {
        {"bytecode", "test/bytecode.ps"},
        {"func", "test/func.ps"},
    };
    
    compiler::Env env;

    for(auto& item: items) {
        std::string fullpath = path + item.filepath;
        auto buff = Buffer::createFileBuffer(fullpath.c_str());
        if(!buff) {
            assert(false);
        }
        env.precompileModule(item.moduleName, (char const*)buff->buffer());
        delete buff;
    }

    for(auto& item: items) {
        env.compileModule(item.moduleName); // 编译字节码，生成debug信息
        env.initializeModule(items->moduleName); // 初始化模块
    }
    auto val = env.callFuncWithPath("test.bytecode.entry");
    return 0;
}