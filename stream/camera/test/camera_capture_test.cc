/*
 * Copyright (C) 2017 Hertz Wang 1989wanghang@163.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses
 *
 * Any non-GPL usage of this software or parts of this software is strictly
 * forbidden.
 *
 */

#ifdef NDEBUG
#undef NDEBUG
#endif
#ifndef DEBUG
#define DEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>

#include "buffer.h"
#include "stream.h"

static char optstr[] = "?i:o:d:w:h:f:";

int main(int argc, char **argv) {
  int c;
  std::string input_path;
  std::string output_path;
  int w = 0, h = 0;
  std::string input_format;
  std::string output_format;
  bool display = false;

  opterr = 1;
  while ((c = getopt(argc, argv, optstr)) != -1) {
    switch (c) {
    case 'i':
      input_path = optarg;
      printf("input path: %s\n", input_path.c_str());
      break;
    case 'o':
      output_path = optarg;
      printf("output file path: %s\n", output_path.c_str());
      break;
    case 'd':
      display = !!atoi(optarg);
      break;
    case 'w':
      w = atoi(optarg);
      break;
    case 'h':
      h = atoi(optarg);
      break;
    case 'f': {
      char *cut = strchr(optarg, ',');
      if (!cut) {
        fprintf(stderr, "input/output format must be cut by \',\'\n");
        exit(EXIT_FAILURE);
      }
      cut[0] = 0;
      input_format = optarg;
      output_format = cut + 1;
      printf("input format: %s\n", input_format.c_str());
      printf("output format: %s\n", output_format.c_str());
    } break;
    case '?':
    default:
      printf("usage example: \n");
      printf("camera_cap_test -i /dev/video0 -o output.yuv -w 1920 -h 1080 -f "
             "image:yuyv422\n");
      exit(0);
    }
  }
  if (input_path.empty())
    exit(EXIT_FAILURE);
  if (output_path.empty() && !display)
    exit(EXIT_FAILURE);
  if (!w || !h)
    exit(EXIT_FAILURE);
  if (input_format.empty())
    exit(EXIT_FAILURE);
  printf("width, height: %d, %d\n", w, h);

  easymedia::REFLECTOR(Stream)::DumpFactories();
  // std::string flow_name;
  std::string param;
  PARAM_STRING_APPEND(param, KEY_DEVICE, input_path);
  // PARAM_STRING_APPEND(param, KEY_SUB_DEVICE, sub_input_path);

  return 0;
}
