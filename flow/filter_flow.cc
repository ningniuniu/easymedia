/*
 * Copyright (C) 2019 Hertz Wang 1989wanghang@163.com
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

#include <assert.h>

#include "buffer.h"
#include "filter.h"
#include "flow.h"
#include "image.h"
#include "key_string.h"

namespace easymedia {

static bool do_filters(Flow *f, MediaBufferVector &input_vector);

class FilterFlow : public Flow {
public:
  FilterFlow(const char *param);
  virtual ~FilterFlow() = default;
  static const char *GetFlowName() { return "filter"; }

private:
  std::vector<std::shared_ptr<Filter>> filters;
  bool support_async;
  Model thread_model;
  PixelFormat input_pix_fmt; // a hack for rga copy yuyv, by set fake rgb565
  std::shared_ptr<MediaBuffer> out_buffer;

  friend bool do_filters(Flow *f, MediaBufferVector &input_vector);
};

FilterFlow::FilterFlow(const char *param)
    : support_async(true), thread_model(Model::NONE),
      input_pix_fmt(PIX_FMT_NONE) {
  std::list<std::string> separate_list;
  if (!parse_media_param_list(param, separate_list, ' ') ||
      separate_list.size() != 2) {
    errno = EINVAL;
    return;
  }
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(separate_list.front().c_str(), params)) {
    errno = EINVAL;
    return;
  }
  std::string &name = params[KEY_NAME];
  if (name.empty()) {
    LOG("missing filter name\n");
    errno = EINVAL;
    return;
  }
  const char *filter_name = name.c_str();
  // check input/output type
  std::string &&rule = gen_datatype_rule(params);
  if (rule.empty()) {
    errno = EINVAL;
    return;
  }
  if (!REFLECTOR(Filter)::IsMatch(filter_name, rule.c_str())) {
    LOG("Unsupport for filter %s : [%s]\n", filter_name, rule.c_str());
    errno = EINVAL;
    return;
  }
  input_pix_fmt = GetPixFmtByString(params[KEY_INPUTDATATYPE].c_str());
  SlotMap sm;
  int input_maxcachenum = 2;
  ParseParamToSlotMap(params, sm, input_maxcachenum);
  if (sm.thread_model == Model::NONE)
    sm.thread_model =
        !params[KEY_FPS].empty() ? Model::ASYNCATOMIC : Model::SYNC;
  thread_model = sm.thread_model;
  if (sm.mode_when_full == InputMode::NONE)
    sm.mode_when_full = InputMode::DROPCURRENT;

  std::string &filter_param = separate_list.back();
  std::list<std::string> param_list;
  if (!parse_media_param_list(filter_param.c_str(), param_list)) {
    errno = EINVAL;
    return;
  }
  int input_idx = 0;
  for (auto &param_str : param_list) {
    auto filter =
        REFLECTOR(Filter)::Create<Filter>(filter_name, param_str.c_str());
    if (!filter && !filter->AttachBufferArgs(nullptr)) {
      LOG("Fail to create filter %s<%s>\n", filter_name, param_str.c_str());
      errno = EINVAL;
      return;
    }
    filters.push_back(filter);
    sm.input_slots.push_back(input_idx++);
    sm.input_maxcachenum.push_back(input_maxcachenum);
  }
  sm.output_slots.push_back(0);
  sm.process = do_filters;
  if (!InstallSlotMap(sm, name, -1)) {
    LOG("Fail to InstallSlotMap, %s\n", filter_name);
    errno = EINVAL;
    return;
  }
  if (filters[0]->SendInput(nullptr) == -1 && errno == ENOSYS) {
    support_async = false;
  } else {
    LOG_TODO();
    errno = EINVAL;
    return;
  }
  if (!support_async) {
    ImageInfo info;
    if (!ParseImageInfoFromMap(params, info, false)) {
      errno = EINVAL;
      return;
    }
    size_t size = CalPixFmtSize(info);
    auto &&mb = MediaBuffer::Alloc2(size, MediaBuffer::MemType::MEM_HARD_WARE);
    out_buffer = std::make_shared<ImageBuffer>(mb, info);
    if (!out_buffer || out_buffer->GetSize() < size) {
      errno = ENOMEM;
      return;
    }
    out_buffer->SetValidSize(size);
  }
  errno = 0;
}

bool do_filters(Flow *f, MediaBufferVector &input_vector) {
  FilterFlow *flow = static_cast<FilterFlow *>(f);
  int i = 0;
  std::shared_ptr<Filter> last_filter;
  assert(flow->filters.size() == input_vector.size());
  for (auto &filter : flow->filters) {
    auto &in = input_vector[i];
    if (!in)
      continue;
    last_filter = filter;
    if (flow->input_pix_fmt != PIX_FMT_NONE && in->GetType() == Type::Image) {
      auto in_img = std::static_pointer_cast<ImageBuffer>(in);
      if (flow->input_pix_fmt != in_img->GetPixelFormat())
        in_img->GetImageInfo().pix_fmt = flow->input_pix_fmt;
    }
    if (flow->support_async) {
      int ret = filter->SendInput(in);
      if (ret == -EAGAIN)
        flow->SendInput(in, i);
    } else {
      if (filter->Process(in, flow->out_buffer))
        return false;
    }
    i++;
  }
  if (!flow->support_async) {
    flow->SetOutput(flow->out_buffer, 0);
  } else if (flow->thread_model == Model::SYNC) {
    do {
      auto out = last_filter->FetchOutput();
      if (!out)
        break;
      flow->SetOutput(out, 0);
    } while (true);
  }
  return true;
}

} // namespace easymedia
