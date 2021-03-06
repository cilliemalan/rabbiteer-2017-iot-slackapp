
#include <cstdio>
#include <cstdlib>

#include <string>
#include <array>
#include <chrono>
#include <thread>
#include <codecvt>
#include <atomic>
#include <regex>
#include <map>
#include <vector>

#include <cpprest/http_client.h>
#include <cpprest/ws_client.h>
#include <cpprest/filestream.h>

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) && !CPPREST_FORCE_PPLX
#else
#include <pplx/pplx.h>
#endif

#include <pplx/pplxtasks.h>

#include <gif_lib.h>
#include <png.h>
