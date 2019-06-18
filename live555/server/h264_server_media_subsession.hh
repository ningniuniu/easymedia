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

#ifndef EASYMEDIA_H264_SERVER_MEDIA_SUBSESSION_HH_
#define EASYMEDIA_H264_SERVER_MEDIA_SUBSESSION_HH_

#include <list>
#include <mutex>

#include "live555_media_input.hh"
#include <liveMedia/OnDemandServerMediaSubsession.hh>

namespace easymedia {
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
} // namespace easymedia

#endif // #ifndef EASYMEDIA_H264_SERVER_MEDIA_SUBSESSION_HH_