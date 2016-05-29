@echo off
call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" amd64
cl.exe /nologo /D_USRDLL /D_WINDLL /MD /O2 /EHsc /fp:fast /Zi %1 /link /DLL /OUT:%2
@echo on