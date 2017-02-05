**[New WASAPI application capturing plugin is available here!](https://github.com/stmy/win-wasapi-capture)**

# obs-win-gamesound

Plugin for OBS-Studio to capture sound of game application.

Currently only support for 64-bit games which uses XAudio2.7 on Windows.

## WARNING

*This is a very very WIP version and tested only on DX11 version of FFXIV benchmark.*

## How to use

* Put `winmm.dll` and `xaudio2_7-hook.dll` into next to game's exe files.
* Put `obs-win-gamesound.dll` into `obs-plugins\[32|64]bit`.

## How to build

* Open Property Manager on the Visual Studio
* Open 'Variables' property sheet
* Set user macros
    * `BOOST_INCLUDE`: Path to BOOST include directory
    * `BOOST_LIB_[X86|X64]`: Path to BOOST library directory
    * `OBS_SRC`: Path to obs-studio source code directory
    * `LIBOBS_[X86|X64]_[DEBUG|RELEASE]`: Path to `obs.lib` static library file (e.g. `...\obs-studio\build64\libobs\Release\obs.lib`)
    * `OBS_RUNDIR_[X86|X64]`: Path to run directory (e.g. `...\obs-studio\build64\rundir\Release\`)
* Run build

## License

MIT
