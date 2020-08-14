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

#include "PeqBank/peqbank.h"

float peqbank_pow10(float x) {
  return expf(LOG_10 * x);
}

float peqbank_pow2(float x) {
  return expf(LOG_2 * x);
}

void peqbank_allocmem(t_peqbank *x) {
  // alocate and initialize memory
  x->s_vec_in = (float **)malloc(x->b_channels * sizeof(float *));
  x->s_vec_out = (float **)malloc(x->b_channels * sizeof(float *));
  x->s_vec_bak = x->s_vec_out;
  for (int i = 0; i < x->b_channels; i++) {
    x->s_vec_in[i] = malloc(x->s_n * sizeof(float));
    x->s_vec_out[i] = malloc(x->s_n * sizeof(float));
  }
  x->coeff = (float *)malloc(x->b_max * NBCOEFF * sizeof(*x->coeff));
  x->oldcoeff = x->coeff;
  x->newcoeff = (float *)malloc(x->b_max * NBCOEFF * sizeof(*x->newcoeff));
  x->freecoeff = (float *)malloc(x->b_max * NBCOEFF * sizeof(*x->freecoeff));
  x->b_ym1 = (float *)malloc(x->b_max * x->b_channels * sizeof(*x->b_ym1));
  x->b_ym2 = (float *)malloc(x->b_max * x->b_channels * sizeof(*x->b_ym2));
  x->b_xm1 = (float *)malloc(x->b_max * x->b_channels * sizeof(*x->b_xm1));
  x->b_xm2 = (float *)malloc(x->b_max * x->b_channels * sizeof(*x->b_xm2));
  if (x->coeff == NULL || x->newcoeff == NULL || x->freecoeff == NULL || x->b_ym1 == NULL ||
      x->b_ym2 == NULL || x->b_xm1 == NULL || x->b_xm2 == NULL) {
    printf("Warning: not enough memory. Expect to crash soon.\n");
  }
}

void peqbank_resize_buffer(t_peqbank *x, int buffer_size) {
  for (int i = 0; i < x->b_channels; i++) {
    free((char *)x->s_vec_in[i]);
    free((char *)x->s_vec_bak[i]);
  }
  free((char *)x->s_vec_in);
  free((char *)x->s_vec_bak);

  x->s_n = buffer_size;
  x->s_vec_in = (float **)malloc(x->b_channels * sizeof(float *));
  x->s_vec_out = (float **)malloc(x->b_channels * sizeof(float *));
  x->s_vec_bak = x->s_vec_out;
  for (int i = 0; i < x->b_channels; i++) {
    x->s_vec_in[i] = malloc(x->s_n * sizeof(float));
    x->s_vec_out[i] = malloc(x->s_n * sizeof(float));
  }
}

void peqbank_freemem(t_peqbank *x) {
  if (x->coeff != x->oldcoeff) free((char *)x->oldcoeff);
  free((char *)x->coeff);
  free((char *)x->newcoeff);
  if (x->freecoeff) free((char *)x->freecoeff);
  free((char *)x->b_ym1);
  free((char *)x->b_ym2);
  free((char *)x->b_xm1);
  free((char *)x->b_xm2);
  for (int i = 0; i < x->b_channels; i++) {
    free((char *)x->s_vec_in[i]);
    free((char *)x->s_vec_bak[i]);
  }
  free((char *)x->s_vec_in);
  free((char *)x->s_vec_bak);
}

void peqbank_clear(t_peqbank *x) {
  for (int i = 0; i < (x->b_max * x->b_channels); ++i) {
    x->b_ym1[i] = 0.0f;
    x->b_ym2[i] = 0.0f;
    x->b_xm1[i] = 0.0f;
    x->b_xm2[i] = 0.0f;
  }
}

void peqbank_init(t_peqbank *x) {
  for (int i = 0; i < x->b_max * NBCOEFF; ++i) {
    x->coeff[i] = 0.0f;
    x->oldcoeff[i] = 0.0f;
    x->newcoeff[i] = 0.0f;
    x->freecoeff[i] = 0.0f;
  }
  peqbank_clear(x);
}

