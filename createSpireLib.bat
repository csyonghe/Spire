cd LibraryRelease\
"../BuildTools/CodePack.exe" codegen.xml
type Spire.h >> SpireImpl.cpp
type Spire.cpp >> SpireImpl.cpp
copy /Y ..\Source\SpireLib\include\Spire.h Spire.h
del Spire.cpp
ren SpireImpl.cpp Spire.cpp
cd ..\