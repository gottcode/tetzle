@ECHO OFF

SET APP=Tetzle
SET VERSION=2.0.3

ECHO Copying executable
MKDIR %APP%
TYPE COPYING | FIND "" /V > %APP%\License.txt
TYPE CREDITS | FIND "" /V > %APP%\Credits.txt
COPY release\%APP%.exe %APP% >nul
COPY tools\windows\jpegtran.exe %APP% >nul
COPY tools\windows\jhead.exe %APP% >nul
strip %APP%\%APP%.exe

ECHO Copying translations
SET TRANSLATIONS=%APP%\translations
MKDIR %TRANSLATIONS%
COPY translations\*.qm %TRANSLATIONS% >nul
COPY %QTDIR%\translations\qt_*.qm %TRANSLATIONS% >nul

ECHO Copying Qt libraries
COPY %QTDIR%\bin\libgcc_s_dw2-1.dll %APP% >nul
COPY %QTDIR%\bin\mingwm10.dll %APP% >nul
COPY %QTDIR%\bin\QtCore4.dll %APP% >nul
COPY %QTDIR%\bin\QtGui4.dll %APP% >nul
COPY %QTDIR%\bin\QtOpenGL4.dll %APP% >nul

ECHO Copying Qt image plugins
MKDIR %APP%\imageformats
COPY %QTDIR%\plugins\imageformats\qgif4.dll %APP%\imageformats >nul
COPY %QTDIR%\plugins\imageformats\qico4.dll %APP%\imageformats >nul
COPY %QTDIR%\plugins\imageformats\qjpeg4.dll %APP%\imageformats >nul
COPY %QTDIR%\plugins\imageformats\qmng4.dll %APP%\imageformats >nul
COPY %QTDIR%\plugins\imageformats\qtiff4.dll %APP%\imageformats >nul

ECHO Creating compressed file
CD %APP%
7z a %APP%_%VERSION%.zip * >nul
CD ..
MOVE %APP%\%APP%_%VERSION%.zip . >nul

ECHO Cleaning up
RMDIR /S /Q %APP%
