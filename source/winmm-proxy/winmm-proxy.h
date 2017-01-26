#pragma once

#include <string>

std::wstring get_original_winmm();
bool winmm_proxy_init();
void winmm_proxy_free();