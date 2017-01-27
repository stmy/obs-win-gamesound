#include "stdafx.h"
#include <obs-module.h>
#include "obs-win-gamesound.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-win-gamesound", "en-US")

bool obs_module_load(void)
{
    register_source();
    return true;
}
