REM -------------------
REM Compile EA0 BIOS
REM -------------------
SET BIOS_TARGET_DIR=%cd%
@REM # Build EA0   

if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EA0_03-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile EB0-00 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EB0_00-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile EB0-11 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EB0_11-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile EC0-01 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EC0_01-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile EC0-10 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EC0_10-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile ED0-02 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-ED0_02-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile ED0-10 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-ED0_10-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile EE0-04 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EE0_04-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile EE0-05 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EE0_05-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile EH0-IOE-03 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EH0_01_IOE_03-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile EK0 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EK0_03-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile EL0 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EL0_05-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -------------------
REM Compile EH0-IOE-23 BIOS
REM -------------------
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EH0_01_IOE_23-D.cmd
copy .\HX0*.bin .\RDBios-bin\

REM -----------------------------------------------
REM Change all bin files to ROM files
REM -----------------------------------------------
ren %BIOS_TARGET_DIR%\RDBios-bin\*.bin *.ROM


