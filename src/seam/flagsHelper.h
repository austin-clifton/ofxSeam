#pragma once

/// meant to be used with enum flag types
namespace seam::flags {
	/// \param in_flags the input list of flags
	/// \param raised_flags the flags which must be raised to return true
	/// \return true if in_flags & raised_flags == raised_flags
	template <typename T>
	bool AreRaised(T in_flags, T raised_flags) {
		return (T((uint64_t)in_flags & (uint64_t)raised_flags) == raised_flags);
	}
}

#define DeclareFlagOperators(enumFlagType, underlyingDataType) \
	enumFlagType operator|(enumFlagType lhs, enumFlagType rhs); \
	enumFlagType operator&(enumFlagType lhs, enumFlagType rhs); \
	enumFlagType operator~(enumFlagType flag);

#define DefineFlagOperators(enumFlagType, underlyingDataType) \
	enumFlagType operator|(enumFlagType lhs, enumFlagType rhs) { \
		return static_cast<enumFlagType>( \
			static_cast<underlyingDataType>(lhs) | static_cast<underlyingDataType>(rhs) \
		); \
	} \
	enumFlagType operator&(enumFlagType lhs, enumFlagType rhs) { \
		return static_cast<enumFlagType>( \
			static_cast<underlyingDataType>(lhs) & static_cast<underlyingDataType>(rhs) \
		); \
	} \
	enumFlagType operator~(enumFlagType flag) { \
		return static_cast<enumFlagType>( \
			~static_cast<underlyingDataType>(flag) \
		); \
	}
