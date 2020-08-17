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

const char *base_path = NULL;

int test1();  // 10 sec white noise, mono, lowpass and highpass pass filters at 4000 Hz
int test2();  // 10 sec, 4 pure tones, stereo, peq filters
int test3();  // 10 sec white noise, stereo, shelf filters and sharp peq in the middle
int test4();  // music filtered by various kinds of filters

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr,
            "Number of arguments not valid. This program expects one argument, the base path to "
            "the test files.\n");
    exit(1);
  }
  base_path = argv[1];
  if (test1())
    printf("test1 succeeded!\n\n");
  else
    printf("test1 failed!\n\n");
  if (test2())
    printf("test2 succeeded!\n\n");
  else
    printf("test2 failed!\n\n");
  if (test3())
    printf("test3 succeeded!\n\n");
  else
    printf("test3 failed!\n\n");
  if (test4())
    printf("test4 succeeded!\n\n");
  else
    printf("test4 failed!\n\n");

  return 0;
}

int test1() {
  printf("Test1: 10 sec white noise, mono, lowpass and highpass pass filters at 4000 Hz\n");
  int sampling_rate = 44100;
  int num_channels = 1;     // mono
  int buffer_size = 4096;   // callback buffer size
  int num_frames = 441000;  // 10 seconds

  t_peqbank *x = peqbank_new(sampling_rate, num_channels, buffer_size);

  if (!x) {
    return -1;
  }

  int16_t *signal_in = (int16_t *)malloc(num_frames * num_channels * sizeof(int16_t));
  int16_t *signal_out = (int16_t *)malloc(num_frames * num_channels * sizeof(int16_t));

  srand((unsigned int)time(NULL));
  float gain = 0.5;
  for (int i = 0; i < num_frames * num_channels; i++) {
    signal_in[i] = (int16_t)(gain * ((rand() % 65534) - 32767.0f));
    signal_out[i] = 0;
  }

  printf("Input: ");
  for (int i = 0; i < 20; i++) {
    printf("%d ", signal_in[i * num_channels]);
  }
  printf("...\n");

  printf("Setting up filter\n");
  t_filter **filters = new_filters(1);  // init number of desired filters

  filters[0] = new_lowpass(4000, 1, 6);  // attach lowpass filter
  peqbank_setup(x, filters);             // setup filters
  peqbank_print_info(x);                 // output info

  printf("Processing signal\n");
  int frame = 0;
  for (int i = 0; i < num_frames; i += buffer_size) {
    if (num_frames - i < buffer_size) x->s_n = num_frames - i;
    frame += peqbank_callback_int16(
        x, &signal_in[frame * num_channels], &signal_out[frame * num_channels]);
  }
  x->s_n = buffer_size;  // reset buffer size...

  printf("Output: ");
  for (int i = 0; i < 20; i++) {
    printf("%d ", signal_out[i * num_channels]);
  }
  printf("...\n");

  char path[256];

  static FILE *fin;
  snprintf(path, sizeof(path), "%s%s", base_path, "test1_in.pcm");
  if (!fin) fin = fopen(path, "wb");
  fwrite(signal_in, num_frames * num_channels, sizeof(int16_t), fin);
  fclose(fin);

  static FILE *flp;
  snprintf(path, sizeof(path), "%s%s", base_path, "test1_out_lp.pcm");
  if (!flp) flp = fopen(path, "wb");
  fwrite(signal_out, num_frames * num_channels, sizeof(int16_t), flp);
  fclose(flp);

  free(filters[0]);
  filters[0] = new_highpass(4000, 1, 6);  // attach highpass filter
  peqbank_setup(x, filters);              // setup filters
  peqbank_print_info(x);                  // output info

  printf("Processing signal\n");
  frame = 0;
  for (int i = 0; i < num_frames; i += buffer_size) {
    if (num_frames - i < buffer_size) x->s_n = num_frames - i;
    frame += peqbank_callback_int16(
        x, &signal_in[frame * num_channels], &signal_out[frame * num_channels]);
  }
  x->s_n = buffer_size;  // reset buffer size...

  printf("Output: ");
  for (int i = 0; i < 20; i++) {
    printf("%d ", signal_out[i * num_channels]);
  }
  printf("...\n");

  static FILE *fhp;
  snprintf(path, sizeof(path), "%s%s", base_path, "test1_out_hp.pcm");
  if (!fhp) fhp = fopen(path, "wb");
  fwrite(signal_out, num_frames * num_channels, sizeof(int16_t), fhp);
  fclose(fhp);

  free(signal_in);
  free(signal_out);
  free_filters(filters);
  free(x);

  return 1;
}

