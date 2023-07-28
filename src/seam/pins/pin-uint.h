#pragma once

#include "pin.h"

/*
namespace seam::pins {


	template <std::size_t N>
	struct PinUint : public PinInput<uint32_t, N>, public PinUintMeta {
		PinUint(
			std::string_view _name = "uint",
			std::string_view _description = "",
			std::array<uint32_t, N> init_vals = { 0 },
			uint32_t _min = 0,
			uint32_t _max = UINT_MAX,
			RangeType _range_type = RangeType::LINEAR,
			PinFlags _flags = PinFlags::INPUT
			)
			: PinInput<uint32_t, N>(init_vals)
		{
			name = _name;
			description = _description;
			type = PinType::UINT;
			flags = PinFlags(flags | _flags);
			min = _min;
			max = _max;
			range_type = _range_type;
		}
	};
}
*/