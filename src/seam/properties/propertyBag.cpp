#include "seam/properties/propertyBag.h"
#include "seam/pins/pin.h"

using namespace seam::pins;
using namespace seam::props;

PropertyBag::PropertyBag(PropertyReader _reader) {
    reader = _reader;

    // Fill the propIndexMap with the names and indices of each Property in the reader.
    for (uint32_t i = 0; i < reader.size(); i++) {
        const char* propName = reader[i].getName().cStr();

        // Check for duped names.
        if (propIndexMap.find(propName) == propIndexMap.end()) {
            propIndexMap[propName] = i;
        } else {
            // IMPROVE ME
            printf("PropertyBag: duplicate property name %s\n", propName);
        }
    }
}

PropertyBag::Property<int32_t> PropertyBag::GetIntProperty(const char* propName) {
    ValuesReader valuesReader;
    Property<int32_t> prop = GetProperty<int32_t>(propName, NodePropertyType::PROP_INT, valuesReader);
    if (prop.name != nullptr) {
        // Fill the Property with values from the reader.
        for (uint32_t i = 0; i < valuesReader.size(); i++) {
            prop.values.push_back(valuesReader[i].getIntValue());
        }
        return prop;
    }
    return Property<int32_t>();
}

PropertyBag::Property<uint32_t> PropertyBag::GetUIntProperty(const char* propName) {
    ValuesReader valuesReader;
    Property<uint32_t> prop = GetProperty<uint32_t>(propName, NodePropertyType::PROP_UINT, valuesReader);
    if (prop.name != nullptr) {
        // Fill the Property with values from the reader.
        for (uint32_t i = 0; i < valuesReader.size(); i++) {
            prop.values.push_back(valuesReader[i].getUintValue());
        }
        return prop;
    }
    return Property<uint32_t>();
}

PropertyBag::Property<float> PropertyBag::GetFloatProperty(const char* propName) {
    ValuesReader valuesReader;
    Property<float> prop = GetProperty<float>(propName, NodePropertyType::PROP_FLOAT, valuesReader);
    if (prop.name != nullptr) {
        // Fill the Property with values from the reader.
        for (uint32_t i = 0; i < valuesReader.size(); i++) {
            prop.values.push_back(valuesReader[i].getFloatValue());
        }
        return prop;
    }
    return Property<float>();
}

PropertyBag::Property<bool> PropertyBag::GetBoolProperty(const char* propName) {
    ValuesReader valuesReader;
    Property<bool> prop = GetProperty<bool>(propName, NodePropertyType::PROP_BOOL, valuesReader);
    if (prop.name != nullptr) {
        // Fill the Property with values from the reader.
        for (uint32_t i = 0; i < valuesReader.size(); i++) {
            prop.values.push_back(valuesReader[i].getBoolValue());
        }
        return prop;
    }
    return Property<bool>();
}

PropertyBag::Property<string> PropertyBag::GetStringProperty(const char* propName) {
    ValuesReader valuesReader;
    Property<string> prop = GetProperty<string>(propName, NodePropertyType::PROP_STRING, valuesReader);
    if (prop.name != nullptr) {
        // Fill the Property with values from the reader.
        for (uint32_t i = 0; i < valuesReader.size(); i++) {
            prop.values.push_back(valuesReader[i].getStringValue());
        }
        return prop;
    }
    return Property<string>();
}

template <typename T>
PropertyBag::Property<T> PropertyBag::GetProperty(
    const char* propName, NodePropertyType propType, ValuesReader& outValuesReader) 
{
    auto it = propIndexMap.find(propName);
    if (it != propIndexMap.end()) {
        auto serializedProperty = reader[it->second];
        NodePropertyType serializedType = SerializedPinTypeToPropType(outValuesReader[0].which());
        Property<T> prop;
        prop.name = serializedProperty.getName().cStr();
        prop.type = serializedType;

        // Validate that values exist and are of the right type.
        outValuesReader = serializedProperty.getValues();
        if (outValuesReader.size() == 0) {
            printf("PropertyBag: Property %s has no values\n", prop.name);
            return Property<T>();
        } else if (propType != NodePropertyType::PROP_NONE && serializedType != propType) {
            printf("PropertyBag: Property %s doesn't have the expected type, cannot deserialize!\n", prop.name);
            return Property<T>();
        }

        return prop;
    }
    return Property<T>();
}