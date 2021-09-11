#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <Windows.h>
#include <algorithm>
#include <cstdint>
#include <cctype>
#include <functional>
#include <thread>
#include <vector>
#include <array>
#include <map>
#include <deque>
#include <fstream>
#include <Psapi.h>
#include <sstream>
#include <libloaderapi.h>
#include <cmath>
#include <memory>
#include <stack>
#include <iostream>
#include <TlHelp32.h>
#include <any>
#include <array>
#include <filesystem>
#include <winternl.h>

#include "utils/xorstr.h"
#include "utils/lazy_importer.h"
#include "utils/memory.h"