t_peqbank *peqbank_new(int sampling_rate, int num_channels, int buffer_size) {
  t_peqbank *x = (t_peqbank *)malloc(sizeof(t_peqbank));

  if (!x) {
    return NULL;
  }

  x->b_mode = SMOOTH;  // Default
  x->b_max = MAXELEM;
  x->b_Fs = (float)sampling_rate;
  x->b_channels = num_channels;
  x->s_n = buffer_size;

  peqbank_allocmem(x);
  peqbank_init(x);

  return (x);
}

void peqbank_print_info(t_peqbank *x) {
  if (x->b_mode == SMOOTH) {
    printf("Smooth Mode: Coefficients linearly interpolated over one buffer\n");
  } else if (x->b_mode == FAST) {
    printf("Fast Mode: No interpolation when filter parameters change\n");
  } else {
    printf("ERROR: object is in neither FAST mode nor SMOOTH mode!\n");
  }

  printf("Audio sampling rate: %.0f Hz\n", x->b_Fs);
  printf("Number of audio channels: %d\n", x->b_channels);
  printf("Max number of biquads: %d\n", x->b_max);

  int i = 0;
  int c = 0;
  while (x->filters[i]->type != NONE) {
    switch (x->filters[i]->type) {
      case SHELF: {
        t_shelf *s = x->filters[i]->filter;
        printf("Filter %2d | Shelving EQ | Params: %.2f dB, %.2f dB, %.2f dB, %.2f Hz, %.2f Hz\n",
               i + 1,
               s->gain_low,
               s->gain_middle,
               s->gain_high,
               s->freq_low,
               s->freq_high);
        printf(
            "          | i.e. %.1f dB below %.0f Hz, %.1f dB in range %.0f - %.0f Hz, %.1f dB "
            "above %.0f Hz\n",
            s->gain_low,
            s->freq_low,
            s->gain_middle,
            s->gain_low,
            s->freq_high,
            s->gain_high,
            s->freq_high);
        printf("          | Coeffs: [%f %f %f %f %f]\n",
               x->coeff[c],
               x->coeff[c + 1],
               x->coeff[c + 2],
               x->coeff[c + 3],
               x->coeff[c + 4]);
        c += NBCOEFF;
        break;
      }
      case PEQ: {
        t_peq *p = x->filters[i]->filter;
        printf(
            "Filter %2d | Parametric EQ | Params: %.2f Hz, %.2f oct, %.2f dB, %.2f dB, %.2f dB\n",
            i + 1,
            p->freq_peak,
            p->bandwidth,
            p->gain_dc,
            p->gain_peak,
            p->gain_bandwidth);
        printf(
            "          | i.e. %.1f dB at DC, peak of %.2f dB at %.2f Hz with %.2f oct bandwidth "
            "and %.2f dB at edges\n",
            p->gain_dc,
            p->gain_peak,
            p->freq_peak,
            p->bandwidth,
            p->gain_bandwidth);
        printf("          | Coeffs: [%f %f %f %f %f]\n",
               x->coeff[c],
               x->coeff[c + 1],
               x->coeff[c + 2],
               x->coeff[c + 3],
               x->coeff[c + 4]);
        c += NBCOEFF;
        break;
      }
      case LPHP: {
        t_lphp *f = x->filters[i]->filter;
        if (f->type == LOWPASS) {
          printf("Filter %2d | Low-pass Filter | Params: %.2f Hz, %.2f%%, %d order\n",
                 i + 1,
                 f->freq,
                 f->ripple,
                 f->order);
        } else {
          printf("Filter %2d | High-pass Filter | Params: %.2f Hz, %.2f%%, %d order\n",
                 i + 1,
                 f->freq,
                 f->ripple,
                 f->order);
        }
        printf("          | i.e. cutoff frequency at %.2f Hz with ripples at %.2f%% and order %d\n",
               f->freq,
               f->ripple,
               f->order);
        for (int j = 0; j < f->order / 2; j++) {
          printf("          | Coeffs: [%f %f %f %f %f]\n",
                 x->coeff[c],
                 x->coeff[c + 1],
                 x->coeff[c + 2],
                 x->coeff[c + 3],
                 x->coeff[c + 4]);
          c += NBCOEFF;
        }
        break;
      }
    }
    i++;
  }
  printf("Number of filters: %d\n", i);
  printf("Number of biquads: %d\n", x->b_nbiquads);
  printf("Complexity per sample: %d multiplications, %d additions\n", c, c - x->b_nbiquads);
  printf("Complexity per second: %.0f multiplications, %.0f additions\n",
         c * x->b_Fs,
         (c - 1) * x->b_Fs);
}

