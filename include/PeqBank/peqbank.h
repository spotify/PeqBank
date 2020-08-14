// Copyright (c) 2020 Spotify AB.
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//

#ifndef peqbank_h
#define peqbank_h

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <malloc.h> // for alloca
#define alloca _alloca
#endif

#ifdef WIN_VERSION
#define sinhf sinh
#define sqrtf sqrt
#define tanf tan
#define expf exp
#endif

#define FLUSH_TO_ZERO(fv) (((*(unsigned int *)&(fv)) & 0x7f800000) == 0) ? 0.0f : (fv)
#ifndef max
#define max(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })
#endif
#ifndef min
#define min(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })
#endif

#undef PI     // so we can use the one below instead of the one from math.h
#undef TWOPI  // ditto

#define LOG_10 2.30258509299405f  // ln(10)
#define LOG_2 0.69314718055994f   // ln(2)
#define LOG_22 0.34657359027997f  // ln(2)/2
#define PI 3.14159265358979f      // PI
#define PI2 9.86960440108936f     // PI squared
#define TWOPI 6.28318530717959f   // 2 * PI
#define SMALL 0.000001
#define NBCOEFF 5
#define FAST 1
#define SMOOTH 0
#define USESHELF 0
#define NOSHELF NBCOEFF
#define MAXELEM 16
#define MINORDER 2
#define MAXORDER (MAXELEM * 2)

enum { LOWPASS, HIGHPASS };
enum { LPHP, SHELF, PEQ, NONE };

typedef struct _filter {
  int type;
  void *filter;
} t_filter;

typedef struct _peq {
  float freq_peak;
  float bandwidth;
  float gain_dc;
  float gain_peak;
  float gain_bandwidth;
} t_peq;

typedef struct _shelf {
  float gain_low;
  float gain_middle;
  float gain_high;
  float freq_low;
  float freq_high;
} t_shelf;

typedef struct _lphp {
  float freq;    // cutoff frequency in Hz -- must be < sampling rate /2
  float ripple;  // percent ripple (e.g. 0.5) -- must be > 0 and < 29
  int order;     // filter order (e.g. 2) -- must be even number >=2 and <= 2*MAXELEM
  int type;      // LOWPASS (0) or HIGHPASS (1)
} t_lphp;

typedef struct _peqbank {
  t_filter **filters;  // Ptr on list of filters (e.g. shelf, peq, lowpass, highpass)

  float *coeff;     // Pointers for smoothly interpolating between biquad coefficients
  float *oldcoeff;  // There are 5 coeffs per biquad. There can be multiple biquads per filter.
  float *newcoeff;
  float *freecoeff;

  float b_Fs;      // Sample rate
  int b_channels;  // Number of audio channels to process in parallel
  int b_max;       // Max number of biquads (used to allocate memory)
  int b_nbiquads;  // Actual number of biquads

  int b_mode;         // SMOOTH (0) or FAST (1)
  float *b_ym1;       // Ptr on y minus 1 per biquad, per channel
  float *b_ym2;       // Ptr on y minus 2 per biquad, per channel
  float *b_xm1;       // Ptr on x minus 1 per biquad, per channel
  float *b_xm2;       // Ptr on x minus 2 per biquad, per channel
  float **s_vec_in;   // Input buffers
  float **s_vec_out;  // Output buffers
  float **s_vec_bak;  // Pointer to memory alocated for output buffer if in-place filtering happens
  int s_n;            // Size buffer

} t_peqbank;

float peqbank_pow10(float x);
float peqbank_pow2(float x);
void peqbank_allocmem(t_peqbank *x);
void peqbank_resize_buffer(t_peqbank *x, int buffer_size);
void peqbank_freemem(t_peqbank *x);
void peqbank_clear(t_peqbank *x);
void peqbank_init(t_peqbank *x);
t_peqbank *peqbank_new(int sampling_rate, int num_channels, int buffer_size);
void peqbank_print_info(t_peqbank *x);
int do_peqbank_perform_fast(t_peqbank *x);
int peqbank_perform_fast(t_peqbank *x);
int peqbank_perform_smooth(t_peqbank *x);
int16_t sampleLimiter(int samp);
int peqbank_callback_int16(t_peqbank *x, int16_t *sig_input, int16_t *sig_output);
int peqbank_callback_float(t_peqbank *x, float *sig_input, float *sig_output);
void compute_shelf(t_peqbank *x, t_shelf *s, int index);
void compute_peq(t_peqbank *x, t_peq *p, int index);
void compute_lphp(t_peqbank *x, t_lphp *f, int index);
void swap_in_new_coeffs(t_peqbank *x);
void peqbank_compute(t_peqbank *x);
void peqbank_reset(t_peqbank *x);
t_filter *new_shelf(
    float gain_low, float gain_middle, float gain_high, float freq_low, float freq_high);
t_filter *new_peq(
    float freq_peak, float bandwidth, float gain_dc, float gain_peak, float gain_bandwidth);
t_filter *new_lphp(float freq, float ripple, int order);
t_filter *new_lowpass(float freq, float ripple, int order);
t_filter *new_highpass(float freq, float ripple, int order);
t_filter **new_filters(int num_filters);
void free_filters(t_filter **filters);
void peqbank_setup(t_peqbank *x, t_filter **filters);

#endif  // peqbank_h
