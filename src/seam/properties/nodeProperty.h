#pragma once

#include <cstdint>
#include <string>
#include <functional>

#include "seam/schema/codegen/node-graph.capnp.h"

namespace seam::props {
    enum NodePropertyType : uint8_t {
        PROP_NONE,
		PROP_BOOL,
		PROP_CHAR,
		PROP_FLOAT,
		PROP_INT,
		PROP_UINT,
		PROP_STRING
	}; 

	using PropertyReader = capnp::List<seam::schema::Property, capnp::Kind::STRUCT>::Reader;
    using ValuesReader = capnp::List<seam::schema::PinValue, capnp::Kind::STRUCT>::Reader;

	struct NodeProperty {
		using Getter = std::function<void*(size_t& size)>;
		using Setter = std::function<void(void* values, size_t size)>;

		NodeProperty(std::string&& _name, NodePropertyType _type, Getter&& _getValues, Setter&& _setValues) {
			name = std::move(_name);
			type = _type;
			getValues = std::move(_getValues);
			setValues = std::move(_setValues);
		}

		std::string name;
		NodePropertyType type;
		Getter getValues;
		Setter setValues;
	};

    NodePropertyType SerializedPinTypeToPropType(seam::schema::PinValue::Which pinType);
    seam::schema::PinValue::Which PropTypeToSerializedPinType(NodePropertyType propType);

	NodeProperty SetupFloatProperty(std::string&& name, 
		std::function<float*(size_t&)> getter, std::function<void(float*, size_t)> setter);

	NodeProperty SetupIntProperty(std::string&& name, 
		std::function<int32_t*(size_t&)> getter, std::function<void(int32_t*, size_t)> setter);

	void Deserialize(const ValuesReader& serializedValues, 
		NodePropertyType type, void* dstBuff, size_t dstElementsCount);

	void Serialize(capnp::List<seam::schema::PinValue, capnp::Kind::STRUCT>::Builder& serializedValues,
		NodePropertyType type, void* srcBuff, size_t srcElementsCount);

	template <typename T>
	std::vector<T> Deserialize(ValuesReader reader, NodePropertyType propType) {
		std::vector<T> values;
		values.resize(reader.size());
		Deserialize(reader, propType, values.data(), values.size());
		return values;
	}
}