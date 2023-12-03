#pragma once

#include <string>
#include <functional>

#include "seam/schema/codegen/node-graph.capnp.h"
#include "seam/pins/pinInput.h"
#include "seam/properties/nodePropertyType.h"

namespace seam::props {
	using PropertyReader = capnp::List<seam::schema::Property, capnp::Kind::STRUCT>::Reader;
    using ValuesReader = capnp::List<seam::schema::PinValue, capnp::Kind::STRUCT>::Reader;
    using ValuesBuilder = capnp::List<seam::schema::PinValue, capnp::Kind::STRUCT>::Builder;

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

	NodeProperty SetupUintProperty(std::string&& name, 
		std::function<uint32_t*(size_t&)> getter, std::function<void(uint32_t*, size_t)> setter);

	void Deserialize(const seam::schema::PinIn::Reader& serializedPin, seam::pins::PinInput* pinIn);

	void Serialize(ValuesBuilder& serializedValues, 
		NodePropertyType type, seam::pins::PinInput* pinIn);

	void DeserializeProperty(const ValuesReader& serializedValues, 
		NodePropertyType type, void* dstBuff, size_t dstSize);

	void SerializeProperty(ValuesBuilder& serializedValues, 
		NodePropertyType type, void* srcBuff, size_t srcSize);

	template <typename T>
	std::vector<T> Deserialize(ValuesReader reader, NodePropertyType propType) {
		std::vector<T> values;
		values.resize(reader.size());
		DeserializeProperty(reader, propType, values.data(), values.size());
		return values;
	}
}