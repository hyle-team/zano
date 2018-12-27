call utils/build_script_windows.bat
IF %ERRORLEVEL% NEQ 0 (
  @echo "FAILED TO build_script_windows.bat"
  goto error
)


call utils/build_script_windows32.bat
IF %ERRORLEVEL% NEQ 0 (
  @echo "FAILED TO build_script_windows32.bat"
  goto error
)



goto success

:error
echo "BUILD FAILED"
exit /B %ERRORLEVEL%

:success
echo "BUILD SUCCESS"

