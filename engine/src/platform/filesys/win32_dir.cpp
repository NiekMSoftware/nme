#include "nme/platform/filesys/dir.h"

#include "nme/platform/error/result.h"
#include "nme/platform/filesys/file.h"   // FileError
#include "nme/platform/filesys/path.h"   // StrView, kMaxPath
#include "nme/platform/types.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstring>

namespace nme::fs {



}  // namespace nme::fs