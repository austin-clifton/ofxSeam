#if RUN_DOCTEST
#include "doctest.h"
#endif
 
#include "pinConnection.h"
#include "pinInput.h"
#include "pin.h"

namespace {
    template <typename SrcT, typename DstT>
    void Convert(void* src, void* dst) {
        *(DstT*)dst = *(SrcT*)src;
    }
}

namespace seam::pins {
    PinConnection::PinConnection(PinInput* _input, PinType outputType) {
        input = _input;
        inputPinId = input->id;
        RecacheConverts(outputType);
    }

    void PinConnection::RecacheConverts(PinType outputType) {
        bool canConvert;
        convertSingle = GetConvertSingle(outputType, input->type, canConvert);
        assert(canConvert);
        convertMulti = GetConvertMulti(outputType, input, canConvert);
        assert(canConvert);
    }

    ConvertSingle GetConvertSingle(PinType srcType, PinType dstType, bool& isConvertible) {
        isConvertible = true;
        const bool dstIsAny = dstType == PinType::ANY;
        // If types match, no conversion is needed and a simple memcpy can be used.
        if (dstType == srcType || dstIsAny) {
            size_t elementSize = PinTypeToElementSize(dstIsAny ? srcType : dstType);
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
                    case PinType::CHAR:
                        return Convert<char, float>;
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
                    case PinType::CHAR:
                        return Convert<char, int32_t>;
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
                    case PinType::CHAR:
                        return Convert<char, bool>;
                    default:
                        break;
                }
            case PinType::UINT:
                switch (srcType) {
                    case PinType::FLOAT:
                        return Convert<float, uint32_t>;
                    case PinType::INT:
                        return Convert<int32_t, uint32_t>;
                    case PinType::BOOL:
                        return Convert<bool, uint32_t>;
                    case PinType::CHAR:
                        return Convert<char, uint32_t>;
                    default:
                        break;
                }
            case PinType::CHAR:
                switch(srcType) {
                    case PinType::FLOAT:
                        return Convert<float, char>;
                    case PinType::INT:
                        return Convert<int32_t, char>;
                    case PinType::BOOL:
                        return Convert<bool, char>;
                    case PinType::UINT:
                        return Convert<uint32_t, char>;
                }
            default:
                break;
        }

        // Return empty converter since we can't actually do it...
        isConvertible = false;
        return ConvertSingle();
    }

    ConvertMulti GetConvertMulti(PinType srcType, PinInput* pinIn, bool& isConvertible) {
        if (srcType == pinIn->type) {
            isConvertible = true;
            size_t elementSize = PinTypeToElementSize(srcType);

            // If there's no stride, just copy away.
            if (pinIn->Stride() == elementSize) {
                return [elementSize](void* src, size_t srcSize, PinInput* pinIn) {
                    size_t size;
                    void* dst = pinIn->GetChannels(size);
                    size_t bytesToCopy = std::min(srcSize, pinIn->BufferSize());

                    std::copy((char*)src, (char*)src + bytesToCopy, (char*)dst);
                };
            } else {
                // Has to account for stride!
                return [elementSize](void* _src, size_t srcSize, PinInput* pinIn) {
                    size_t dstElements;
                    char* dst = (char*)pinIn->GetChannels(dstElements);

                    char* src = (char*)_src;
                    char* srcEnd = src + srcSize;

                    for (size_t i = 0; i < dstElements && src != srcEnd; src += elementSize, i++) {
                        char* dsti = dst + i * pinIn->Stride() + pinIn->Offset();
                        std::copy(src, src + elementSize, dsti);
                    }
                };
            }

        } else {
            ConvertSingle Convert = GetConvertSingle(srcType, pinIn->type, isConvertible);
            if (isConvertible) {
                size_t srcElementSize = PinTypeToElementSize(srcType);
                size_t dstElementSize = PinTypeToElementSize(pinIn->type);
                return [Convert, srcElementSize, dstElementSize](void* _src, size_t srcSize, PinInput* pinIn) {
                    size_t dstElements;
                    char* dst = (char*)pinIn->GetChannels(dstElements);
                    const size_t stride = pinIn->Stride();
                    
                    char* src = (char*)_src;
                    char* srcEnd = src + srcSize;
                    // char* dstEnd = dst + dstSize;

                    for (size_t i = 0; i < dstElements && src != srcEnd; src += srcElementSize, dst += stride) {
                        Convert(src, dst);
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

    pins::PinInput pinIn = pins::SetupInputPin(PinType::INT, nullptr, &ints, ints.size(), "");

    pins::ConvertMulti Convert = seam::pins::GetConvertMulti(PinType::FLOAT, &pinIn, isConvertible);
    CHECK(isConvertible);
    Convert(floats.data(), floats.size() * sizeof(float), &pinIn);
    for (size_t i = 0; i < floats.size(); i++) {
        CHECK((int32_t)floats[i] == ints[i]);
    }
}

TEST_CASE("Test multi float to bool pin converter") {
    std::array<float, 4> floats = { 1.0, 2.0, 3.5, 0.0 };
    std::array<bool, 4> bools = { 0 };
    bool isConvertible;

    pins::PinInput pinIn = pins::SetupInputPin(PinType::BOOL, nullptr, &bools, bools.size(), "");

    pins::ConvertMulti Convert = seam::pins::GetConvertMulti(PinType::FLOAT, &pinIn, isConvertible);
    CHECK(isConvertible);
    Convert(floats.data(), floats.size() * sizeof(float), &pinIn);
    for (size_t i = 0; i < floats.size(); i++) {
        CHECK((bool)floats[i] == bools[i]);
    }
}

TEST_CASE("Test strided converter where source and destination are the same type") {
    std::array<float, 5> dst = { 1.0, 1.0, 1.0, 1.0, 1.0 };
    std::array<float, 2> src = { -1.0, -1.0 };
    bool isConvertible;

    pins::PinInput pinIn = pins::SetupInputPin(PinType::FLOAT, nullptr, &dst, dst.size(), "",
        PinInOptions(sizeof(float) * 2));

    pins::ConvertMulti Convert = seam::pins::GetConvertMulti(PinType::FLOAT, &pinIn, isConvertible);
    CHECK(isConvertible);

    Convert(src.data(), src.size() * sizeof(float), &pinIn);
    CHECK(dst[0] == -1.0f);
    CHECK(dst[1] == 1.0f);
    CHECK(dst[2] == -1.0f);
    CHECK(dst[3] == 1.0f);
    CHECK(dst[4] == 1.0f);
}

TEST_CASE("Test strided converter where source and destination are different types") {
    // ...TODO
}

};
#endif // RUN_DOCTEST