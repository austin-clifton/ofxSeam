#include "nodeProperty.h"

#include <stdexcept>

namespace seam::props  {
    
void Serialize(capnp::List<seam::schema::PinValue, capnp::Kind::STRUCT>::Builder& builder,
    NodePropertyType type, void* srcBuff, size_t srcElementsCount)
{
    if (srcElementsCount == 0) {
        return;
    }

    switch (type) {
    case NodePropertyType::PROP_BOOL: {
        bool* bool_values = (bool*)srcBuff;
        for (size_t i = 0; i < srcElementsCount; i++) {
            builder[i].setBoolValue(bool_values[i]);
        }
        break;
    }
    case NodePropertyType::PROP_INT: {
        int32_t* int_values = (int32_t*)srcBuff;
        for (size_t i = 0; i < srcElementsCount; i++) {
            builder[i].setIntValue(int_values[i]);
        }
        break;
    }
    case NodePropertyType::PROP_UINT: {
        uint32_t* uint_values = (uint32_t*)srcBuff;
        for (size_t i = 0; i < srcElementsCount; i++) {
            builder[i].setUintValue(uint_values[i]);
        }
        break;
    }
    case NodePropertyType::PROP_FLOAT: {
        float* float_values = (float*)srcBuff;
        for (size_t i = 0; i < srcElementsCount; i++) {
            builder[i].setFloatValue(float_values[i]);
        }
        break;
    }
    case NodePropertyType::PROP_CHAR:
    case NodePropertyType::PROP_STRING:
    default:
        throw std::logic_error("not implemented yet!");
    }
}

void Deserialize(const capnp::List<seam::schema::PinValue, capnp::Kind::STRUCT>::Reader& serializedValues, NodePropertyType type, void* dstBuff, size_t dstElementsCount) {
    if (serializedValues.size() == 0) {
        return;
    }

    switch (type) {
    case NodePropertyType::PROP_BOOL: {
        if (serializedValues[0].isBoolValue()) {
            bool* bool_channels = (bool*)dstBuff;
            for (size_t i = 0; i < dstElementsCount && i < serializedValues.size(); i++) {
                bool_channels[i] = serializedValues[i].getBoolValue();
            }
        }
        break;
    }
    case NodePropertyType::PROP_FLOAT: {
        if (serializedValues[0].isFloatValue()) {
            float* float_channels = (float*)dstBuff;
            for (size_t i = 0; i < dstElementsCount && i < serializedValues.size(); i++) {
                float_channels[i] = serializedValues[i].getFloatValue();
            }
        }
        break;
    }
    case NodePropertyType::PROP_INT: {
        if (serializedValues[0].isIntValue()) {
            int* int_channels = (int32_t*)dstBuff;
            for (size_t i = 0; i < dstElementsCount && i < serializedValues.size(); i++) {
                int_channels[i] = serializedValues[i].getIntValue();
            }
        }
        break;
    }
    case NodePropertyType::PROP_UINT: {
        if (serializedValues[0].isUintValue()) {
            uint32_t* uint_channels = (uint32_t*)dstBuff;
            for (size_t i = 0; i < dstElementsCount && i < serializedValues.size(); i++) {
                uint_channels[i] = serializedValues[i].getUintValue();
            }
        }
        break;
    }
    case NodePropertyType::PROP_STRING: {
        if (serializedValues[0].isStringValue()) {
            std::string* string_channels = (std::string*)dstBuff;
            for (size_t i = 0; i < dstElementsCount && i < serializedValues.size(); i++) {
                string_channels[i] = serializedValues[i].getStringValue();
            }
        }
        break;
    }

    default:
        throw std::logic_error("not implemented!");
    }
}

NodeProperty SetupFloatProperty(std::string&& name, 
    std::function<float*(size_t&)> getter, std::function<void(float*, size_t)> setter)
{
    // Wrap the type-specific getter and setter around getters and setters that accept void*.
    // This isn't cheap, but property getters and setters are meant to be convenient, not fast...
    return NodeProperty(std::move(name), NodePropertyType::PROP_FLOAT, [getter](size_t& size) -> void* {
        return getter(size);
    }, [setter](void* data, size_t size) {
        setter((float*)data, size);
    });
}

NodeProperty SetupIntProperty(std::string&& name, 
    std::function<int32_t*(size_t&)> getter, std::function<void(int32_t*, size_t)> setter)
{
    return NodeProperty(std::move(name), NodePropertyType::PROP_INT, [getter](size_t& size) -> void* {
        return getter(size);
    }, [setter](void* data, size_t size) {
        setter((int32_t*)data, size);
    });
}

NodeProperty SetupUintProperty(std::string&& name, 
    std::function<uint32_t*(size_t&)> getter, std::function<void(uint32_t*, size_t)> setter)
{
    return NodeProperty(std::move(name), NodePropertyType::PROP_UINT, [getter](size_t& size) -> void* {
        return getter(size);
    }, [setter](void* data, size_t size) {
        setter((uint32_t*)data, size);
    });
}

NodePropertyType SerializedPinTypeToPropType(seam::schema::PinValue::Which pinType) {
    switch (pinType) {
    case seam::schema::PinValue::Which::BOOL_VALUE:
        return NodePropertyType::PROP_BOOL;
    case seam::schema::PinValue::Which::FLOAT_VALUE:
        return NodePropertyType::PROP_FLOAT;
    case seam::schema::PinValue::Which::INT_VALUE:
        return NodePropertyType::PROP_INT;
    case seam::schema::PinValue::Which::UINT_VALUE:
        return NodePropertyType::PROP_UINT;
    case seam::schema::PinValue::Which::STRING_VALUE:
        return NodePropertyType::PROP_STRING;
    default:
        throw std::logic_error("Unimplemented?");
    }
}

seam::schema::PinValue::Which PropTypeToSerializedPinType(NodePropertyType propType) {
    switch (propType) {
        case NodePropertyType::PROP_BOOL:
            return seam::schema::PinValue::Which::BOOL_VALUE;
        case NodePropertyType::PROP_FLOAT:
            return seam::schema::PinValue::Which::FLOAT_VALUE;
        case NodePropertyType::PROP_INT:
            return seam::schema::PinValue::Which::INT_VALUE;
        case NodePropertyType::PROP_CHAR:
        case NodePropertyType::PROP_STRING:
            return seam::schema::PinValue::Which::STRING_VALUE;
        case NodePropertyType::PROP_UINT:
            return seam::schema::PinValue::Which::UINT_VALUE;
        case NodePropertyType::PROP_NONE:
            throw std::logic_error("None is not a valid property type, is this a real Property?");
        default:
            throw std::logic_error("Unimplemented?");
    }
}

} // namespace