@ECHO OFF
setlocal enabledelayedexpansion
for %%f in (instances\*.json) do (
  ..\build\RelWithDebInfo\mpp.exe %%f solutions -c config.json
)