int test2() {
  printf("Test2: 10 sec, 4 pure tones, stereo, peq filters\n");
  int sampling_rate = 44100;
  int num_channels = 2;     // stereo
  int buffer_size = 4096;   // callback buffer size
  int num_frames = 441000;  // 10 seconds

  t_peqbank *x = peqbank_new(sampling_rate, num_channels, buffer_size);

  if (!x) {
    return -1;
  }

  int16_t *signal_in = (int16_t *)malloc(num_frames * num_channels * sizeof(int16_t));
  int16_t *signal_out = (int16_t *)malloc(num_frames * num_channels * sizeof(int16_t));

  float gain = 0.5;
  for (int j = 0; j < num_channels; j++) {
    float f1 = 100;
    float v1 = 0.5;
    float f2 = 700;
    float v2 = 0.9f;
    float f3 = 4000;
    float v3 = 0.33f;
    float f4 = 13000;
    float v4 = 0.25;

    double t = 1.0 / sampling_rate;
    for (int i = 0; i < num_frames; i++) {
      double sig1 = v1 * sin(TWOPI * f1 * i * t);
      double sig2 = v2 * sin(TWOPI * f2 * i * t);
      double sig3 = v3 * sin(TWOPI * f3 * i * t);
      double sig4 = v4 * sin(TWOPI * f4 * i * t);
      signal_in[i * num_channels + j] = (int16_t)(gain * (sig1 + sig2 + sig3 + sig4) * 32767.0f);
      signal_out[i * num_channels + j] = 0;
    }
  }

  printf("Input: ");
  for (int i = 0; i < 20; i++) {
    printf("%d ", signal_in[i * num_channels]);
  }
  printf("...\n");

  printf("Setting up filter\n");
  t_filter **filters = new_filters(2);           // init number of desired filters
  filters[0] = new_peq(100, 0.1f, -48, 48, -12);  // extracting 2 pure tones out of 4
  filters[1] = new_peq(4000, 0.1f, -48, 48, -12);
  peqbank_setup(x, filters);  // setup filters
  peqbank_print_info(x);      // output info

  printf("Processing signal\n");
  int frame = 0;
  while (frame < num_frames) {
    if (num_frames - frame < buffer_size) x->s_n = num_frames - frame;
    frame += peqbank_callback_int16(
        x, &signal_in[frame * num_channels], &signal_out[frame * num_channels]);
  }
  x->s_n = buffer_size;  // reset buffer size...

  printf("Output: ");
  for (int i = 0; i < 20; i++) {
    printf("%d ", signal_out[i * num_channels]);
  }
  printf("...\n");

  char path[256];

  static FILE *fin;
  snprintf(path, sizeof(path), "%s%s", base_path, "test2_in.pcm");
  if (!fin) fin = fopen(path, "wb");
  fwrite(signal_in, num_frames * num_channels, sizeof(int16_t), fin);
  fclose(fin);

  static FILE *flp;
  snprintf(path, sizeof(path), "%s%s", base_path, "test2_out.pcm");
  if (!flp) flp = fopen(path, "wb");
  fwrite(signal_out, num_frames * num_channels, sizeof(int16_t), flp);
  fclose(flp);

  free(signal_in);
  free(signal_out);
  free_filters(filters);
  free(x);

  return 1;
}

