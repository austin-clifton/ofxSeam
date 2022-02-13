#pragma once

#include "pin.h"

namespace seam::pins {
	struct PinFloatMeta {
		float min = -FLT_MAX;
		float max = FLT_MAX;
		RangeType range_type = RangeType::LINEAR;
	};

	template <std::size_t N>
	struct PinFloat : public PinInput<float, N>, public PinFloatMeta {
		PinFloat(
			std::string_view _name = "float",
			std::string_view _description = "",
			std::array<float, N> _init_vals = { 0 },
			float _min = -FLT_MAX,
			float _max = FLT_MAX,
			RangeType _range_type = RangeType::LINEAR,
			PinFlags _flags = PinFlags::INPUT
		)
			: PinInput<float, N>(_init_vals)
		{
			name = _name;
			description = _description;
			type = PinType::FLOAT;
			flags = PinFlags(flags | _flags);
			min = _min;
			max = _max;
			range_type = _range_type;
		}
	};
}