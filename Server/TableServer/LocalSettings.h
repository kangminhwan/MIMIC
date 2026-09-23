#pragma once
#include <stdexcept>
#if __has_include("LocalSettings.local.h")
#include "LocalSettings.local.h"
#else
#include "LocalSettings.example.h"
#endif