int test3() {
  printf("Test3: 10 sec white noise, stereo, double shelf filters and peq in the middle\n");
  int sampling_rate = 44100;
  int num_channels = 2;     // stereo
  int buffer_size = 4096;   // callback buffer size
  int num_frames = 441000;  // 10 seconds

  t_peqbank *x = peqbank_new(sampling_rate, num_channels, buffer_size);

  if (!x) {
    return -1;
  }

  int16_t *signal_in = (int16_t *)malloc(num_frames * num_channels * sizeof(int16_t));
  int16_t *signal_out = (int16_t *)malloc(num_frames * num_channels * sizeof(int16_t));

  srand((unsigned int)time(NULL));
  float gain = 0.5;
  for (int i = 0; i < num_frames * num_channels; i++) {
    signal_in[i] = (int16_t)(gain * ((rand() % 65534) - 32767.0f));
    signal_out[i] = 0;
  }

  printf("Input: ");
  for (int i = 0; i < 20; i++) {
    printf("%d ", signal_in[i * num_channels]);
  }
  printf("...\n");

  printf("Setting up filter\n");
  t_filter **filters = new_filters(4);           // init number of desired filters
  filters[0] = new_shelf(0, -6, 0, 1000, 6000);  // make filters steeper by stacking them
  filters[1] = new_shelf(0, -6, 0, 1000, 6000);
  filters[2] = new_shelf(0, -6, 0, 1000, 6000);
  filters[3] = new_peq(3000, 0.1f, 0, 48, 3);  // sounds like a sinusoid at 3KHz
  peqbank_setup(x, filters);                  // setup filters
  peqbank_print_info(x);                      // output info

  printf("Processing signal\n");
  int frame = 0;
  for (int i = 0; i < num_frames - num_channels * buffer_size; i += buffer_size) {
    if (num_frames - i < buffer_size) x->s_n = num_frames - i;
    frame += peqbank_callback_int16(
        x, &signal_in[frame * num_channels], &signal_out[frame * num_channels]);
  }
  x->s_n = buffer_size;  // reset buffer size...

  printf("Output: ");
  for (int i = 0; i < 20; i++) {
    printf("%d ", signal_out[i * num_channels]);
  }
  printf("...\n");

  char path[256];

  static FILE *fin;
  snprintf(path, sizeof(path), "%s%s", base_path, "test3_in.pcm");
  if (!fin) fin = fopen(path, "wb");
  fwrite(signal_in, num_frames * num_channels, sizeof(int16_t), fin);
  fclose(fin);

  static FILE *fou;
  snprintf(path, sizeof(path), "%s%s", base_path, "test3_out.pcm");
  if (!fou) fou = fopen(path, "wb");
  fwrite(signal_out, num_frames * num_channels, sizeof(int16_t), fou);
  fclose(fou);

  free(signal_in);
  free(signal_out);
  free_filters(filters);
  free(x);

  return 1;
}

int test4() {
  printf("Test4: music filtered by various kinds of filters\n");
  int sampling_rate = 44100;
  int num_channels = 2;    // stereo
  int buffer_size = 4096;  // callback buffer size
  int num_frames = 943828;

  t_peqbank *x = peqbank_new(sampling_rate, num_channels, buffer_size);

  if (!x) {
    return -1;
  }

  int16_t *signal_in = (int16_t *)malloc(num_frames * num_channels * sizeof(int16_t));
  int16_t *signal_out = (int16_t *)malloc(num_frames * num_channels * sizeof(int16_t));

  char path[256];

  static FILE *fin;
  snprintf(path, sizeof(path), "%s%s", base_path, "music_test.pcm");
  if (!fin) fin = fopen(path, "rb");
  fread(signal_in, sizeof(int16_t), num_frames * num_channels, fin);
  fclose(fin);

  printf("Input: ");
  for (int i = 0; i < 20; i++) {
    printf("%d ", signal_in[i * num_channels]);
  }
  printf("...\n");

  printf("Setting up filter\n");
  t_filter **filters = new_filters(4);           // init number of desired filters
  filters[0] = new_shelf(0, -12, 0, 100, 5000);  // -12 db between 100 and 5000 Hz
  filters[1] = new_highpass(500, 0.5, 8);        // cut below 500 Hz
  filters[2] = new_peq(3000, 0.5, -3, 12, 3);    // bump at 3000 Hz
  filters[3] = new_peq(4000, 0.25, 0, -6, -3);   // slight cut at 4000 Hz
  peqbank_setup(x, filters);                     // setup filters
  peqbank_print_info(x);                         // output info

  printf("Processing signal\n");
  int frame = 0;
  for (int i = 0; i < num_frames - buffer_size * num_channels; i += buffer_size) {
    if (num_frames - i < buffer_size) x->s_n = num_frames - i;
    frame += peqbank_callback_int16(
        x, &signal_in[frame * num_channels], &signal_out[frame * num_channels]);
  }
  x->s_n = buffer_size;  // reset buffer size...

  printf("Output: ");
  for (int i = 0; i < 20; i++) {
    printf("%d ", signal_out[i * num_channels]);
  }
  printf("...\n");

  static FILE *fou;
  snprintf(path, sizeof(path), "%s%s", base_path, "test4_out.pcm");
  if (!fou) fou = fopen(path, "wb");
  fwrite(signal_out, num_frames * num_channels, sizeof(int16_t), fou);
  fclose(fou);

  free(signal_in);
  free(signal_out);
  free_filters(filters);
  free(x);

  return 1;
}