int do_peqbank_perform_fast(t_peqbank *x) {
  int n = x->s_n;
  float a0, a1, a2, b1, b2;

  // msvc does not support C99 VLA, so stack allocate instead
  float *xn = alloca(x->b_channels);
  float *yn = alloca(x->b_channels);
  float *xm2 = alloca(x->b_channels);
  float *xm1 = alloca(x->b_channels);
  float *ym2 = alloca(x->b_channels);
  float *ym1 = alloca(x->b_channels);

  // First copy input vector to output vector, we'll filter the output in-place
  for (int c = 0; c < x->b_channels; c++) {
    if (x->s_vec_out[c] != x->s_vec_in[c]) {
      for (int i = 0; i < n; i++) x->s_vec_out[c][i] = x->s_vec_in[c][i];
    }
  }

  // Cascade of Biquads
  int k = 0, s = 0;
  for (int j = 0; j < x->b_nbiquads * NBCOEFF; j += NBCOEFF) {
    for (int c = 0; c < x->b_channels; c++) {
      xm2[c] = x->b_xm2[k * x->b_channels + c];
      xm1[c] = x->b_xm1[k * x->b_channels + c];
      ym2[c] = x->b_ym2[k * x->b_channels + c];
      ym1[c] = x->b_ym1[k * x->b_channels + c];
    }

    a0 = x->coeff[j];
    a1 = x->coeff[j + 1];
    a2 = x->coeff[j + 2];
    b1 = x->coeff[j + 3];
    b2 = x->coeff[j + 4];

    for (int i = 0; i < n; i++) {
      for (int c = 0; c < x->b_channels; c++) {
        xn[c] = x->s_vec_out[c][i];
      }
      for (int c = 0; c < x->b_channels; c++) {
        yn[c] = (a0 * xn[c]) + (a1 * xm1[c]) + (a2 * xm2[c]) - (b1 * ym1[c]) - (b2 * ym2[c]);
      }
      for (int c = 0; c < x->b_channels; c++) {
        x->s_vec_out[c][i] = yn[c];
      }
      for (int c = 0; c < x->b_channels; c++) {
        xm2[c] = xm1[c];
        xm1[c] = xn[c];
        ym2[c] = ym1[c];
        ym1[c] = yn[c];
      }
      s++;
    }
    for (int c = 0; c < x->b_channels; c++) {
      x->b_xm2[k * x->b_channels + c] = FLUSH_TO_ZERO(xm2[c]);
      x->b_xm1[k * x->b_channels + c] = FLUSH_TO_ZERO(xm1[c]);
      x->b_ym2[k * x->b_channels + c] = FLUSH_TO_ZERO(ym2[c]);
      x->b_ym1[k * x->b_channels + c] = FLUSH_TO_ZERO(ym1[c]);
    }
    k++;
  }  // cascade loop

  if (k == 0) return 0;
  return s / k;
}

int peqbank_perform_fast(t_peqbank *x) {
  int k = do_peqbank_perform_fast(x);

  // We still have to shuffle the coeff pointers around.
  if (x->coeff != x->oldcoeff) {
    if (x->freecoeff != 0) printf("Disaster (fast)! freecoeff should be zero now!\n");
    x->freecoeff = x->oldcoeff;
    x->oldcoeff = x->coeff;
  }
  return k;
}

