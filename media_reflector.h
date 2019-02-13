/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_MEDIA_REFLECTOR_H_
#define RKMEDIA_MEDIA_REFLECTOR_H_

#include "reflector.h"

#include <algorithm>

#define DEFINE_MEDIA_CHILD_FACTORY(REAL_PRODUCT, IDENTIFIER,                   \
                                   FINAL_EXPOSE_PRODUCT, PRODUCT)              \
  DEFINE_CHILD_FACTORY(                                                        \
      REAL_PRODUCT, IDENTIFIER, FINAL_EXPOSE_PRODUCT, PRODUCT,                 \
                                                                               \
      public                                                                   \
      :                                                                        \
                                                                               \
      virtual bool AcceptRules(const std::map<std::string, std::string> &map)  \
          const override;                                                      \
                                                                               \
      /* common type: "video", "audio", "image", "stream", etc.                \
         Even "video && audio".                                                \
         Empty "" means any type while nullptr means nothing.                  \
         self-defined type: "video:h264", etc.                                 \
         more ref to media_type.h */                                           \
      static const char *ExpectedInputDataType();                              \
      static const char *OutPutDataType();                                     \
                                                                               \
      )

#define DEFINE_MEDIA_CHILD_FACTORY_EXTRA(REAL_PRODUCT)                         \
  bool REAL_PRODUCT##Factory::AcceptRules(                                     \
      const std::map<std::string, std::string> &map) const {                   \
    static std::list<std::string> expected_data_type_list;                     \
    static std::list<std::string> out_data_type_list;                          \
    static const char *static_keys[] = {"input_data_type", "output_data_type", \
                                        NULL};                                 \
    static const decltype(ExpectedInputDataType) *static_call[] = {            \
        &ExpectedInputDataType, &OutPutDataType, NULL};                        \
    static std::list<std::string> *static_list[] = {                           \
        &expected_data_type_list, &out_data_type_list, NULL};                  \
    const char **keys = static_keys;                                           \
    const decltype(ExpectedInputDataType) **call = static_call;                \
    std::list<std::string> **list = static_list;                               \
    while (*keys) {                                                            \
      auto it = map.find(*keys);                                               \
      if (it == map.end()) {                                                   \
        if ((*call)())                                                         \
          return false;                                                        \
      } else {                                                                 \
        const std::string &value = it->second;                                 \
        if (!value.empty() &&                                                  \
            !has_intersection(value.c_str(), (*call)(), *list))                \
          return false;                                                        \
      }                                                                        \
      ++keys;                                                                  \
      ++call;                                                                  \
      ++list;                                                                  \
    }                                                                          \
    return true;                                                               \
  }

#endif // #ifndef RKMEDIA_MEDIA_REFLECTOR_H_
