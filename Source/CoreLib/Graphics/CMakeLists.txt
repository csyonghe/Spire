add_library(CoreLib_Graphics
 ObjModel.cpp
 ObjModel.h
 BezierMesh.h
 BezierMesh.cpp
 Camera.h
 Camera.cpp
)

target_link_libraries(CoreLib_Graphics CoreLib_Basic)