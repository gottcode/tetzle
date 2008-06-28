MKDIR Tetzle
COPY release\Tetzle.exe Tetzle
COPY %QTDIR%\bin\mingwm10.dll Tetzle
COPY %QTDIR%\bin\QtCore4.dll Tetzle
COPY %QTDIR%\bin\QtGui4.dll Tetzle
COPY %QTDIR%\bin\QtXml4.dll Tetzle
COPY %QTDIR%\bin\QtOpenGL4.dll Tetzle
MKDIR Tetzle\imageformats
COPY %QTDIR%\plugins\imageformats\qgif4.dll Tetzle\imageformats
COPY %QTDIR%\plugins\imageformats\qico4.dll Tetzle\imageformats
COPY %QTDIR%\plugins\imageformats\qjpeg4.dll Tetzle\imageformats
COPY %QTDIR%\plugins\imageformats\qmng4.dll Tetzle\imageformats
COPY %QTDIR%\plugins\imageformats\qsvg4.dll Tetzle\imageformats
COPY %QTDIR%\plugins\imageformats\qtiff4.dll Tetzle\imageformats
