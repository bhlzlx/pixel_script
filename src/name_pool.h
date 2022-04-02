#pragma once
#include <cassert>
#include <cstdint>
#include <set>
#include <vector>

namespace ksgw {

    namespace {
        struct name_prototype {
            uint16_t    length;
            char        text[0];
        };
        static_assert(sizeof(name_prototype) == 2, "name_prototype is not 2 bytes");

        class AllocRst {
        private:
            uint8_t* _mem;
            size_t   _size;
        public:
            AllocRst(uint8_t* mem, size_t size)
                : _mem(mem)
                , _size(size) {
            }
            void write(uint8_t const* data, size_t offset, size_t size) {
                assert(size<=_size - offset);
                memcpy(_mem + offset, data, size);
            }
            uint8_t const* ptr() const {
                return _mem;
            }
            size_t size() const {
                return _size;
            }
        };

        template<size_t PageSize = 4096 * 256>
        class NameMemoryPoolPage {
        public:
            constexpr static size_t size = PageSize;
        private:
            uint8_t* _mem;
            size_t   _pos;
        public:
            NameMemoryPoolPage()
                : _mem((uint8_t*)malloc(PageSize))
                , _pos(0) {
            }
            bool valid() const {
                return !!_mem;
            }
            ~NameMemoryPoolPage() {
                if(_mem) {
                    free(_mem);
                    _mem = nullptr;
                }
            }
            AllocRst alloc( size_t size) {
                if(PageSize - _pos >= size ) {
                    AllocRst rst(_mem+_pos, size);
                    _pos += size;
                    return rst;
                }
                return AllocRst(nullptr, 0);
            }
        };
    }

    class Name {
    private:
        union {
            uint8_t const*        _addr;
            name_prototype const* _proto;
        };
    public:
        Name(uint8_t const* addr = nullptr)
            : _addr(addr) {
        }
        char const* text() const {
            if(_proto && _proto->length) {
                return &_proto->text[0];
            }
            return nullptr;
        }
        uint16_t length() const {
            return _proto->length;
        }
        uint8_t const* data() const {
            return _addr;
        }
        bool operator < ( Name const& other) const {
            if(_proto->length < other._proto->length) {
                return true;
            }
            if(_proto->length > other._proto->length) {
                return false;
            }
            return memcmp(_proto->text, other._proto->text, _proto->length) < 0;
        }
    };

    class NamePool {
    private:
        std::vector<NameMemoryPoolPage<>*>      _pages;
        std::set<Name>                          _names;
    public:
        NamePool()
            : _pages{ new NameMemoryPoolPage<>() }
            , _names{}
        {}
        Name getName(Name name) {
            auto rst = _names.find(name);
            if(rst == _names.end()) {
                auto allocRst = _pages.back()->alloc(sizeof(uint16_t) + name.length() + 1);
                if(!allocRst.ptr()) {
                    _pages.emplace_back(new NameMemoryPoolPage<>());
                    allocRst = _pages.back()->alloc(sizeof(uint16_t) + name.length() + 1);
                    assert(allocRst.ptr());
                }
                allocRst.write(name.data(), 0, allocRst.size());
                auto insertIter = _names.insert(Name(allocRst.ptr()));
                return *insertIter.first;
            }
            return *rst;
        }
    };

}