copy ..\*.exe .\
copy ..\*.txt .\
set PATH=%PATH%;%ProgramFiles%\7-zip;%ProgramFiles%\Nsis
mkdir bin
7z a -y -tzip bin\volfix.zip volfix.exe
mkdir setup
makensis volfix.nsi
move *Setup.exe setup\
del /Q *.exe
del /Q *.txt
