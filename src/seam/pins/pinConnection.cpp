#if RUN_DOCTEST
#include "doctest.h"
#endif
 
#include "pinConnection.h"

namespace seam::pins {
    PinConnection::PinConnection(PinInput* _input, PinType outputType) {
        input = _input;
        bool canConvert;
        convertSingle = GetConvertSingle(outputType, input->type, canConvert);
        assert(canConvert);
        convertMulti = GetConvertMulti(outputType, input->type, canConvert);
        assert(canConvert);
    }


    ConvertSingle GetConvertSingle(PinType srcType, PinType dstType, bool& isConvertible) {
        isConvertible = true;
        // If types match, no conversion is needed and a simple memcpy can be used.
        if (dstType == srcType) {
            size_t elementSize = PinTypeToElementSize(dstType);
            return [elementSize](void* src, void* dst) { 
                std::copy((char*)src, (char*)src + elementSize, (char*)dst); 
            };
        }
        
        switch (dstType) {
            case PinType::FLOAT:
                switch (srcType) {
                    case PinType::INT:
                        return Convert<int32_t, float>;
                    case PinType::UINT:
                        return Convert<uint32_t, float>;
                    case PinType::BOOL:
                        return Convert<bool, float>;
                    default:
                        break;
                }
            case PinType::INT:
                switch (srcType) {
                    case PinType::FLOAT:
                        return Convert<float, int32_t>;
                    case PinType::UINT:
                        return Convert<uint32_t, int32_t>;
                    case PinType::BOOL:
                        return Convert<bool, int32_t>;
                    default:
                        break;
                }
            case PinType::BOOL:
                switch (srcType) {
                    case PinType::FLOAT:
                        return Convert<float, bool>;
                    case PinType::INT:
                        return Convert<int32_t, bool>;
                    case PinType::UINT:
                        return Convert<uint32_t, bool>;
                    default:
                        break;
                }
            case PinType::UINT:
                switch (srcType) {
                    case PinType::FLOAT:
                        return Convert<float, bool>;
                    case PinType::INT:
                        return Convert<int32_t, bool>;
                    case PinType::BOOL:
                        return Convert<uint32_t, bool>;
                    default:
                        break;
                }
            case PinType::CHAR: // TODO maybe if char converters are actually needed...
            default:
                break;
        }

        // Return empty converter since we can't actually do it...
        isConvertible = false;
        return ConvertSingle();
    }

    ConvertMulti GetConvertMulti(PinType srcType, PinType dstType, bool& isConvertible) {
        if (srcType == dstType) {
            isConvertible = true;
            size_t elementSize = PinTypeToElementSize(dstType);
            return [elementSize](void* src, size_t srcSize, void* dst, size_t dstSize) {
                size_t bytesToCopy = std::min(srcSize, dstSize);
                std::copy((char*)src, (char*)src + bytesToCopy, (char*)dst);
            };
        } else {
            ConvertSingle Convert = GetConvertSingle(srcType, dstType, isConvertible);
            if (isConvertible) {
                size_t srcElementSize = PinTypeToElementSize(srcType);
                size_t dstElementSize = PinTypeToElementSize(dstType);
                return [Convert, srcElementSize, dstElementSize](void* _src, size_t srcSize, void* _dst, size_t dstSize) {
                    char* src = (char*)_src;
                    char* srcEnd = src + srcSize;
                    char* dst = (char*)_dst;
                    char* dstEnd = dst + dstSize;

                    while (src != srcEnd && dst != dstEnd) {
                        Convert(src, dst);
                        src += srcElementSize;
                        dst += dstElementSize;
                    }
                };
            }
        }
        return ConvertMulti();
    }
}


#if RUN_DOCTEST
namespace seam::pins {

TEST_CASE("Test single int to float pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::INT, PinType::FLOAT, isConvertible);
	CHECK(isConvertible);
	float f = 4.2f;
	int i = 29;
	Convert(&i, &f);
	CHECK(i == f);
}

TEST_CASE("Test single uint to float pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::UINT, PinType::FLOAT, isConvertible);
	CHECK(isConvertible);
	float f = -4.2f;
    uint32_t u = 10;
	Convert(&u, &f);
	CHECK(u == f);
}

TEST_CASE("Test single bool to float pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::BOOL, PinType::FLOAT, isConvertible);
	CHECK(isConvertible);
	float f = 1092.234f;
    bool b = true;
	Convert(&b, &f);
	CHECK(b == f);
}

TEST_CASE("Test single float to int32 pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::FLOAT, PinType::INT, isConvertible);
	CHECK(isConvertible);
	int i = 29;
	float f = 4.2f;
	Convert(&f, &i);
	CHECK(i == (int)f);
}

TEST_CASE("Test single uint to int32 pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::UINT, PinType::INT, isConvertible);
	CHECK(isConvertible);
	int i = -29;
    uint32_t u = 10;
	Convert(&u, &i);
	CHECK(u == (uint32_t)i);
}

TEST_CASE("Test single bool to int32 pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::BOOL, PinType::INT, isConvertible);
	CHECK(isConvertible);
	int i = -29;
    bool b = true;
	Convert(&b, &i);
	CHECK(b == i);
}

TEST_CASE("Test single float to bool pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::FLOAT, PinType::BOOL, isConvertible);
	CHECK(isConvertible);
	bool b = false;
	float f = 4.2f;
	Convert(&f, &b);
	CHECK(b == (bool)f);
}

TEST_CASE("Test single int to bool pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::INT, PinType::BOOL, isConvertible);
	CHECK(isConvertible);
	bool b = false;
    int32_t i = 10;
	Convert(&i, &b);
	CHECK(b == (bool)i);
}

TEST_CASE("Test single uint to bool pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::UINT, PinType::BOOL, isConvertible);
	CHECK(isConvertible);
	bool b = false;
    uint32_t u = 10;
	Convert(&u, &b);
	CHECK(b == (bool)u);
}

TEST_CASE("Test multi float to int pin converter") {
    std::array<float, 4> floats = { 1.0, 2.0, 3.5, -10.9 };
    std::array<int32_t, 4> ints = { 5, 10, 14, 20 };
    bool isConvertible;
    pins::ConvertMulti Convert = seam::pins::GetConvertMulti(PinType::FLOAT, PinType::INT, isConvertible);
    CHECK(isConvertible);
    Convert(floats.data(), floats.size() * sizeof(float), ints.data(), ints.size() * sizeof(int32_t));
    for (size_t i = 0; i < floats.size(); i++) {
        CHECK((int32_t)floats[i] == ints[i]);
    }
}

TEST_CASE("Test multi float to bool pin converter") {
    std::array<float, 4> floats = { 1.0, 2.0, 3.5, 0.0 };
    std::array<bool, 4> bools = { 0 };
    bool isConvertible;
    pins::ConvertMulti Convert = seam::pins::GetConvertMulti(PinType::FLOAT, PinType::BOOL, isConvertible);
    CHECK(isConvertible);
    Convert(floats.data(), floats.size() * sizeof(float), bools.data(), bools.size() * sizeof(bool));
    for (size_t i = 0; i < floats.size(); i++) {
        CHECK((bool)floats[i] == bools[i]);
    }
}

};
#endif // RUN_DOCTEST