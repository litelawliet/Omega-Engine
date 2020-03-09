@echo off

setlocal
set GLSL_COMPILER=glslangValidator.exe
set SOURCE_FOLDER=""
set BINARIES_FOLDER="bin/"

:: raygen shaders
%GLSL_COMPILER% -t -V %SOURCE_FOLDER%ray_gen.rgen -o %BINARIES_FOLDER%ray_gen.spv

:: closest hit shaders
%GLSL_COMPILER% -t -V %SOURCE_FOLDER%ray_chit.rchit -o %BINARIES_FOLDER%ray_chit.spv

:: miss shaders
%GLSL_COMPILER% -t -V %SOURCE_FOLDER%ray_miss.rmiss -o %BINARIES_FOLDER%ray_miss.spv

:: rasterizer shaders
%GLSL_COMPILER% -t -V %SOURCE_FOLDER%rast_vert.vert -o %BINARIES_FOLDER%rast_vert.spv
%GLSL_COMPILER% -t -V %SOURCE_FOLDER%rast_frag.frag -o %BINARIES_FOLDER%rast_frag.spv

pause