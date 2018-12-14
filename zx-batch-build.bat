SET BIOS_TARGET_DIR=%cd%

@REM # Build EA0_03
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EA0_03-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\
@REM # 

@REM # Build ED0_02
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-ED0_02-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\
@REM # 
@REM # Build ED0_10
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-ED0_10-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\
@REM # 

@REM # Build EH0_01_IOE_03
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EH0_01_IOE_03-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\
@REM #
@REM # Build EH0_01_IOE_23
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EH0_01_IOE_23-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\
@REM # 

@REM # Build EK0_03
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EK0_03-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\
@REM #  

@REM # Build EB0_00
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EB0_00-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\
@REM # Build EB0_11
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EB0_11-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\

@REM # Build EC0_01
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EC0_01-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\
@REM # Build EC0_10
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EC0_10-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\

@REM # Build EE0_04
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EE0_04-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\
@REM # Build EE0_05
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EE0_05-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\
@REM #

@REM # Build EL0_05
if exist %BIOS_TARGET_DIR%\Build\PlatformPkg\*.*  rd /s/q %BIOS_TARGET_DIR%\Build\PlatformPkg
call A0-EL0_05-R.cmd
copy /y %BIOS_TARGET_DIR%\*.bin %BIOS_TARGET_DIR%\release\
@REM # 


