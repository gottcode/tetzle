@echo off

echo Copying executable
mkdir Tetzle
copy release\Tetzle.exe Tetzle
copy tools\windows\jpegtran.exe Tetzle
copy tools\windows\jhead.exe Tetzle
strip release\Tetzle.exe

echo Copying libraries
copy %QTDIR%\bin\libgcc_s_dw2-1.dll Tetzle
copy %QTDIR%\bin\mingwm10.dll Tetzle
copy %QTDIR%\bin\QtCore4.dll Tetzle
copy %QTDIR%\bin\QtGui4.dll Tetzle
copy %QTDIR%\bin\QtXml4.dll Tetzle
copy %QTDIR%\bin\QtOpenGL4.dll Tetzle
mkdir Tetzle\imageformats
copy %QTDIR%\plugins\imageformats\qgif4.dll Tetzle\imageformats
copy %QTDIR%\plugins\imageformats\qico4.dll Tetzle\imageformats
copy %QTDIR%\plugins\imageformats\qjpeg4.dll Tetzle\imageformats
copy %QTDIR%\plugins\imageformats\qmng4.dll Tetzle\imageformats
copy %QTDIR%\plugins\imageformats\qsvg4.dll Tetzle\imageformats
copy %QTDIR%\plugins\imageformats\qtiff4.dll Tetzle\imageformats
