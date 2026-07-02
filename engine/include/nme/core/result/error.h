#pragma once

namespace nme 
{
	enum class Error {
		None = 0,

		Unknown = -1,
		InvalidArgs = -2,
		OutOfMemory = -3,
		NotFound = -4,
		AlreadyExists = -5,
		Unsupported = -6,
		NotInitialized = -7,
		AlreadyInitialized = -8,
		Timeout = -9,
		PermissionDenied = -10,
		BufferTooSmall = -11,
		Overflow = -12,
		Internal = -13
	};

	[[nodiscard]] const char *error_toString(Error err);

}  // namespace NME

#define NME_SUCCEEDED(r)  ((r) == nme::Error::None)
#define NME_FAILED(r)	  ((r) != nme::Error::None)
