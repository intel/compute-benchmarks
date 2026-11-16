/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define MAX_ITERATIONS 50

float get_color(const uint iterations, const uint maxIterations) {
    float pixel = 0.0f;
    if (iterations != maxIterations) {
        const float factor = sqrt((float)iterations / (float)maxIterations);
        const float rawValue = round((float)maxIterations * factor);
        pixel = rawValue / (float)maxIterations;
    }
    return pixel;
}

kernel void mandelbrot(global float *pixels, int width, int height) {
    const uint x = get_global_id(0);
    const uint y = get_global_id(1);

    const float2 uv = {(float)x / (float)width, (float)y / (float)height};
    const float2 offset = {1.75f, 1.25f};
    const float2 c = (uv * 2.5f) - offset;
    float2 z = 0.0f;

    int i;
    for (i = 0; i < MAX_ITERATIONS; ++i) {
        if (dot(z, z) > 4.0f) {
            break;
        }
        const float real = z.x * z.x - z.y * z.y + c.x;
        z.y = 2.0f * z.x * z.y + c.y;
        z.x = real;
    }

    pixels[y * width + x] = get_color(i, MAX_ITERATIONS);
}
