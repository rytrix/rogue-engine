# rogue-engine

A heavily work in progress game and game engine written in c++ and opengl

### Current Features
- Scene entity heirachy with automatic rendering optimizations (indirect rendering, instancing, bindless textures)
- PBR lighting, shadows, and model loading with support for skeletal animations
- Physics engine (jolt physics)

# Building

has various presets that can be used with cmake --preset=config
example:
 
cmake --preset=linux-clang-debug &&
ninja -C build/debug
