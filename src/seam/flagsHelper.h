#pragma once

/// meant to be used with enum flag types
namespace seam::flags {
	/// \param in_flags the input list of flags
	/// \param raised_flags the flags which must be raised to return true
	/// \return true if in_flags & raised_flags == raised_flags
	template <typename T>
	bool AreRaised(T in_flags, T raised_flags) {
		return ((in_flags & raised_flags) == raised_flags);
	}



}