int peqbank_perform_smooth(t_peqbank *x) {
  int n = x->s_n;

  float *mycoeff = x->coeff;
  if (mycoeff == x->oldcoeff) {
    // Coefficients haven't changed, so no need to interpolate
    return do_peqbank_perform_fast(x);
  } else {
    // Biquad with linear interpolation: smooth-biquad~
    float rate = 1.0f / n;

    // msvc does not support C99 VLA, so stack allocate instead
    float *i0 = alloca(x->b_channels);
    float *i1 = alloca(x->b_channels);
    float *i2 = alloca(x->b_channels);
    float *i3 = alloca(x->b_channels);
    float *y0 = alloca(x->b_channels);
    float *y1 = alloca(x->b_channels);

    float a0, a1, a2, b1, b2;
    float a0inc, a1inc, a2inc, b1inc, b2inc;

    // First copy input vector to output vector, we'll filter the output in-place
    for (int c = 0; c < x->b_channels; c++) {
      if (x->s_vec_out[c] != x->s_vec_in[c]) {
        for (int i = 0; i < n; i++) x->s_vec_out[c][i] = x->s_vec_in[c][i];
      }
    }

    //  Cascade of Biquads
    int k = 0, s = 0;
    for (int j = 0; j < x->b_nbiquads * NBCOEFF; j += NBCOEFF) {
      for (int c = 0; c < x->b_channels; c++) {
        i2[c] = x->b_xm2[k * x->b_channels + c];
        i3[c] = x->b_xm1[k * x->b_channels + c];
        y0[c] = x->b_ym2[k * x->b_channels + c];
        y1[c] = x->b_ym1[k * x->b_channels + c];
      }

      // Interpolated values
      a0 = x->oldcoeff[j];
      a1 = x->oldcoeff[j + 1];
      a2 = x->oldcoeff[j + 2];
      b1 = x->oldcoeff[j + 3];
      b2 = x->oldcoeff[j + 4];

      // Incrementation values
      a0inc = (mycoeff[j] - a0) * rate;
      a1inc = (mycoeff[j + 1] - a1) * rate;
      a2inc = (mycoeff[j + 2] - a2) * rate;
      b1inc = (mycoeff[j + 3] - b1) * rate;
      b2inc = (mycoeff[j + 4] - b2) * rate;

      for (int i = 0; i < n; i += 4) {
        for (int c = 0; c < x->b_channels; c++) {
          x->s_vec_out[c][i] = y0[c] = (a0 * (i0[c] = x->s_vec_out[c][i])) + (a1 * i3[c]) +
                                       (a2 * i2[c]) - (b1 * y1[c]) - (b2 * y0[c]);
        }
        a1 += a1inc;
        a2 += a2inc;
        a0 += a0inc;
        b1 += b1inc;
        b2 += b2inc;
        for (int c = 0; c < x->b_channels; c++) {
          x->s_vec_out[c][i + 1] = y1[c] = (a0 * (i1[c] = x->s_vec_out[c][i + 1])) + (a1 * i0[c]) +
                                           (a2 * i3[c]) - (b1 * y0[c]) - (b2 * y1[c]);
        }
        a1 += a1inc;
        a2 += a2inc;
        a0 += a0inc;
        b1 += b1inc;
        b2 += b2inc;
        for (int c = 0; c < x->b_channels; c++) {
          x->s_vec_out[c][i + 2] = y0[c] = (a0 * (i2[c] = x->s_vec_out[c][i + 2])) + (a1 * i1[c]) +
                                           (a2 * i0[c]) - (b1 * y1[c]) - (b2 * y0[c]);
        }
        a1 += a1inc;
        a2 += a2inc;
        a0 += a0inc;
        b1 += b1inc;
        b2 += b2inc;
        for (int c = 0; c < x->b_channels; c++) {
          x->s_vec_out[c][i + 3] = y1[c] = (a0 * (i3[c] = x->s_vec_out[c][i + 3])) + (a1 * i2[c]) +
                                           (a2 * i1[c]) - (b1 * y0[c]) - (b2 * y1[c]);
        }
        a1 += a1inc;
        a2 += a2inc;
        a0 += a0inc;
        b1 += b1inc;
        b2 += b2inc;

        s += 4;
      }  // Interpolation loop

      for (int c = 0; c < x->b_channels; c++) {
        x->b_xm2[k * x->b_channels + c] = FLUSH_TO_ZERO(i2[c]);
        x->b_xm1[k * x->b_channels + c] = FLUSH_TO_ZERO(i3[c]);
        x->b_ym2[k * x->b_channels + c] = FLUSH_TO_ZERO(y0[c]);
        x->b_ym1[k * x->b_channels + c] = FLUSH_TO_ZERO(y1[c]);
      }

      k++;
    }  // cascade loop

    if (x->freecoeff != 0) printf("Disaster (smooth)! freecoeff should be zero now!\n");
    x->freecoeff = x->oldcoeff;
    x->oldcoeff = mycoeff;

    if (k == 0) return 0;
    return s / k;
  }
}

