/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifdef DATA_TYPE_DOUBLE
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#define FLOAT_TYPE double
#define LITERAL(value) value
#else
#define FLOAT_TYPE float
#define LITERAL(value) value##f
#endif

FLOAT_TYPE cumulative_normal_distribution(const FLOAT_TYPE d) {
    const FLOAT_TYPE a1 = LITERAL(0.31938153);
    const FLOAT_TYPE a2 = LITERAL(-0.356563782);
    const FLOAT_TYPE a3 = LITERAL(1.781477937);
    const FLOAT_TYPE a4 = LITERAL(-1.821255978);
    const FLOAT_TYPE a5 = LITERAL(1.330274429);
    const FLOAT_TYPE rsqrt2pi = LITERAL(0.39894228040143267793994605993438);

    const FLOAT_TYPE k = LITERAL(1.0) / (LITERAL(1.0) + LITERAL(0.2316419) * fabs(d));
    const FLOAT_TYPE value = rsqrt2pi * exp(LITERAL(-0.5) * d * d) * (k * (a1 + k * (a2 + k * (a3 + k * (a4 + k * a5)))));
    return d > LITERAL(0.0) ? LITERAL(1.0) - value : value;
}

kernel void black_scholes(const FLOAT_TYPE riskFree, const FLOAT_TYPE volatility, global const FLOAT_TYPE *years, global const FLOAT_TYPE *strike,
                          global const FLOAT_TYPE *stockPrice, global FLOAT_TYPE *call, global FLOAT_TYPE *put) {
    const int id = get_global_id(0);
    const FLOAT_TYPE sqrtYears = sqrt(years[id]);
    const FLOAT_TYPE d1 = (log(stockPrice[id] / strike[id]) + (riskFree + LITERAL(0.5) * volatility * volatility) * years[id]) / (volatility * sqrtYears);
    const FLOAT_TYPE d2 = d1 - volatility * sqrtYears;
    const FLOAT_TYPE expRT = exp(-riskFree * years[id]);

    call[id] = stockPrice[id] * cumulative_normal_distribution(d1) - strike[id] * expRT * cumulative_normal_distribution(d2);
    put[id] = call[id] + strike[id] * expRT - stockPrice[id];
}
