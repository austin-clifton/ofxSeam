#pragma once

#include "pin.h"

namespace seam::pins {
	struct PinIntMeta {
		int min = INT_MIN;
		int max = INT_MAX;
		RangeType range_type = RangeType::LINEAR;
	};

	template <std::size_t N>
	struct PinInt : public PinInput<int, N>, public PinIntMeta {
		PinInt(
			std::string_view _name = "int",
			std::string_view _description = "",
			std::array<int, N> init_vals = { 0 },
			int _min = INT_MIN,
			int _max = INT_MAX,
			RangeType _range_type = RangeType::LINEAR,
			PinFlags _flags = PinFlags::INPUT
		)
			: PinInput<int, N>(init_vals)
		{
			name = _name;
			description = _description;
			type = PinType::INT;
			flags = _flags;
			min = _min;
			max = _max;
			range_type = _range_type;
		}
	};
}