static const int16_t limThresh = 31000;
#define limRange (INT16_MAX - limThresh)

// Ensures that the mixer never clips
int16_t sampleLimiter(int samp) {
  if (limThresh < samp) {
    printf("Limiter: %6d > ", samp);
    double s = (samp - limThresh) / (double)limRange;
    samp = (int16_t)((atan(s) / M_PI_2) * limRange + limThresh);
    printf("%d\n", samp);
  } else if (samp < -limThresh) {
    printf("Limiter: %6d > ", samp);
    double s = -(limThresh + samp) / (double)limRange;
    samp = (int16_t) - ((atan(s) / M_PI_2) * limRange + limThresh);
    printf("%d\n", samp);
  }
  return samp;
}

int peqbank_callback_int16(t_peqbank *x, int16_t *sig_input, int16_t *sig_output) {
  for (int i = 0; i < x->s_n; i++) {
    for (int j = 0; j < x->b_channels; j++) {
      x->s_vec_in[j][i] = (float)(sig_input[i * x->b_channels + j] / 32767.0f);
    }
  }

  int k = 0;
  if (x->b_mode == FAST) {
    k = peqbank_perform_fast(x);
  } else {
    k = peqbank_perform_smooth(x);
  }

  for (int i = 0; i < x->s_n; i++) {
    for (int j = 0; j < x->b_channels; j++) {
      sig_output[i * x->b_channels + j] = (int)(x->s_vec_out[j][i] * 32767.0f);
    }
  }
  return k;
}

int peqbank_callback_float(t_peqbank *x, float *sig_input, float *sig_output) {
  for (int i = 0; i < x->s_n; i++) {
    for (int j = 0; j < x->b_channels; j++) {
      x->s_vec_in[j][i] = sig_input[i * x->b_channels + j];
    }
  }

  if (sig_input == sig_output) {
    x->s_vec_out = x->s_vec_in;
  } else {
    x->s_vec_out = x->s_vec_bak;  // in case we were in-place filtering previously
  }

  int k = 0;
  if (x->b_mode == FAST) {
    k = peqbank_perform_fast(x);
  } else {
    k = peqbank_perform_smooth(x);
  }

  for (int i = 0; i < x->s_n; i++) {
    for (int j = 0; j < x->b_channels; j++) {
      sig_output[i * x->b_channels + j] = x->s_vec_out[j][i];
    }
  }
  return k;
}

void compute_shelf(t_peqbank *x, t_shelf *s, int index) {
  // Biquad coefficient estimation
  float G1 = peqbank_pow10((s->gain_low - s->gain_middle) * 0.05f);
  float G2 = peqbank_pow10((s->gain_middle - s->gain_high) * 0.05f);
  float Gh = peqbank_pow10(s->gain_high * 0.05f);

  // Low shelf
  float X = tanf(s->freq_low * PI / x->b_Fs) / sqrtf(G1);
  float L1 = (X - 1.0f) / (X + 1.0f);
  float L2 = (G1 * X - 1.0f) / (G1 * X + 1.0f);
  float L3 = (G1 * X + 1.0f) / (X + 1.0f);

  // High shelf
  float Y = tanf(s->freq_high * PI / x->b_Fs) / sqrtf(G2);
  float H1 = (Y - 1.0f) / (Y + 1.0f);
  float H2 = (G2 * Y - 1.0f) / (G2 * Y + 1.0f);
  float H3 = (G2 * Y + 1.0f) / (Y + 1.0f);

  float C0 = L3 * H3 * Gh;

  // New values
  x->newcoeff[index] = C0;
  x->newcoeff[index + 1] = C0 * (L2 + H2);
  x->newcoeff[index + 2] = C0 * L2 * H2;
  x->newcoeff[index + 3] = L1 + H1;
  x->newcoeff[index + 4] = L1 * H1;
}

