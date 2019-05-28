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

#include "h264_server_media_subsession.hh"

#include <liveMedia/H264VideoRTPSink.hh>
#include <liveMedia/H264VideoStreamDiscreteFramer.hh>

#include "utils.h"

namespace easymedia {
H264ServerMediaSubsession *
H264ServerMediaSubsession::createNew(UsageEnvironment &env,
                                     Live555MediaInput &wisInput) {
  return new H264ServerMediaSubsession(env, wisInput);
}

H264ServerMediaSubsession::H264ServerMediaSubsession(
    UsageEnvironment &env, Live555MediaInput &mediaInput)
    : OnDemandServerMediaSubsession(env, True /*reuse the first source*/),
      fMediaInput(mediaInput), fEstimatedKbps(1000), fDoneFlag(0),
      fDummyRTPSink(NULL), fGetSdpTimeOut(1000 * 10), sdpState(INITIAL) {}

H264ServerMediaSubsession::~H264ServerMediaSubsession() {
  LOG_FILE_FUNC_LINE();
}

std::mutex H264ServerMediaSubsession::kMutex;
std::list<unsigned int> H264ServerMediaSubsession::kSessionIdList;

void H264ServerMediaSubsession::startStream(
    unsigned clientSessionId, void *streamToken, TaskFunc *rtcpRRHandler,
    void *rtcpRRHandlerClientData, unsigned short &rtpSeqNum,
    unsigned &rtpTimestamp,
    ServerRequestAlternativeByteHandler *serverRequestAlternativeByteHandler,
    void *serverRequestAlternativeByteHandlerClientData) {
  OnDemandServerMediaSubsession::startStream(
      clientSessionId, streamToken, rtcpRRHandler, rtcpRRHandlerClientData,
      rtpSeqNum, rtpTimestamp, serverRequestAlternativeByteHandler,
      serverRequestAlternativeByteHandlerClientData);
  kMutex.lock();
  if (kSessionIdList.empty())
    fMediaInput.Start(envir());
  LOG("%s - clientSessionId: 0x%08x\n", __func__, clientSessionId);
  kSessionIdList.push_back(clientSessionId);
  kMutex.unlock();
}
void H264ServerMediaSubsession::deleteStream(unsigned clientSessionId,
                                             void *&streamToken) {
  kMutex.lock();
  LOG("%s - clientSessionId: 0x%08x\n", __func__, clientSessionId);
  kSessionIdList.remove(clientSessionId);
  if (kSessionIdList.empty())
    fMediaInput.Stop(envir());
  kMutex.unlock();
  OnDemandServerMediaSubsession::deleteStream(clientSessionId, streamToken);
}

static void afterPlayingDummy(void *clientData) {
  H264ServerMediaSubsession *subsess = (H264ServerMediaSubsession *)clientData;
  LOG("%s, set done.\n", __func__);
  // Signal the event loop that we're done:
  subsess->setDoneFlag();
}

static void checkForAuxSDPLine(void *clientData) {
  H264ServerMediaSubsession *subsess = (H264ServerMediaSubsession *)clientData;
  subsess->checkForAuxSDPLine1();
}

void H264ServerMediaSubsession::checkForAuxSDPLine1() {
  LOG("** fDoneFlag: %d, fGetSdpTimeOut: %d **\n", fDoneFlag, fGetSdpTimeOut);
  if (fDummyRTPSink->auxSDPLine() != NULL) {
    // Signal the event loop that we're done:
    LOG("%s, set done.\n", __func__);
    setDoneFlag();
  } else {
    if (fGetSdpTimeOut <= 0) {
      LOG("warning: get sdp time out, set done.\n");
      sdpState = GET_SDP_LINES_TIMEOUT;
      setDoneFlag();
    } else {
      // try again after a brief delay:
      int uSecsToDelay = 100000; // 100 ms
      nextTask() = envir().taskScheduler().scheduleDelayedTask(
          uSecsToDelay, (TaskFunc *)checkForAuxSDPLine, this);
      fGetSdpTimeOut -= uSecsToDelay;
    }
  }
}

char const *
H264ServerMediaSubsession::getAuxSDPLine(RTPSink *rtpSink,
                                         FramedSource *inputSource) {
  // Note: For MPEG-4 video buffer, the 'config' information isn't known
  // until we start reading the Buffer.  This means that "rtpSink"s
  // "auxSDPLine()" will be NULL initially, and we need to start reading
  // data from our buffer until this changes.
  sdpState = GETTING_SDP_LINES;
  fDoneFlag = 0;
  fDummyRTPSink = rtpSink;
  fGetSdpTimeOut = 100000 * 10;
  // Start reading the buffer:
  fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);
  LOG_FILE_FUNC_LINE();
  // Check whether the sink's 'auxSDPLine()' is ready:
  checkForAuxSDPLine(this);

  envir().taskScheduler().doEventLoop(&fDoneFlag);
  LOG_FILE_FUNC_LINE();
  char const *auxSDPLine = fDummyRTPSink->auxSDPLine();
  LOG("++ auxSDPLine: %p\n", auxSDPLine);
  if (auxSDPLine)
    sdpState = GOT_SDP_LINES;
  return auxSDPLine;
}

char const *H264ServerMediaSubsession::sdpLines() {
  if (!fSDPLines)
    sdpState = INITIAL;
  char const *ret = OnDemandServerMediaSubsession::sdpLines();
  if (sdpState == GET_SDP_LINES_TIMEOUT) {
    if (fSDPLines) {
      delete[] fSDPLines;
      fSDPLines = NULL;
    }
    ret = NULL;
  }
  return ret;
}

FramedSource *
H264ServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/,
                                                 unsigned &estBitrate) {
  estBitrate = fEstimatedKbps;
  if (sdpState == GETTING_SDP_LINES || sdpState == GET_SDP_LINES_TIMEOUT) {
    LOG("sdpline is not ready, can not create new stream source\n");
    return NULL;
  }
  LOG_FILE_FUNC_LINE();
  // Create a framer for the Video Elementary Stream:
  FramedSource *source = H264VideoStreamDiscreteFramer::createNew(
      envir(), fMediaInput.videoSource());
  LOG("h264 framedsource : %p\n", source);
  return source;
}

RTPSink *H264ServerMediaSubsession::createNewRTPSink(
    Groupsock *rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,
    FramedSource *inputSource) {
  if (!inputSource) {
    LOG("inputSource is not ready, can not create new rtp sink\n");
    return NULL;
  }
  setVideoRTPSinkBufferSize();
  LOG_FILE_FUNC_LINE();
  RTPSink *rtp_sink = H264VideoRTPSink::createNew(envir(), rtpGroupsock,
                                                  rtpPayloadTypeIfDynamic);
  LOG("h264 rtp sink : %p\n", rtp_sink);
  return rtp_sink;
}

} // namespace easymedia
