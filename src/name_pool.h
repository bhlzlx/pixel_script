#pragma once
#include <cassert>
#include <cstdint>
#include <set>
#include <vector>

namespace ksgw {

    // namespace {
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
    // }

    class Name {
    public:
        // 描述一段内存
        struct buffer_t {
            uint8_t const*      data;
            uint16_t            length;
            //
            uint16_t byteNeed() const {
                return sizeof(length) + length;
            }
            bool operator < ( buffer_t const& other) const {
                if(length<other.length) {
                    return true;
                } else if(length>other.length) {
                    return false;
                } else {
                    return memcmp(data, other.data, length) < 0;
                }
            }
            operator Name() const {
                return Name(data-sizeof(length));
            }
        };
        struct GernalLess {
            bool operator () (Name const& a, Name const& b) const {
                buffer_t bufa { (uint8_t const*)a.text(), a.length() };
                buffer_t bufb { (uint8_t const*)b.text(), b.length() };
                return bufa < bufb;
            };
        };
        struct FastLess {
            bool operator () (Name const& a, Name const& b) const {
                return a.data() < b.data();
            };
        };
    private:
        union {
            uint8_t const*        _addr;
            name_prototype const* _proto;
        };
    public:
        Name()
            : _addr(nullptr) {
        }
        operator size_t() const {
            return (size_t)_addr;
        }
        Name(uint8_t const* addr)
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
        // bool operator < ( Name const& other) const {
        //     if(_proto->length < other._proto->length) {
        //         return true;
        //     }
        //     if(_proto->length > other._proto->length) {
        //         return false;
        //     }
        //     return memcmp(_proto->text, other._proto->text, _proto->length) < 0;
        // }
        bool operator == (Name const& other) const {
            return _proto == other._proto;
        }
        bool operator != (Name const& other) const {
            return _proto != other._proto;
        }
    };

    class NamePool {
        using buffer_t = Name::buffer_t;
    private:
        std::vector<NameMemoryPoolPage<>*>      _pages;
        std::set<buffer_t>                      _names;
    public:
        NamePool()
            : _pages{ new NameMemoryPoolPage<>() }
            , _names{}
        {}
        Name getName(char const* str) {
            buffer_t buf = {
                (uint8_t*)str,
                (uint16_t)(strlen(str) + 1)
            };
            auto rst = _names.find(buf);
            if(rst == _names.end()) {
                auto allocRst = _pages.back()->alloc(buf.byteNeed());
                if(!allocRst.ptr()) {
                    _pages.emplace_back(new NameMemoryPoolPage<>());
                    allocRst = _pages.back()->alloc(buf.byteNeed());
                    assert(allocRst.ptr());
                }
                allocRst.write((uint8_t const*)&buf.length, 0, sizeof(buf.length));
                allocRst.write((uint8_t const*)buf.data, sizeof(buf.length), buf.length);
                buf.data = allocRst.ptr() + sizeof(buf.length);
                auto insertIter = _names.insert(buf);
                return buf;
            }
            return *rst;
        }
    };

}

namespace std {
    template<>
    struct hash<ksgw::Name> {
        size_t operator()(ksgw::Name const& name) const {
            std::hash<size_t> hasher;
            return hasher((size_t)name.data());
        }
    };
}