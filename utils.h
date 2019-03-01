/*
 * Copyright (C) 2018 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_UTILS_H_
#define RKMEDIA_UTILS_H_

#define _UNUSED __attribute__((unused))
#define UNUSED(x) (void)x

void LOG(const char *format, ...);

#define LOG_NO_MEMORY()                                                        \
  fprintf(stderr, "No memory %s: %d\n", __FUNCTION__, __LINE__)

#define LOG_FILE_FUNC_LINE()                                                   \
  fprintf(stderr, "%s : %s: %d\n", __FILE__, __FUNCTION__, __LINE__)

#define UPALIGNTO(value, align) ((value + align - 1) & (~(align - 1)))

#define UPALIGNTO16(value) UPALIGNTO(value, 16)

#define ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

#define MATH_LOG2(x) (31 - __builtin_clz((x) | 1))

template <typename T, typename TBase> class IsDerived {
public:
  static int t(TBase *base) { return 1; }
  static char t(void *t2) { return 0; }
  static const bool Result = (sizeof(int) == sizeof(t((T *)nullptr)));
};

#include <list>
#include <map>
#include <string>

namespace rkmedia {

// delim: '=', '\n'
bool parse_media_param_map(const char *param,
                           std::map<std::string, std::string> &map);
bool parse_media_param_list(const char *param, std::list<std::string> &list,
                            const char delim = '\n');
int parse_media_param_match(
    const char *param, std::map<std::string, std::string> &map,
    std::list<std::pair<const std::string, std::string &>> &list);
bool has_intersection(const char *str, const char *expect,
                      std::list<std::string> *expect_list);

bool string_end_withs(std::string const &fullString, std::string const &ending);
}

#endif // #ifndef RKMEDIA_UTILS_H_
