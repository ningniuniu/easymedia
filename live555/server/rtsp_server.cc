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

#include "flow.h"

#include <time.h>

#include <BasicUsageEnvironment/BasicUsageEnvironment.hh>
#ifndef _RTSP_SERVER_HH
#include <liveMedia/RTSPServer.hh>
#endif

#include "h264_server_media_subsession.hh"
#include "live555_media_input.hh"

#include "buffer.h"
#include "media_config.h"
#include "media_reflector.h"
#include "media_type.h"

namespace rkmedia {

static bool SendVideoToServer(Flow *f, MediaBufferVector &input_vector);
// static bool SendAudioToServer(Flow *f, MediaBufferVector &input_vector);

class RtspServerFlow : public Flow {
public:
  RtspServerFlow(const char *param);
  virtual ~RtspServerFlow();
  static const char *GetFlowName() { return "live555_rtsp_server"; }

private:
  void service_session_run();

  TaskScheduler *scheduler;
  UsageEnvironment *env;
  UserAuthenticationDatabase *authDB;
  Live555MediaInput *server_input;
  RTSPServer *rtspServer;
  std::thread *session_thread;
  volatile char out_loop_cond;

  friend bool SendVideoToServer(Flow *f, MediaBufferVector &input_vector);
  // friend bool SendAudioToServer(Flow *f, MediaBufferVector &input_vector);
};

bool SendVideoToServer(Flow *f, MediaBufferVector &input_vector) {
  RtspServerFlow *rtsp_flow = (RtspServerFlow *)f;
  auto &buffer = input_vector[0];
  if (buffer && buffer->IsHwBuffer()) {
    // hardware buffer is limited, copy it
    auto new_buffer = MediaBuffer::Clone(*buffer.get());
    buffer = new_buffer;
  }
  rtsp_flow->server_input->PushNewVideo(buffer);
  return true;
}

#if 0
bool SendAudioToServer(Flow *f, MediaBufferVector &input_vector) {
  RtspServerFlow *rtsp_flow = (RtspServerFlow *)f;
  rtsp_flow->server_input->PushNewAudio(input_vector[1]);
  return true;
}
#endif

RtspServerFlow::RtspServerFlow(const char *param)
    : scheduler(nullptr), env(nullptr), authDB(nullptr), server_input(nullptr),
      rtspServer(nullptr), session_thread(nullptr), out_loop_cond(1) {
  std::list<std::string> input_data_types;
  std::string channel_name;
  std::map<std::string, std::string> params;
  if (!parse_media_param_map(param, params)) {
    errno = EINVAL;
    return;
  }
  std::string value;
  CHECK_EMPTY_SETERRNO(value, params, KEY_INPUTDATATYPE, EINVAL)
  parse_media_param_list(value.c_str(), input_data_types, ',');
  CHECK_EMPTY_SETERRNO(channel_name, params, KEY_CHANNEL_NAME, EINVAL)
  int idx = 0;
  int ports[3] = {0, 554, 8554};
  value = params[KEY_PORT_NUM];
  if (!value.empty()) {
    int port = std::stoi(value);
    if (port != 554 && port != 8554)
      ports[0] = port;
  }
  std::string &username = params[KEY_USERNAME];
  std::string &userpwd = params[KEY_USERPASSWORD];
  if (!username.empty() && !userpwd.empty()) {
    authDB = new UserAuthenticationDatabase;
    if (!authDB)
      goto err;
    authDB->addUserRecord(username.c_str(), userpwd.c_str());
  }
  scheduler = BasicTaskScheduler::createNew();
  if (!scheduler)
    goto err;
  env = BasicUsageEnvironment::createNew(*scheduler);
  if (!env)
    goto err;
  server_input = Live555MediaInput::createNew(*env);
  if (!server_input)
    goto err;
  while (idx < 3 && !rtspServer) {
    int port = ports[idx++];
    if (port <= 0)
      continue;
    rtspServer = RTSPServer::createNew(*env, port, authDB, 1000);
  }
  if (rtspServer) {
    int in_idx = 0;
    std::string markname;
    char *url = nullptr;
    time_t t;
    t = time(&t);
    ServerMediaSession *sms =
        ServerMediaSession::createNew(*env, channel_name.c_str(), ctime(&t),
                                      "rtsp stream server", False /*UNICAST*/);
    if (!sms) {
      *env << "Error: Failed to create ServerMediaSession: "
           << env->getResultMsg() << "\n";
      goto err;
    }
    rtspServer->addServerMediaSession(sms);
    url = rtspServer->rtspURL(sms);
    *env << "Play this stream using the URL:\n\t" << url << "\n";
    if (url)
      delete[] url;
    for (auto &type : input_data_types) {
      SlotMap sm;
      ServerMediaSubsession *subsession = nullptr;
      if (type == VIDEO_H264) {
        subsession =
            H264ServerMediaSubsession::createNew(sms->envir(), *server_input);
        sm.process = SendVideoToServer;
      } else if (string_start_withs(type, AUDIO_PREFIX)) {
        // pcm or vorbis
        LOG_TODO();
        goto err;
      } else {
        LOG("TODO, unsupport type : %s\n", type.c_str());
        goto err;
      }
      if (!subsession)
        goto err;
      sm.input_slots.push_back(in_idx); // video
      sm.thread_model = Model::SYNC;
      sm.mode_when_full = InputMode::BLOCKING;
      sm.input_maxcachenum.push_back(0); // no limit
      markname = "rtsp " + channel_name + std::to_string(in_idx);
      if (!InstallSlotMap(sm, markname, 0)) {
        LOG("Fail to InstallSlotMap, %s\n", markname.c_str());
        goto err;
      }
      sms->addSubsession(subsession);
      in_idx++;
    }
  } else {
    goto err;
  }
  *env << "...rtsp done initializing\n";
  out_loop_cond = 0;
  session_thread = new std::thread(&RtspServerFlow::service_session_run, this);
  if (!session_thread) {
    LOG_NO_MEMORY();
    goto err;
  }
  errno = 0;
  return;
err:
  errno = EINVAL;
}

RtspServerFlow::~RtspServerFlow() {
  AutoPrintLine apl(__func__);
  StopAllThread();
  SetDisable();
  out_loop_cond = 1;
  if (session_thread) {
    session_thread->join();
    delete session_thread;
    session_thread = nullptr;
  }
  if (rtspServer) {
    // will also reclaim ServerMediaSession and ServerMediaSubsessions
    Medium::close(rtspServer);
    rtspServer = nullptr;
  }
  if (server_input) {
    delete server_input;
    server_input = nullptr;
  }
  if (authDB) {
    delete authDB;
    authDB = nullptr;
  }
  if (env && env->reclaim() == True)
    env = nullptr;
  if (scheduler) {
    delete scheduler;
    scheduler = nullptr;
  }
}

void RtspServerFlow::service_session_run() {
  AutoPrintLine apl(__func__);
  env->taskScheduler().doEventLoop(&out_loop_cond);
}

DEFINE_FLOW_FACTORY(RtspServerFlow, Flow)
const char *FACTORY(RtspServerFlow)::ExpectedInputDataType() { return ""; }
const char *FACTORY(RtspServerFlow)::OutPutDataType() { return ""; }

} // namespace rkmedia
