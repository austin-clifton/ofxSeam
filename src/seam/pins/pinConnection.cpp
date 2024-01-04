#if RUN_DOCTEST
#include "doctest.h"
#endif
 
#include "pinConnection.h"
#include "pinInput.h"
#include "pin.h"

namespace {
    template <typename SrcT, typename DstT>
    void Convert(seam::pins::ConvertSingleArgs args) {
        for (uint16_t i = 0; i < args.srcNumCoords && i < args.dstNumCoords; i++) {
            *((DstT*)args.dst + i) = *((SrcT*)args.src + i);
        }
    }
}

namespace seam::pins {
    PinConnection::PinConnection(PinInput* _pinIn, PinOutput* _pinOut) : inputPinId(_pinIn->id) {
        pinIn = _pinIn;
        pinOut = _pinOut;
        RecacheConverts();
    }

    void PinConnection::RecacheConverts() {
        bool canConvert;
        convertSingle = GetConvertSingle(pinOut->type, pinIn->type, canConvert);
        assert(canConvert);
        convertMulti = GetConvertMulti(pinIn, pinOut, canConvert);
        assert(canConvert);
    }

    ConvertSingle GetConvertSingle(PinType srcType, PinType dstType, bool& isConvertible) {
        isConvertible = true;
        const bool dstIsAny = dstType == PinType::ANY;
        // If types match, no conversion is needed and a simple memcpy can be used.
        if (dstType == srcType || dstIsAny) {
            size_t elementSize = PinTypeToElementSize(dstIsAny ? srcType : dstType);
            return [elementSize](ConvertSingleArgs args) {
                uint16_t numCoords = std::min(args.srcNumCoords, args.dstNumCoords);
                std::copy((char*)args.src, (char*)args.src + elementSize * numCoords, 
                    (char*)args.dst); 
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
                    default:
                        break;
                }
            case PinType::FLOW: {
                return [](ConvertSingleArgs args) {
                    args.pinIn->Callback();
                };
            }
            default:
                break;
        }

        // Return empty converter since we can't actually do it...
        isConvertible = false;
        return ConvertSingle();
    }

