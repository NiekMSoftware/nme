#include <nme/core/result/error.h>

namespace nme
{

	const char *error_toString(const Error err) {
		switch (err)
		{
		case Error::None:				return "None";
		case Error::Unknown:			return "Unknown";
		case Error::OutOfMemory:		return "OutOfMemory";
		case Error::NotFound:			return "NotFound";
		case Error::AlreadyExists:		return "AlreadyExists";
		case Error::Unsupported:		return "Unsupported";
		case Error::NotInitialized:		return "NotInitialized";
		case Error::Timeout:			return "Timeout";
		case Error::PermissionDenied:	return "PermissionDenied";
		case Error::BufferTooSmall:		return "BufferTooSmall";
		case Error::Overflow:			return "Overflow";
		case Error::Internal:			return "Internal";
        case Error::InvalidArgs:        return "InvalidArgs";
        case Error::AlreadyInitialized: return "AlreadyInitialized";

		default: return "";
        }
    }

}	// namespace nme