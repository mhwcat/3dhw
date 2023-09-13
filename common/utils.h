#pragma once

#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define LOG3DHW(...) { TCHAR buffer[1024] = { '\0' }; snprintf(buffer, 1024, __VA_ARGS__); OutputDebugString(buffer); OutputDebugString("\n"); }
#else
#define LOG3DHW(...) { char buffer[1024] = { '\0' }; int _result = snprintf(buffer, 1024, __VA_ARGS__); printf("%s\n", buffer); }
#endif

#define M_PI 3.14159265358979323846

inline static int clampi(int val, int min, int max) {
    const int t = val < min ? min : val;
    
    return t > max ? max : t;
}

#ifndef _WIN32
#include <time.h>
// Calculate nanoseconds difference between two clock_gettime() results
inline static long nanosecDiff(struct timespec start, struct timespec end) {
    if ((end.tv_nsec - start.tv_nsec) < 0) {
        return 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        return end.tv_nsec - start.tv_nsec;
    }
}
#endif