void compute_peq(t_peqbank *x, t_peq *p, int index) {
  // Biquad coefficient estimation
  float G0 = peqbank_pow10(p->gain_dc * 0.05f);
  float G = peqbank_pow10(p->gain_peak * 0.05f);
  float GB = peqbank_pow10(p->gain_bandwidth * 0.05f);

  float w0 = TWOPI * p->freq_peak / x->b_Fs;
  float G02 = G0 * G0;
  float GB2 = GB * GB;
  float G2 = G * G;
  float w02 = w0 * w0;

  float val1 = (float)(1.0f / fabs(G2 - GB2));
  float val2 = (float)(fabs(G2 - G02));
  float val3 = (float)(fabs(GB2 - G02));
  float val4 = (w02 - PI2) * (w02 - PI2);

  float mul1 = LOG_22 * p->bandwidth;
  float Dw = 2.0f * w0 * sinhf(mul1);
  float mul2 = val3 * PI2 * Dw * Dw;
  float num = G02 * val4 + G2 * mul2 * val1;
  float den = val4 + mul2 * val1;

  float G1 = sqrtf(num / den);
  float G12 = G1 * G1;

  float mul3 = G0 * G1;
  float val5 = (float)(fabs(G2 - mul3));
  float val6 = (float)(fabs(G2 - G12));
  float val7 = (float)(fabs(GB2 - mul3));
  float val8 = (float)(fabs(GB2 - G12));
  float val9 = sqrtf((val3 * val6) / (val8 * val2));

  float tan0 = tanf(w0 * 0.5f);
  float w1 = w0 * peqbank_pow2(p->bandwidth * -0.5f);
  float tan1 = tanf(w1 * 0.5f);
  float tan2 = val9 * tan0 * tan0 / tan1;

  float W2 = sqrtf(val6 / val2) * tan0 * tan0;
  float DW = tan2 - tan1;

  float C = val8 * DW * DW - 2.0f * W2 * (val7 - sqrtf(val3 * val8));
  float D = 2.0f * W2 * (val5 - sqrtf(val2 * val6));
  float A = sqrtf((C + D) * val1);
  float B = sqrtf((G2 * C + GB2 * D) * val1);

  float val10 = 1.0f / (1.0f + W2 + A);

  // New values
  x->newcoeff[index] = (G1 + G0 * W2 + B) * val10;
  x->newcoeff[index + 1] = -2.0f * (G1 - G0 * W2) * val10;
  x->newcoeff[index + 2] = (G1 - B + G0 * W2) * val10;
  x->newcoeff[index + 3] = -2.0f * (1.0f - W2) * val10;
  x->newcoeff[index + 4] = (1.0f + W2 - A) * val10;
}

