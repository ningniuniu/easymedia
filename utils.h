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

#define LOG_TODO()                                                             \
  fprintf(stderr, "TODO, %s : %s: %d\n", __FILE__, __FUNCTION__, __LINE__)

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
#include <thread>

namespace rkmedia {

#define CHECK_EMPTY_SETERRNO_RETURN(v_type, v, map, k, seterrno, ret)          \
  v_type v = map[k];                                                           \
  if (v.empty()) {                                                             \
    LOG("miss %s\n", k);                                                       \
    seterrno;                                                                  \
    return ret;                                                                \
  }

#define CHECK_EMPTY(v, map, k) CHECK_EMPTY_SETERRNO_RETURN(, v, map, k, , false)

#define CHECK_EMPTY_WITH_DECLARE(v_type, v, map, k)                            \
  CHECK_EMPTY_SETERRNO_RETURN(v_type, v, map, k, , false)

#define CHECK_EMPTY_SETERRNO(v, map, k, err)                                   \
  CHECK_EMPTY_SETERRNO_RETURN(, v, map, k, errno = err, )

#define PARAM_STRING_APPEND(s, s1, s2) s.append(s1 "=").append(s2).append("\n")

#define PARAM_STRING_APPEND_TO(s, s1, s2)                                      \
  s.append(s1 "=").append(std::to_string(s2)).append("\n")

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

bool string_start_withs(std::string const &fullString,
                        std::string const &starting);
bool string_end_withs(std::string const &fullString, std::string const &ending);

// return milliseconds
inline int64_t gettimeofday() {
  std::chrono::milliseconds ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch());
  return ms.count();
}

inline void msleep(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

class AutoDuration {
public:
  AutoDuration() { Reset(); }
  int64_t Get() { return gettimeofday() - start; }
  void Reset() { start = gettimeofday(); }
  int64_t GetAndReset() {
    int64_t now = gettimeofday();
    int64_t pretime = start;
    start = now;
    return now - pretime;
  }

private:
  int64_t start;
};

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

class AutoPrintLine {
#ifdef DEBUG
public:
  AutoPrintLine(const char *f) : func(f) { LOG("Enter %s\n", f); }
  ~AutoPrintLine() { LOG("Exit %s\n", func); }

private:
  const char *func;
#else
public:
  AutoPrintLine(const char *f _UNUSED) = default;
#endif
};

} // namespace rkmedia

#endif // #ifndef RKMEDIA_UTILS_H_
