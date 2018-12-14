@echo off

echo [ZXE]

set MAP_DRV_NO=Y

if exist %MAP_DRV_NO%: subst %MAP_DRV_NO%: /d 
subst %MAP_DRV_NO%: .

cd /d %MAP_DRV_NO%:


set PATH=%PATH%;%CD%\ZxCmd
echo A0-EA0_03-D / A0-EA0_03-R        // EA0


cmd /k