void compute_lphp(t_peqbank *x, t_lphp *f, int index) {
  float FC, PR;
  float A0, A1, A2, B1, B2;
  float RP, IP, ES, VX, KX, T, W, M, D, X0, X1, X2, Y1, Y2;
  float K = 0;
  float GAIN;
  unsigned int LH, NP, P;

  FC = f->freq;
  LH = f->type;
  PR = f->ripple;
  NP = f->order;
  FC = FC / x->b_Fs;

  for (P = 0; P < NP / 2; P++) {
    // calculate pole location on unit circle
    RP = (float)(-cos(PI / (NP * 2) + P * PI / NP));
    IP = (float)(sin(PI / (NP * 2) + P * PI / NP));

    // warp from a circle to an ellipse
    if (PR != 0) {
      ES = (float)(sqrt(((100.0f / (100.0f - PR)) * (100.0f / (100.0f - PR))) - 1.0f));
      VX = (float)((1.0f / NP) * log((1.0f / ES) + sqrt((1.0f / (ES * ES)) + 1.0f)));
      KX = (float)((1.0f / NP) * log((1.0f / ES) + sqrt((1.0f / (ES * ES)) - 1.0f)));
      KX = (float)((exp(KX) + exp(-KX)) / 2.0f);
      RP = (float)(RP * ((exp(VX) - exp(-VX)) / 2.0f) / KX);
      IP = (float)(IP * ((exp(VX) + exp(-VX)) / 2.0f) / KX);
    }

    // s-domain to z-domain conversion
    T = (float)(2 * tan(0.5f));
    W = 2 * PI * FC;
    M = (RP * RP) + (IP * IP);
    D = 4 - 4 * RP * T + M * (T * T);
    X0 = (T * T) / D;
    X1 = 2 * (T * T) / D;
    X2 = (T * T) / D;
    Y1 = (8 - 2 * M * (T * T)) / D;
    Y2 = (-4 - 4 * RP * T - M * (T * T)) / D;

    // lopass to lopass or lopass to hipass transform
    if (LH == HIGHPASS) K = (float)(-cos(W / 2.0f + 0.5f) / cos(W / 2.0f - 0.5f));
    if (LH == LOWPASS) K = (float)(sin(0.5f - W / 2.0f) / sin(0.5f + W / 2.0f));

    D = 1.0f + Y1 * K - Y2 * (K * K);
    A0 = (X0 - X1 * K + X2 * (K * K)) / D;
    A1 = (-2 * X0 * K + X1 + X1 * (K * K) - 2 * X2 * K) / D;
    A2 = (X0 * (K * K) - X1 * K + X2) / D;
    B1 = (2 * K + Y1 + Y1 * (K * K) - 2 * Y2 * K) / D;
    B2 = (-(K * K) - Y1 * K + Y2) / D;
    GAIN = (1.0f - (B1 + B2)) / (A0 + A1 + A2);

    if (LH == HIGHPASS) {
      A1 = -A1;
      B1 = -B1;
    }

    x->newcoeff[(P * 5) + index] = A0 * GAIN;
    x->newcoeff[(P * 5) + index + 1] = A1 * GAIN;
    x->newcoeff[(P * 5) + index + 2] = A2 * GAIN;
    x->newcoeff[(P * 5) + index + 3] = -B1;
    x->newcoeff[(P * 5) + index + 4] = -B2;
  }
}

void swap_in_new_coeffs(t_peqbank *x) {
  float *prevcoeffs, *prevnew, *prevfree;

  if (x->coeff == x->oldcoeff) {
    // Normal case: these are the first new coeffients since the last perform routine
    if (x->freecoeff == 0) printf("Problem: freecoeff shouldn't be 0\n");

    prevcoeffs = x->coeff;
    prevnew = x->newcoeff;
    prevfree = x->freecoeff;

    x->freecoeff = 0;
    x->coeff = x->newcoeff;  // Now if we're interrupted the new values will be used.
    x->newcoeff = prevfree;

  } else {
    prevcoeffs = x->coeff;
    prevnew = x->newcoeff;

    x->coeff = x->newcoeff;
    x->newcoeff = prevcoeffs;
  }
}

void peqbank_compute(t_peqbank *x) {
  // Do the actual computation of coefficients, into x->newcoeff
  int i = 0;
  int c = 0;
  while (x->filters[i]->type != NONE) {
    switch (x->filters[i]->type) {
      case SHELF: {
        t_shelf *s = x->filters[i]->filter;
        compute_shelf(x, s, c);
        c += NBCOEFF;
        break;
      }
      case PEQ: {
        t_peq *p = x->filters[i]->filter;
        compute_peq(x, p, c);
        c += NBCOEFF;
        break;
      }
      case LPHP: {
        t_lphp *f = x->filters[i]->filter;
        compute_lphp(x, f, c);
        c += (f->order / 2) * NBCOEFF;
        break;
      }
    }
    i++;
  }
  x->b_nbiquads = c / NBCOEFF;
  swap_in_new_coeffs(x);
}

void peqbank_reset(t_peqbank *x) {
  long oldmax = x->b_max;
  x->b_max = MAXELEM;

  if (oldmax != x->b_max) {
    peqbank_freemem(x);
    peqbank_allocmem(x);
  }

  if (x->coeff) {
    memset(x->coeff, 0, x->b_max * NBCOEFF * sizeof(float));
  }
  if (x->oldcoeff) {
    memset(x->oldcoeff, 0, x->b_max * NBCOEFF * sizeof(float));
  }
  if (x->newcoeff) {
    memset(x->newcoeff, 0, x->b_max * NBCOEFF * sizeof(float));
  }
  if (x->freecoeff) {
    memset(x->freecoeff, 0, x->b_max * NBCOEFF * sizeof(float));
  }
  if (x->b_ym1) {
    memset(x->b_ym1, 0, x->b_max * x->b_channels * sizeof(float));
  }
  if (x->b_ym2) {
    memset(x->b_ym2, 0, x->b_max * x->b_channels * sizeof(float));
  }
  if (x->b_xm1) {
    memset(x->b_xm1, 0, x->b_max * x->b_channels * sizeof(float));
  }
  if (x->b_xm2) {
    memset(x->b_xm2, 0, x->b_max * x->b_channels * sizeof(float));
  }

  peqbank_init(x);
  peqbank_compute(x);
}

