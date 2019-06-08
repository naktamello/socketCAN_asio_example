#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <csignal>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <ieee754.h>

using namespace std::chrono_literals;

//#define BIG_ENDIAN_ 0
//#define LITTLE_ENDIAN_ 1


enum class Endianness {
    LITTLE = 0,
    BIG = 1
};

class CanSimpleSerializer {
public:
    CanSimpleSerializer() {
//        endian_ = byte_order();
        endian_ = Endianness::LITTLE;
    }


    template<typename T>
    void serialize(const T &value, uint8_t *dst) {
        auto src = reinterpret_cast<const uint8_t *>(&value);
        endian_copy(src, dst, sizeof(T));
    }

    template<typename T>
    T deserialize(uint8_t *src) {
        T dst;
        endian_copy(src, reinterpret_cast<uint8_t *>(&dst), sizeof(T));
        return dst;
    };

    void serialize_float(const float &value, uint8_t *dst) {
        serialize(value, dst);
    }

    float deserialize_float(uint8_t *src) {

        return deserialize<float>(src);
    }

    void serialize_uint32(const uint32_t &value, uint8_t *dst) {
        serialize(value, dst);
    }

    uint32_t deserialize_uint32(uint8_t *src){
        return deserialize<uint32_t>(src);
    }


    void serialize_int32(const int32_t &value, uint8_t *dst) {
        serialize(value, dst);
    }

    int32_t deserialize_int32(uint8_t *src){
        return deserialize<int32_t>(src);
    }

    void serialize_uint16(const uint16_t &value, uint8_t *dst) {
        serialize(value, dst);
    }

    uint16_t deserialize_uint16(uint8_t *src){
        return deserialize<uint16_t>(src);
    }


    void serialize_int16(const int16_t &value, uint8_t *dst) {
        serialize(value, dst);
    }

    int16_t deserialize_int16(uint8_t *src){
        return deserialize<int16_t>(src);
    }



    void endian_copy(const uint8_t *src, uint8_t *dst, size_t size) {
        if (endian_ == Endianness::LITTLE)
            std::memcpy(dst, src, size);
        else {
            size_t offset = size - 1;
            for (std::size_t i = 0; i < size; ++i) {
                *dst++ = *(src + offset--);
            }
        }
    }


    Endianness byte_order() {
        short int word = 0x00001;
        char *b = (char *) &word;
        return (b[0] ? Endianness::LITTLE : Endianness::BIG);
    }

private:
    Endianness endian_;
};


int main(int argc, char *argv[]) {
//    bool uses_correct_float = std::numeric_limits<float>::is_iec559;
    static_assert(std::numeric_limits<float>::is_iec559, "float type is not IEEE754");
    CanSimpleSerializer serializer = CanSimpleSerializer();
    int value = 1212;
    uint8_t buf[8] = {0};
    serializer.serialize_uint16(static_cast<uint16_t >(value), buf);
    uint16_t result =serializer.deserialize_uint16(buf);
    std::cout << "deserialized:" + std::to_string(result) << std::endl;

    std::cout << std::endl << "done" << std::endl;
    return EXIT_SUCCESS;

}