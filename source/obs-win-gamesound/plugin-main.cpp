#include "stdafx.h"
#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-win-gamesound", "en-US")

void register_source();

bool obs_module_load(void)
{
    register_source();
    return true;
}