t_filter *new_shelf(
    float gain_low, float gain_middle, float gain_high, float freq_low, float freq_high) {
  t_shelf *shelf = (t_shelf *)malloc(sizeof(t_shelf));
  shelf->gain_low = gain_low;
  shelf->gain_middle = gain_middle;
  shelf->gain_high = gain_high;
  shelf->freq_low = freq_low;
  shelf->freq_high = freq_high;
  if (freq_low == 0) shelf->freq_low = (float)SMALL;
  if (freq_high == 0) shelf->freq_high = (float)SMALL;

  t_filter *filter = (t_filter *)malloc(sizeof(t_filter));
  filter->type = SHELF;
  filter->filter = shelf;
  return filter;
}

t_filter *new_peq(
    float freq_peak, float bandwidth, float gain_dc, float gain_peak, float gain_bandwidth) {
  t_peq *peq = (t_peq *)malloc(sizeof(t_peq));
  peq->freq_peak = freq_peak;
  peq->bandwidth = bandwidth;
  float G0 = gain_dc;
  float G = gain_peak;
  float GB = gain_bandwidth;

  if (G0 == G) {
    GB = G0;
    G = (float)(G0 + SMALL);
    G0 = (float)(G0 - SMALL);
  } else if (!((G0 < GB) && (GB < G)) && !((G0 > GB) && (GB > G))) {
    GB = (G0 + G) * 0.5f;
  }
  peq->gain_dc = G0;
  peq->gain_peak = G;
  peq->gain_bandwidth = GB;

  t_filter *filter = (t_filter *)malloc(sizeof(t_filter));
  filter->type = PEQ;
  filter->filter = peq;
  return filter;
}

t_filter *new_lphp(float freq, float ripple, int order) {
  if ((order < MINORDER) || (order > MAXORDER) || (order % 2) || (ripple < 0) || (ripple > 29)) {
    printf("Problem in seting up the filter parameters. Cancelling.\n");
    return NULL;
  }
  t_lphp *lphp = (t_lphp *)malloc(sizeof(t_lphp));
  lphp->freq = freq;
  lphp->ripple = ripple;
  lphp->order = order;

  t_filter *filter = (t_filter *)malloc(sizeof(t_filter));
  filter->type = LPHP;
  filter->filter = lphp;
  return filter;
}

t_filter *new_lowpass(float freq, float ripple, int order) {
  t_filter *lphp = new_lphp(freq, ripple, order);
  ((t_lphp *)lphp->filter)->type = LOWPASS;
  return lphp;
}

t_filter *new_highpass(float freq, float ripple, int order) {
  t_filter *lphp = new_lphp(freq, ripple, order);
  ((t_lphp *)lphp->filter)->type = HIGHPASS;
  return lphp;
}

t_filter **new_filters(int num_filters) {
  int actual_num_filters = min(num_filters, MAXELEM);
  t_filter **filters = (t_filter **)malloc((actual_num_filters + 1) * sizeof(t_filter *));
  t_filter *dummy = (t_filter *)malloc(sizeof(t_filter));
  dummy->type = NONE;
  dummy->filter = NULL;
  filters[actual_num_filters] = dummy;
  return filters;
}

void free_filters(t_filter **filters) {
  int i = 0;
  while (filters[i]->type != NONE) {
    free(filters[i]->filter);
    free(filters[i]);
    i++;
  }
  free(filters[i]->filter);
  free(filters[i]);
}

void peqbank_setup(t_peqbank *x, t_filter **filters) {
  peqbank_init(x);
  x->filters = filters;
  peqbank_compute(x);
}