    ConvertMulti GetConvertMulti(PinInput* pinIn, PinOutput* pinOut, bool& isConvertible) {
        if (pinOut->type == pinIn->type) {
            isConvertible = true;
            size_t elementSize = PinTypeToElementSize(pinOut->type);

            // If there's no stride, one big copy might be possible.
            if (pinIn->Stride() == elementSize * pinIn->NumCoords()) {
                if (pinIn->NumCoords() == pinOut->numCoords) {
                    // Destination data has the same type and is formatted as a flat array,
                    // and has the same number of coords as the source. A single copy is possible.
                    return [elementSize](ConvertMultiArgs args) {
                        size_t size;
                        void* dst = args.pinIn->Buffer(size);
                        size_t bytesToCopy = std::min(args.srcSize * args.srcNumCoords * elementSize, 
                            args.pinIn->BufferSize());

                        std::copy((char*)args.src, (char*)args.src + bytesToCopy, (char*)dst);
                    };
                } else {
                    // Src and dst types are the same, and dst is a flat array,
                    // but numCoords differ.
                    return [elementSize](ConvertMultiArgs args) {
                        size_t dstSize;
                        void* dst = args.pinIn->Buffer(dstSize);
                        const uint16_t coordsPerCopy = std::min(args.srcNumCoords, args.pinIn->NumCoords());
                        const size_t bytesPerCopy = coordsPerCopy * elementSize;
                        const size_t totalElements = std::min(args.srcSize, dstSize);

                        // Copy as many coords as possible per element, and then skip to the next element.
                        for (size_t i = 0; i < totalElements; i++) {
                            std::copy((char*)args.src + elementSize * args.srcNumCoords * i,
                                (char*)args.src + bytesPerCopy * (i + i),
                                (char*)dst + elementSize * args.pinIn->NumCoords() * i);
                        }
                    };
                }
            } else {
                // Has to account for stride!
                return [elementSize](ConvertMultiArgs args) {
                    size_t dstSize;
                    char* dst = (char*)args.pinIn->Buffer(dstSize);

                    const uint16_t numCoords = std::min(args.srcNumCoords, args.pinIn->NumCoords());
                    char* src = (char*)args.src;
                    char* srcEnd = src + args.srcSize * args.srcNumCoords * elementSize;
                    size_t i = 0;

                    while (i < dstSize && src != srcEnd) {
                        char* dsti = dst + i * args.pinIn->Stride();
                        std::copy(src, src + elementSize * numCoords, dsti);

                        i++;
                        src = src + elementSize * args.srcNumCoords;
                    }
                };
            }

        } else {
            ConvertSingle Convert = GetConvertSingle(pinOut->type, pinIn->type, isConvertible);
            if (isConvertible) {
                size_t srcStride = PinTypeToElementSize(pinOut->type) * pinOut->numCoords;
                return [Convert, srcStride](ConvertMultiArgs args) {
                    size_t dstElements;
                    char* dst = (char*)args.pinIn->Buffer(dstElements);
                    const size_t dstStride = args.pinIn->Stride();
                    
                    char* src = (char*)args.src;
                    char* srcEnd = src + args.srcSize * srcStride;

                    for (size_t i = 0; i < dstElements && src != srcEnd; src += srcStride, dst += dstStride) {
                        ConvertSingleArgs sArgs(src, args.srcNumCoords, 
                            dst, args.pinIn->NumCoords(), args.pinIn);
                        Convert(sArgs);
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
    ConvertSingleArgs args(&i, 1, &f, 1);
	Convert(args);
	CHECK(i == f);
}

TEST_CASE("Test single uint to float pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::UINT, PinType::FLOAT, isConvertible);
	CHECK(isConvertible);
	float f = -4.2f;
    uint32_t u = 10;
    ConvertSingleArgs args(&u, 1, &f, 1);
	Convert(args);
	CHECK(u == f);
}

TEST_CASE("Test single bool to float pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::BOOL, PinType::FLOAT, isConvertible);
	CHECK(isConvertible);
	float f = 1092.234f;
    bool b = true;
    ConvertSingleArgs args(&b, 1, &f, 1);
	Convert(args);
	CHECK(b == f);
}

TEST_CASE("Test single float to int32 pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::FLOAT, PinType::INT, isConvertible);
	CHECK(isConvertible);
	int i = 29;
	float f = 4.2f;
    ConvertSingleArgs args(&f, 1, &i, 1);
	Convert(args);
	CHECK(i == (int)f);
}

TEST_CASE("Test single uint to int32 pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::UINT, PinType::INT, isConvertible);
	CHECK(isConvertible);
	int i = -29;
    uint32_t u = 10;
    ConvertSingleArgs args(&u, 1, &i, 1);
    Convert(args);
	CHECK(u == (uint32_t)i);
}

TEST_CASE("Test single bool to int32 pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::BOOL, PinType::INT, isConvertible);
	CHECK(isConvertible);
	int i = -29;
    bool b = true;
    ConvertSingleArgs args(&b, 1, &i, 1);
    Convert(args);
	CHECK(b == i);
}

TEST_CASE("Test single float to bool pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::FLOAT, PinType::BOOL, isConvertible);
	CHECK(isConvertible);
	bool b = false;
	float f = 4.2f;
    ConvertSingleArgs args(&f, 1, &b, 1);
    Convert(args);
	CHECK(b == (bool)f);
}

TEST_CASE("Test single int to bool pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::INT, PinType::BOOL, isConvertible);
	CHECK(isConvertible);
	bool b = false;
    int32_t i = 10;
    ConvertSingleArgs args(&i, 1, &b, 1);
	Convert(args);
	CHECK(b == (bool)i);
}

TEST_CASE("Test single uint to bool pin converter") {
	bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::UINT, PinType::BOOL, isConvertible);
	CHECK(isConvertible);
	bool b = false;
    uint32_t u = 10;
    ConvertSingleArgs args(&u, 1, &b, 1);
    Convert(args);
	CHECK(b == (bool)u);
}

TEST_CASE("Test single converter for 2-coord float to 3-coord float") {
    bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::FLOAT, PinType::FLOAT, isConvertible);
	CHECK(isConvertible);
	glm::vec2 src = glm::vec2(-1.f);
    glm::vec3 dst = glm::vec3(1.f);
    ConvertSingleArgs args(&src, 2, &dst, 3);
    Convert(args);
	CHECK(dst.x == -1.f);
	CHECK(dst.y == -1.f);
	CHECK(dst.z == 1.f);
}

TEST_CASE("Test single converter for 3-coord uint to 2-coord float") {
    bool isConvertible = false;
	pins::ConvertSingle Convert = seam::pins::GetConvertSingle(PinType::UINT, PinType::FLOAT, isConvertible);
	CHECK(isConvertible);
	glm::uvec3 src = glm::uvec3(10);
    glm::vec2 dst = glm::vec2(1.f);
    ConvertSingleArgs args(&src, 3, &dst, 2);
    Convert(args);
	CHECK(dst.x == 10.f);
	CHECK(dst.y == 10.f);
}

TEST_CASE("Test multi float to int pin converter") {
    std::array<float, 4> floats = { 1.0, 2.0, 3.5, -10.9 };
    std::array<int32_t, 4> ints = { 5, 10, 14, 20 };
    bool isConvertible;

    pins::PinInput pinIn = pins::SetupInputPin(PinType::INT, nullptr, &ints, ints.size(), "ints");
    pins::PinOutput pinOut = pins::SetupOutputPin(nullptr, PinType::FLOAT, "floats");

    pins::ConvertMulti Convert = seam::pins::GetConvertMulti(&pinIn, &pinOut, isConvertible);
    CHECK(isConvertible);
    Convert(ConvertMultiArgs(floats.data(), 1, floats.size(), &pinIn));
    for (size_t i = 0; i < floats.size(); i++) {
        CHECK((int32_t)floats[i] == ints[i]);
    }
}

TEST_CASE("Test multi float to bool pin converter") {
    std::array<float, 4> floats = { 1.0, 2.0, 3.5, 0.0 };
    std::array<bool, 4> bools = { 0 };
    bool isConvertible;

    pins::PinInput pinIn = pins::SetupInputPin(PinType::BOOL, nullptr, &bools, bools.size(), "");
    pins::PinOutput pinOut = pins::SetupOutputPin(nullptr, PinType::FLOAT, "floats");

    pins::ConvertMulti Convert = seam::pins::GetConvertMulti(&pinIn, &pinOut, isConvertible);
    CHECK(isConvertible);
    Convert(ConvertMultiArgs(floats.data(), 1, floats.size(), &pinIn));
    for (size_t i = 0; i < floats.size(); i++) {
        CHECK((bool)floats[i] == bools[i]);
    }
}

TEST_CASE("Test strided multi converter where source and destination are the same type") {
    std::array<float, 5> dst = { 1.0, 1.0, 1.0, 1.0, 1.0 };
    std::array<float, 2> src = { -1.0, -1.0 };
    bool isConvertible;

    pins::PinInput pinIn = pins::SetupInputPin(PinType::FLOAT, nullptr, &dst, dst.size(), "dst",
        PinInOptions(sizeof(float) * 2));
    pins::PinOutput pinOut = pins::SetupOutputPin(nullptr, PinType::FLOAT, "src");

    pins::ConvertMulti Convert = seam::pins::GetConvertMulti(&pinIn, &pinOut, isConvertible);
    CHECK(isConvertible);

    Convert(ConvertMultiArgs(src.data(), 1, src.size(), &pinIn));
    CHECK(dst[0] == -1.0f);
    CHECK(dst[1] == 1.0f);
    CHECK(dst[2] == -1.0f);
    CHECK(dst[3] == 1.0f);
    CHECK(dst[4] == 1.0f);
}

TEST_CASE("Test strided multi converter where source and destination are different types and have different number of coordinates") {
    std::array<glm::vec2, 4> dst = { glm::vec2(0.f), glm::vec2(1.f), glm::vec2(2.f), glm::vec2(3.f) };
    std::array<glm::ivec3, 2> src = { glm::ivec3(-1), glm::ivec3(-2) };
    bool isConvertible;

    pins::PinInput pinIn = pins::SetupInputPin(PinType::FLOAT, nullptr, &dst, dst.size(), "dst",
        PinInOptions(sizeof(glm::vec2) * 2, 2));
    pins::PinOutput pinOut = pins::SetupOutputPin(nullptr, PinType::INT, "src", 3);

    pins::ConvertMulti Convert = seam::pins::GetConvertMulti(&pinIn, &pinOut, isConvertible);
    CHECK(isConvertible);

    Convert(ConvertMultiArgs(src.data(), 3, src.size(), &pinIn));

    CHECK(dst[0].x == -1.f);
    CHECK(dst[0].y == -1.f);
    CHECK(dst[1].x == 1.f);
    CHECK(dst[1].y == 1.f);
    CHECK(dst[2].x == -2.f);
    CHECK(dst[2].y == -2.f);
    CHECK(dst[3].x == 3.f);
    CHECK(dst[3].y == 3.f);
}

}

#endif // RUN_DOCTEST