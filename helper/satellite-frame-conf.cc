/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Sami Rantanen <sami.rantanen@magister.fi>
 */

#include "ns3/log.h"

#include "satellite-frame-conf.h"

NS_LOG_COMPONENT_DEFINE ("SatFrameConf");

namespace ns3 {

// BTU conf

SatBtuConf::SatBtuConf ()
{
  // default constructor should not be used
  NS_ASSERT (false);
}

SatBtuConf::SatBtuConf (double bandwidth_hz, double length_s, double symbolRate_baud)
  : m_bandwidth_hz (bandwidth_hz),
    m_length_s (length_s),
    m_symbolRate_baud (symbolRate_baud)
{
  NS_LOG_FUNCTION (this);
}

SatBtuConf::~SatBtuConf ()
{
  NS_LOG_FUNCTION (this);
}

// Time Slot conf

SatTimeSlotConf::SatTimeSlotConf ()
{
  // default constructor should not be used
  NS_ASSERT (false);
}

SatTimeSlotConf::SatTimeSlotConf (double startTime_s, double duration_s, uint8_t waveFormId, uint32_t frameCarrierId)
  : m_startTime_s (startTime_s),
    m_duration_s (duration_s),
    m_waveFormId (waveFormId),
    m_frameCarrierId (frameCarrierId)
{
  NS_LOG_FUNCTION (this);
}

SatTimeSlotConf::~SatTimeSlotConf ()
{
  NS_LOG_FUNCTION (this);
}

// Frame conf

SatFrameConf::SatFrameConf ()
{
  // default constructor should not be used
  NS_ASSERT (false);
}

SatFrameConf::SatFrameConf ( double bandwidth_hz, double duration_s,
                             Ptr<SatBtuConf> btu, std::vector<Ptr<SatTimeSlotConf> > * timeSlots)
  : m_bandwidth_hz (bandwidth_hz),
    m_duration_s (duration_s),
    m_btu (btu)
{
  NS_LOG_FUNCTION (this);

  m_carrierCount = bandwidth_hz / btu->GetBandwidth_hz();

  if ( timeSlots != NULL )
    m_timeSlots = *timeSlots;
}

SatFrameConf::~SatFrameConf ()
{
  NS_LOG_FUNCTION (this);
}

double
SatFrameConf::GetCarrierFrequency_hz (uint32_t carrierId) const
{
  NS_ASSERT (carrierId < m_carrierCount);

  double carrierBandwidth_hz = m_btu->GetBandwidth_hz();

  return ( (carrierBandwidth_hz * carrierId) + ( carrierBandwidth_hz / 2.0 ) );
}

Ptr<SatTimeSlotConf>
SatFrameConf::GetTimeSlotConf (uint16_t index) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT ( index < m_timeSlots.size() );

  return m_timeSlots[index];
}

void
SatFrameConf::AddTimeSlotConf ( Ptr<SatTimeSlotConf> conf)
{
  NS_LOG_FUNCTION (this);

  m_timeSlots.push_back(conf);
}

// Super frame conf

SatSuperframeConf::SatSuperframeConf ()
{
  // default constructor should not be used
  NS_ASSERT (false);
}

SatSuperframeConf::SatSuperframeConf ( double bandwidth_hz, double duration_s,
                                       std::vector<Ptr<SatFrameConf> > * frames)
  : m_bandwidth_hz (bandwidth_hz),
    m_duration_s (duration_s)

{
  NS_LOG_FUNCTION (this);

  if ( frames != NULL)
    {
      m_frames = *frames;
    }
}

SatSuperframeConf::~SatSuperframeConf ()
{
  NS_LOG_FUNCTION (this);
}

void
SatSuperframeConf::AddFrameConf (Ptr<SatFrameConf> conf)
{
  NS_LOG_FUNCTION (this);

  m_frames.push_back (conf);
}

Ptr<SatFrameConf>
SatSuperframeConf::GetFrameConf (uint8_t index) const
{
  NS_LOG_FUNCTION (this);

  return m_frames[index];
}

uint32_t
SatSuperframeConf::GetCarrierCount () const
{
  uint32_t carrierCount = 0;

  for (uint8_t i = 0; i < m_frames.size(); i++)
    {
      carrierCount += m_frames[i]->GetCarrierCount();
    }

  return carrierCount;
}

uint32_t
SatSuperframeConf::GetCarrierId ( uint8_t frameId, uint16_t frameCarrierId ) const
{
  uint32_t carrierId = frameCarrierId;

  for (uint8_t i = 0; i < frameId; i++)
    {
      NS_ASSERT ( i < m_frames.size());

      carrierId += m_frames[i]->GetCarrierCount();
    }

  return carrierId;
}

double
SatSuperframeConf::GetCarrierFrequency_hz (uint32_t carrierId) const
{
  double frameStartFrequency = 0.0;
  uint32_t currentFrame = 0;
  uint32_t lastIdInFrame = m_frames[0]->GetCarrierCount() - 1;
  uint32_t carrierIdInFrame = carrierId;

  while( carrierId > lastIdInFrame )
    {
      carrierIdInFrame -= m_frames[currentFrame]->GetCarrierCount();
      frameStartFrequency += m_frames[currentFrame]->GetBandwidth_hz();
      currentFrame++;
      lastIdInFrame += m_frames[currentFrame]->GetCarrierCount();
    }

  double carrierFrequencyInFrame = m_frames[currentFrame]->GetCarrierFrequency_hz( carrierIdInFrame );

  return frameStartFrequency + carrierFrequencyInFrame;
}

double
SatSuperframeConf::GetCarrierBandwidth_hz (uint32_t carrierId) const
{
  uint32_t currentFrame = 0;
  uint32_t lastIdInFrame = m_frames[0]->GetCarrierCount() - 1;
  uint32_t carrierIdInFrame = carrierId;

  while( carrierId > lastIdInFrame )
    {
      carrierIdInFrame -= m_frames[currentFrame]->GetCarrierCount();
      currentFrame++;
      lastIdInFrame += m_frames[currentFrame]->GetCarrierCount();
    }

  return m_frames[currentFrame]->GetCarrierBandwidth_hz();
}


}; // namespace ns3