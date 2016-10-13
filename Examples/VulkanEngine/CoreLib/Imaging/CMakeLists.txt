add_library(CoreLib_Imaging
 Bitmap.cpp
 stb_image.c
 TextureData.cpp
)

target_link_libraries(CoreLib_Imaging CoreLib_Basic)