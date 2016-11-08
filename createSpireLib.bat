cd LibraryRelease\
"../BuildTools/CodePack.exe" codegen_separated.xml
cd ..\
copy /Y LibraryRelease\Spire.h ..\GameEngine\GameEngineCore\Spire\Spire.h
copy /Y LibraryRelease\Spire.cpp ..\GameEngine\GameEngineCore\Spire\Spire.cpp
copy /Y LibraryRelease\Basic.h ..\GameEngine\GameEngineCore\Spire\Basic.h
copy /Y LibraryRelease\Basic.cpp ..\GameEngine\GameEngineCore\Spire\Basic.cpp
pause
