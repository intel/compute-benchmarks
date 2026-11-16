/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void sobel(global const uint *input, global uint *output, int width, int height) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int offset = y * width + x;

    if (x < 1 || x >= width - 1 || y < 1 || y >= height - 1) {
        output[offset] = input[offset];
        return;
    }

    const int upperLeft = input[offset - 1 - width];
    const int upperMiddle = input[offset - width];
    const int upperRight = input[offset + 1 - width];
    const int middleLeft = input[offset - 1];
    const int middleRight = input[offset + 1];
    const int lowerLeft = input[offset - 1 + width];
    const int lowerMiddle = input[offset + width];
    const int lowerRight = input[offset + 1 + width];

    const float horizontal = (float)(-upperLeft - 2 * middleLeft - lowerLeft + upperRight + 2 * middleRight + lowerRight);
    const float vertical = (float)(-upperLeft - 2 * upperMiddle - upperRight + lowerLeft + 2 * lowerMiddle + lowerRight);
    const uint gradient = (uint)sqrt(horizontal * horizontal + vertical * vertical);
    output[offset] = min(gradient, 255u);
}
