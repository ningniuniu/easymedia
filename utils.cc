/*
 * Copyright (C) 2018 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "utils.h"

#include <stdarg.h>
#include <stdio.h>

#include <algorithm>
#include <sstream>

void LOG(const char *format, ...) {
  char line[1024];
  va_list vl;
  va_start(vl, format);
  vsnprintf(line, sizeof(line), format, vl);
  va_end(vl);
}

namespace rkmedia {

bool parse_media_param_map(const char *param,
                           std::map<std::string, std::string> &map) {
  if (!param)
    return false;

  std::string token;
  std::istringstream tokenStream(param);
  while (std::getline(tokenStream, token)) {
    std::string key, value;
    std::istringstream subTokenStream(token);
    if (std::getline(subTokenStream, key, '='))
      std::getline(subTokenStream, value);
    map[key] = value;
  }

  return true;
}

bool parse_media_param_list(const char *param, std::list<std::string> &list,
                            const char delim) {
  if (!param)
    return false;

  std::string token;
  std::istringstream tokenStream(param);
  while (std::getline(tokenStream, token, delim))
    list.push_back(token);

  return true;
}

bool has_intersection(const char *str, const char *expect,
                      std::list<std::string> *expect_list) {
  std::list<std::string> request;
  if (!parse_media_param_list(str, request, ','))
    return false;
  if (expect_list->empty() && !parse_media_param_list(expect, *expect_list))
    return false;
  for (auto &req : request) {
    if (std::find(expect_list->begin(), expect_list->end(), req) !=
        expect_list->end())
      return true;
  }
  return false;
}
};
