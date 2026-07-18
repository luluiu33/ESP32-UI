#pragma once

#define FW_MAJOR 0
#define FW_MINOR 0
#define FW_PATCH 0

#define FW_VERSION_STR  "v" STR(FW_MAJOR) "." STR(FW_MINOR) "." STR(FW_PATCH)
#define STR(x)          _STR(x)
#define _STR(x)         #x
