#pragma once

#include <vector>
#include <map>
#include <string>

#include "seam/schema/codegen/node-graph.capnp.h"
#include "seam/properties/nodeProperty.h"

namespace seam::props {
    class PropertyBag {
    public:
        /// @brief A Property minus its values.
        struct PropertyInfo {
            const char* name = nullptr;
            NodePropertyType type = NodePropertyType::PROP_NONE;
        };

        template <typename T>
        struct Property : PropertyInfo {
            std::vector<T> values;
        };
        
        PropertyBag(PropertyReader _reader);

        Property<int>           GetIntProperty(const char* propName);
        Property<uint32_t>      GetUIntProperty(const char* propName);
        Property<float>         GetFloatProperty(const char* propName);
        Property<bool>          GetBoolProperty(const char* propName);
        Property<std::string>   GetStringProperty(const char* propName);

        std::vector<const char*> ListProperties();

    private:
        template <typename T>
        Property<T> GetProperty(const char* propName, NodePropertyType type, ValuesReader& outValuesReader);

        PropertyReader reader;

        /// @brief Maps property names to their indices in the reader.
        std::map<const char*, uint32_t> propIndexMap;
    };
}
