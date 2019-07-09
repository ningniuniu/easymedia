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
  virtual ~FilterFlow() { StopAllThread(); }
  static const char *GetFlowName() { return "filter"; }

private:
  std::vector<std::shared_ptr<Filter>> filters;
  bool support_async;
  Model thread_model;
  PixelFormat input_pix_fmt; // a hack for rga copy yuyv, by set fake rgb565
  std::vector<std::shared_ptr<MediaBuffer>> out_buffers;
  size_t out_index;

  friend bool do_filters(Flow *f, MediaBufferVector &input_vector);
};

FilterFlow::FilterFlow(const char *param)
    : support_async(true), thread_model(Model::NONE),
      input_pix_fmt(PIX_FMT_NONE), out_index(0) {
  std::list<std::string> separate_list;
  if (!parse_media_param_list(param, separate_list, ' ') ||
      separate_list.size() != 2) {
    SetError(-EINVAL);
    return;
  }
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(separate_list.front().c_str(), params)) {
    SetError(-EINVAL);
    return;
  }
  std::string &name = params[KEY_NAME];
  if (name.empty()) {
    LOG("missing filter name\n");
    SetError(-EINVAL);
    return;
  }
  const char *filter_name = name.c_str();
  // check input/output type
  std::string &&rule = gen_datatype_rule(params);
  if (rule.empty()) {
    SetError(-EINVAL);
    return;
  }
  if (!REFLECTOR(Filter)::IsMatch(filter_name, rule.c_str())) {
    LOG("Unsupport for filter %s : [%s]\n", filter_name, rule.c_str());
    SetError(-EINVAL);
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
    SetError(-EINVAL);
    return;
  }
  int input_idx = 0;
  for (auto &param_str : param_list) {
    auto filter =
        REFLECTOR(Filter)::Create<Filter>(filter_name, param_str.c_str());
    if (!filter) {
      LOG("Fail to create filter %s<%s>\n", filter_name, param_str.c_str());
      SetError(-EINVAL);
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
    SetError(-EINVAL);
    return;
  }
  if (filters[0]->SendInput(nullptr) == -1 && errno == ENOSYS) {
    support_async = false;
  } else {
    LOG_TODO();
    SetError(-EINVAL);
    return;
  }
  if (!support_async) {
    ImageInfo info;
    if (!ParseImageInfoFromMap(params, info, false)) {
      SetError(-EINVAL);
      return;
    }
    int out_cache_num = 2;
    const std::string &str_ocn = params[KEY_OUTPUT_CACHE_NUM];
    if (!str_ocn.empty()) {
      out_cache_num = std::stoi(str_ocn);
      if (out_cache_num <= 0)
        out_cache_num = 2;
    }
    for (int i = 0; i < out_cache_num; i++) {
      size_t size = CalPixFmtSize(info);
      auto &&mb =
          MediaBuffer::Alloc2(size, MediaBuffer::MemType::MEM_HARD_WARE);
      auto out_buffer = std::make_shared<ImageBuffer>(mb, info);
      if (!out_buffer || out_buffer->GetSize() < size) {
        SetError(-ENOMEM);
        return;
      }
      out_buffer->SetValidSize(size);
      out_buffers.push_back(out_buffer);
    }
  }
}

// comparing timestamp as modification?
bool do_filters(Flow *f, MediaBufferVector &input_vector) {
  FilterFlow *flow = static_cast<FilterFlow *>(f);
  int i = 0;
  bool has_valid_input = false;
  std::shared_ptr<Filter> last_filter;
  assert(flow->filters.size() == input_vector.size());
  for (auto &filter : flow->filters) {
    auto &in = input_vector[i];
    if (!in) {
      i++;
      continue;
    }
    if (!has_valid_input)
      has_valid_input = true;
    last_filter = filter;
    if (flow->input_pix_fmt != PIX_FMT_NONE && in->GetType() == Type::Image) {
      auto in_img = std::static_pointer_cast<ImageBuffer>(in);
      // hack for n4 cif
      if (flow->input_pix_fmt != in_img->GetPixelFormat()) {
        ImageInfo &info = in_img->GetImageInfo();
        int flow_num = 0, flow_den = 0;
        GetPixFmtNumDen(flow->input_pix_fmt, flow_num, flow_den);
        int in_num = 0, in_den = 0;
        GetPixFmtNumDen(info.pix_fmt, in_num, in_den);
        int num = in_num * flow_den;
        int den = in_den * flow_num;
        info.width = info.width * num / den;
        info.vir_width = info.vir_width * num / den;
        info.pix_fmt = flow->input_pix_fmt;
      }
    }
    if (flow->support_async) {
      int ret = filter->SendInput(in);
      if (ret == -EAGAIN)
        flow->SendInput(in, i);
    } else {
      if (filter->Process(in, flow->out_buffers[flow->out_index]))
        return false;
    }
    i++;
  }
  if (!has_valid_input)
    return true;
  if (!flow->support_async) {
    flow->SetOutput(flow->out_buffers[flow->out_index], 0);
    if (++flow->out_index >= flow->out_buffers.size())
      flow->out_index = 0;
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

DEFINE_FLOW_FACTORY(FilterFlow, Flow)
// TODO!
const char *FACTORY(FilterFlow)::ExpectedInputDataType() { return ""; }
// TODO!
const char *FACTORY(FilterFlow)::OutPutDataType() { return ""; }

} // namespace easymedia
