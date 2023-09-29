
mkdir -p build
gcc asteroids.c -g -static -o ./build/asteroids.dll -shared -I../core -Wno-incompatible-pointer-types -lgdi32 -lopengl32
