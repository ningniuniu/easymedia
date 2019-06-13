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

#ifndef RKMEDIA_UTILS_H_
#define RKMEDIA_UTILS_H_

#define _UNUSED __attribute__((unused))
#define UNUSED(x) (void)x

#define _LOCAL __attribute__((visibility("hidden")))
#define _API __attribute__((visibility("default")))

_API void LOG(const char *format, ...);

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

#define PAGE_SIZE (sysconf(_SC_PAGESIZE))

#define DUMP_FOURCC(f)                                                         \
  f & 0xFF, (f >> 8) & 0xFF, (f >> 16) & 0xFF, (f >> 24) & 0xFF

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

namespace easymedia {

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

_API bool string_start_withs(std::string const &fullString,
                             std::string const &starting);
_API bool string_end_withs(std::string const &fullString,
                           std::string const &ending);

// return milliseconds
_API inline int64_t gettimeofday() {
  std::chrono::milliseconds ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch());
  return ms.count();
}

_API inline void msleep(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

typedef int (*Ioctl_f)(int fd, unsigned long int request, ...);
inline int xioctl(Ioctl_f f, int fd, int request, void *argp) {
  int r;

  do
    r = f(fd, request, argp);
  while (-1 == r && EINTR == errno);

  return r;
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
  AutoPrintLine(const char *f _UNUSED) {}
#endif
};

} // namespace easymedia

#endif // #ifndef RKMEDIA_UTILS_H_
