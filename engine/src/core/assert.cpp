#include <../../include/nme/platform/debug/assert.h>

#include <cstdio>  // std::snprintf, std::fputs, std::fflush, stderr

#if NME_PLATFORM_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>	// OutputDebugStringA -> VS Output Window
#endif

namespace nme::detail {
	void nme_assert_handleFailure(const char *expr, const char *file,
		const char *func, const i32 line) {
		// Fixed stack buffer: the handler must never allocate. An assertion can be
		// triggered by an allocation failure, or fire before any allocator exists.
		char buffer[1024];
		std::snprintf(buffer, sizeof(buffer),
			"\n[assert] %s\n        at %s:%d\n        in %s\n\n",
			expr, file, line, func);

		// Rawest sink only. Deliberately NOT the logging subsystem: assert must
		// work when logging is uninitialized, broken, or is itself asserting.
		std::fputs(buffer, stderr);
		std::fflush(stderr);

#if NME_PLATFORM_WINDOWS
		OutputDebugStringA(buffer);
#endif
	}
}
