/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: Hertz Wang wangh@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_H264_SERVER_MEDIA_SUBSESSION_HH_
#define RKMEDIA_H264_SERVER_MEDIA_SUBSESSION_HH_

#include <list>
#include <mutex>

#include "live555_media_input.hh"
#include <liveMedia/OnDemandServerMediaSubsession.hh>

namespace rkmedia {
class H264ServerMediaSubsession : public OnDemandServerMediaSubsession {
public:
  static H264ServerMediaSubsession *createNew(UsageEnvironment &env,
                                              Live555MediaInput &wisInput);
  void setDoneFlag() { fDoneFlag = ~0; }
  void checkForAuxSDPLine1();

protected: // we're a virtual base class
  H264ServerMediaSubsession(UsageEnvironment &env,
                            Live555MediaInput &mediaInput);
  virtual ~H264ServerMediaSubsession();
  void startStream(
      unsigned clientSessionId, void *streamToken, TaskFunc *rtcpRRHandler,
      void *rtcpRRHandlerClientData, unsigned short &rtpSeqNum,
      unsigned &rtpTimestamp,
      ServerRequestAlternativeByteHandler *serverRequestAlternativeByteHandler,
      void *serverRequestAlternativeByteHandlerClientData) override;
  void deleteStream(unsigned clientSessionId, void *&streamToken) override;

protected:
  Live555MediaInput &fMediaInput;
  unsigned fEstimatedKbps;

private: // redefined virtual functions
  virtual char const *sdpLines();
  virtual char const *getAuxSDPLine(RTPSink *rtpSink,
                                    FramedSource *inputSource);
  virtual FramedSource *createNewStreamSource(unsigned clientSessionId,
                                              unsigned &estBitrate);
  virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
                                    FramedSource *inputSource);

private:
  char fDoneFlag;         // used when setting up 'SDPlines'
  RTPSink *fDummyRTPSink; // ditto
  int fGetSdpTimeOut;

  static std::mutex kMutex;
  static std::list<unsigned int> kSessionIdList;

  enum { INITIAL, GETTING_SDP_LINES, GET_SDP_LINES_TIMEOUT, GOT_SDP_LINES };
  int sdpState;
};
} // namespace rkmedia

#endif // #ifndef RKMEDIA_H264_SERVER_MEDIA_SUBSESSION_HH_