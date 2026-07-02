#include <nme/version.h>
#include <nme/platform/platform.h>

#ifndef NME_VERSION_MAJOR
	#define NME_VERSION_MAJOR 0
#endif
#ifndef NME_VERSION_MINOR
	#define NME_VERSION_MINOR 0
#endif
#ifndef NME_VERSION_PATCH
	#define NME_VERSION_PATCH 0
#endif

#define NME_STR_IMPL(x) #x
#define NME_STR(x)      NME_STR_IMPL(x)

namespace nme {

	const char *engine_version() {
		return "nme " NME_STR(NME_VERSION_MAJOR)
			   "."    NME_STR(NME_VERSION_MINOR)
			   "."    NME_STR(NME_VERSION_PATCH);
	}

}