@echo off

echo %OUTPUT_DIR%
if not exist %OUTPUT_DIR%\FV\SystemFirmwareUpdate.FV echo Hello > %OUTPUT_DIR%\FV\SystemFirmwareUpdate.FV

build -p PlatformPkg\PlatformCapsule.dsc
REM del %OUTPUT_DIR%\FV\FIRMWARE.tmp            > NUL
REM del %OUTPUT_DIR%\FV\SystemFirmwareUpdate.FV > NUL
REM del %OUTPUT_DIR%\FV\bios.bin                > NUL
