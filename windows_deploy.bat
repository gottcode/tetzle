@ECHO ON>..\tetzle\windows\dirs.nsh
@ECHO ON>..\tetzle\windows\files.nsh
@ECHO OFF

SET SRCDIR=..\tetzle
SET APP=Tetzle
SET VERSION=2.2.0

ECHO Copying executable
MKDIR %SRCDIR%\%APP%
COPY %APP%.exe %SRCDIR%\%APP%\%APP%.exe >nul

ECHO Copying translations
SET TRANSLATIONS=%SRCDIR%\%APP%\translations
MKDIR %TRANSLATIONS%
COPY *.qm %TRANSLATIONS% >nul

CD %SRCDIR%

ECHO Copying Qt
%QTDIR%\bin\windeployqt.exe --verbose 0 %APP%\%APP%.exe
RMDIR /S /Q %APP%\iconengines

ECHO Creating ReadMe
TYPE README >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
ECHO CREDITS >> %APP%\ReadMe.txt
ECHO ======= >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
TYPE CREDITS >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
ECHO NEWS >> %APP%\ReadMe.txt
ECHO ==== >> %APP%\ReadMe.txt
ECHO. >> %APP%\ReadMe.txt
TYPE ChangeLog >> %APP%\ReadMe.txt

ECHO Creating installer
CD %APP%
SETLOCAL EnableDelayedExpansion
SET "parentfolder=%__CD__%"
FOR /R . %%F IN (*) DO (
  SET "var=%%F"
  ECHO Delete "$INSTDIR\!var:%parentfolder%=!" >> ..\windows\files.nsh
)
FOR /R /D %%F IN (*) DO (
  TYPE ..\windows\dirs.nsh > temp.txt
  SET "var=%%F"
  ECHO RMDir "$INSTDIR\!var:%parentfolder%=!" > ..\windows\dirs.nsh
  TYPE temp.txt >> ..\windows\dirs.nsh
)
DEL temp.txt
ENDLOCAL
CD ..
makensis.exe /V0 windows\installer.nsi

ECHO Cleaning up
RMDIR /S /Q %APP%
DEL windows\dirs.nsh
DEL windows\files.nsh
