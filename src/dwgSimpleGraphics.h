#pragma once

#include <stdint.h>
#include "vectormath.hpp"

#define DWG_PI 3.14159265358979323846

// call once at the beginning of app
bool dwgInitApp(int32_t width, int32_t height, const char* title);

// call inside while loop condition
bool dwgShouldClose();

// call at the end of while loop
void dwgRender(const Matrix4& camera, const float fov);

// call at the end of app
void dwgReleaseApp();

// returns this frame's delta time in seconds
float dwgDeltaTime();

// returns global time passed, since the beginning of app
double dwgGlobalTime();

// add debug line to this frame
void dwgDebugLine(const Vector3& start, const Vector3& end, const Vector3& color);

// add debug sphere to this frame
void dwgDebugSphere(const Vector3& position, const Vector3& scale, const Vector3& color);
