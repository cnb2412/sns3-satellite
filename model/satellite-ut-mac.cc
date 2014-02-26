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

#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/double.h"
#include "ns3/random-variable.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/nstime.h"
#include "ns3/pointer.h"
#include "ns3/packet.h"
#include "ns3/ipv4-l3-protocol.h"
#include "satellite-ut-mac.h"
#include "satellite-enums.h"
#include "satellite-utils.h"
#include "satellite-tbtp-container.h"
#include "satellite-queue.h"
#include "../helper/satellite-wave-form-conf.h"

NS_LOG_COMPONENT_DEFINE ("SatUtMac");

namespace ns3 {


NS_OBJECT_ENSURE_REGISTERED (SatUtMac);

TypeId 
SatUtMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SatUtMac")
    .SetParent<SatMac> ()
    .AddConstructor<SatUtMac> ()
    .AddAttribute ("SuperframeSequence", "Superframe sequence containing information of superframes.",
                    PointerValue (),
                    MakePointerAccessor (&SatUtMac::m_superframeSeq),
                    MakePointerChecker<SatSuperframeSeq> ())
    .AddAttribute ("LowerLayerServiceConf",
                   "Pointer to lower layer service configuration.",
                   PointerValue (),
                   MakePointerAccessor (&SatUtMac::m_llsConf),
                   MakePointerChecker<SatLowerLayerServiceConf> ())
    .AddAttribute ("FramePduHeaderSize",
                   "Frame PDU header size in bytes",
                   UintegerValue (1),
                   MakeUintegerAccessor (&SatUtMac::m_framePduHeaderSizeInBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("GuardTime",
                   "Guard time in return link",
                   TimeValue (MicroSeconds (1)),
                   MakeTimeAccessor (&SatUtMac::m_guardTime),
                   MakeTimeChecker ())
  ;
  return tid;
}

TypeId
SatUtMac::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);

  return GetTypeId ();
}

SatUtMac::SatUtMac ()
 : SatMac (),
   m_superframeSeq (),
   m_timingAdvanceCb (0),
   m_ctrlCallback (0),
   m_llsConf (0),
   m_gwAddress (),
   m_framePduHeaderSizeInBytes (1),
   m_randomAccess (NULL),
   m_guardTime (MicroSeconds (1)),
   m_raChannel (0)
{
  NS_LOG_FUNCTION (this);

  // default construtctor should not be used
  NS_FATAL_ERROR ("SatUtMac::SatUtMac - Constructor not in use");
}

SatUtMac::SatUtMac (Ptr<SatSuperframeSeq> seq, uint32_t beamId)
 : SatMac (beamId),
   m_superframeSeq (seq),
   m_timingAdvanceCb (0),
   m_ctrlCallback (0),
   m_llsConf (0),
   m_gwAddress (),
   m_framePduHeaderSizeInBytes (1),
   m_guardTime (MicroSeconds (1)),
   m_raChannel (0)
{
	NS_LOG_FUNCTION (this);

  m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();
  m_tbtpContainer = CreateObject<SatTbtpContainer> ();
}

SatUtMac::~SatUtMac ()
{
  NS_LOG_FUNCTION (this);

  m_randomAccess = NULL;
  m_tbtpContainer = NULL;
}

void
SatUtMac::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_timingAdvanceCb.Nullify ();
  m_tbtpContainer->DoDispose ();

  SatMac::DoDispose ();
}

void
SatUtMac::SetGwAddress (Mac48Address gwAddress)
{
  NS_LOG_FUNCTION (this);

  m_gwAddress = gwAddress;
}

void
SatUtMac::SetNodeInfo (Ptr<SatNodeInfo> nodeInfo)
{
  NS_LOG_FUNCTION (this << nodeInfo);

  m_tbtpContainer->SetMacAddress (nodeInfo->GetMacAddress ());
  SatMac::SetNodeInfo (nodeInfo);
}

void
SatUtMac::SetRaChannel (uint32_t raChannel)
{
  NS_LOG_FUNCTION (this << raChannel);

  m_raChannel = raChannel;
}

uint32_t
SatUtMac::GetRaChannel () const
{
  NS_LOG_FUNCTION (this);

  return m_raChannel;
}

void
SatUtMac::SetRandomAccess (Ptr<SatRandomAccess> randomAccess)
{
  NS_LOG_FUNCTION (this);

  m_randomAccess = randomAccess;
  m_randomAccess->SetIsDamaAvailableCallback (MakeCallback(&SatTbtpContainer::HasScheduledTimeSlots, m_tbtpContainer));
}

void
SatUtMac::SetTimingAdvanceCallback (SatUtMac::TimingAdvanceCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_timingAdvanceCb = cb;
}

void
SatUtMac::SetCtrlMsgCallback (SatUtMac::SendCtrlCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);

  m_ctrlCallback = cb;
}


Time
SatUtMac::GetSuperFrameTxTime (uint8_t superFrameSeqId) const
{
  Time timingAdvance = m_timingAdvanceCb ();
  Time txTime = m_superframeSeq->GetSuperFrameTxTime (superFrameSeqId, timingAdvance);
  return txTime;
}

Time
SatUtMac::GetCurrentSuperFrameStartTime (uint8_t superFrameSeqId) const
{
  Time timingAdvance = m_timingAdvanceCb ();
  Time txTime = m_superframeSeq->GetCurrentSuperFrameStartTime (superFrameSeqId, timingAdvance);
  return txTime;
}

void
SatUtMac::ScheduleTimeSlots (Ptr<SatTbtpMessage> tbtp)
{
  NS_LOG_FUNCTION (this << tbtp);
  NS_LOG_LOGIC ("UT: " << m_nodeInfo->GetMacAddress () << " received TBTP " << tbtp->GetSuperframeCounter () << " at time: " << Simulator::Now ().GetSeconds ());

  /**
   * Calculate the sending time of the time slots within this TBTP for this specific UT.
   * UTs may be located at different distances from the satellite, thus they shall have to
   * send the time slots at different times so that the transmissions are received at the GW
   * at correct time.
   */
  Time timingAdvance = m_timingAdvanceCb ();
  Time txTime = m_superframeSeq->GetSuperFrameTxTimeWithCount (tbtp->GetSuperframeSeqId (), tbtp->GetSuperframeCounter (), timingAdvance);

  // The delay compared to Now when to start the transmission of this superframe
  Time startDelay = txTime - Simulator::Now ();

  // Add TBTP to a specific container
  m_tbtpContainer->Add (txTime, tbtp);

  // if the calculated start time of the superframe is already in the past
  if (txTime < Simulator::Now ())
    {
      NS_FATAL_ERROR ("UT: " << m_nodeInfo->GetMacAddress () << " received TBTP " << tbtp->GetSuperframeCounter () << ", which should have been sent already in the past");
    }

  // Schedule superframe start
  Simulator::Schedule (startDelay, &SatUtMac::SuperFrameStart, this, tbtp->GetSuperframeSeqId ());

  NS_LOG_LOGIC ("Time to start sending the superframe for this UT: " << txTime.GetSeconds ());
  NS_LOG_LOGIC ("Waiting delay before the superframe start: " << startDelay.GetSeconds ());

  //tbtp->Dump ();

  SatTbtpMessage::DaTimeSlotInfoContainer_t slots = tbtp->GetDaTimeslots (m_nodeInfo->GetMacAddress ());

  if ( !slots.empty ())
    {
      NS_LOG_LOGIC ("TBTP contains " << slots.size () << " timeslots for UT: " << m_nodeInfo->GetMacAddress ());

      uint8_t frameId = 0;

      // schedule time slots
      for ( SatTbtpMessage::DaTimeSlotInfoContainer_t::iterator it = slots.begin (); it != slots.end (); it++ )
        {
          // Store frame id from first slot and check later that frame id is same
          // If frame id changes in TBTP for same UT, raise error.
          if ( it == slots.begin () )
            {
              frameId = it->first;
            }
          else if ( frameId != it->first )
            {
              NS_FATAL_ERROR ("Error in TBTP: slot allocate from different frames for same UT!!!");
            }

          Ptr<SatSuperframeConf> superframeConf = m_superframeSeq->GetSuperframeConf (0);
          Ptr<SatFrameConf> frameConf = superframeConf->GetFrameConf (frameId);
          Ptr<SatTimeSlotConf> timeSlotConf = frameConf->GetTimeSlotConf ( it->second );

          // Start time
          Time slotDelay = startDelay + Seconds (timeSlotConf->GetStartTimeInSeconds ());
          NS_LOG_LOGIC ("Slot start delay: " << slotDelay.GetSeconds());

          // Duration
          Ptr<SatWaveform> wf = m_superframeSeq->GetWaveformConf()->GetWaveform (timeSlotConf->GetWaveFormId ());
          double duration = wf->GetBurstDurationInSeconds (frameConf->GetBtuConf ()->GetSymbolRateInBauds ());

          // Carrier
          uint32_t carrierId = m_superframeSeq->GetCarrierId (0, frameId, timeSlotConf->GetCarrierId () );

          ScheduleTxOpportunity (slotDelay, duration, wf->GetPayloadInBytes (), carrierId);
        }
    }
}

void
SatUtMac::SuperFrameStart (uint8_t superframeSeqId)
{
  NS_LOG_FUNCTION (this << superframeSeqId);
  NS_LOG_LOGIC ("Superframe start time at: " << Simulator::Now ().GetSeconds () << " for UT: " << m_nodeInfo->GetMacAddress ());

  /**
   * Here some functionality may be added for UT related to the superframe start time.
   */
}

void
SatUtMac::ScheduleTxOpportunity(Time transmitDelay, double durationInSecs, uint32_t payloadBytes, uint32_t carrierId)
{
  NS_LOG_FUNCTION (this << transmitDelay.GetSeconds() << durationInSecs << payloadBytes << carrierId);

  Simulator::Schedule (transmitDelay, &SatUtMac::Transmit, this, durationInSecs, payloadBytes, carrierId);
}

void
SatUtMac::Transmit (double durationInSecs, uint32_t payloadBytes, uint32_t carrierId)
{
  NS_LOG_FUNCTION (this << durationInSecs << payloadBytes << carrierId);
  NS_LOG_LOGIC ("Tx opportunity for UT: " << m_nodeInfo->GetMacAddress () << " at time: " << Simulator::Now ().GetSeconds () << ": duration: " << durationInSecs << ", payload: " << payloadBytes << ", carrier: " << carrierId);

  /**
   * TODO: the TBTP should hold also the RC_index for each time slot. Here, the RC_index
   * should be passed with txOpportunity to higher layer, so that it knows which RC_index
   * (= queue) to serve.
   */
   
  NS_ASSERT (payloadBytes > m_framePduHeaderSizeInBytes);

  /**
   * The frame PDU header is taken into account as an overhead,
   * thus the payload size of the time slot is reduced by a
   * configured frame PDU header size.
   */
  uint32_t payloadLeft = payloadBytes - m_framePduHeaderSizeInBytes;
  uint32_t bytesLeftInBuffer (0);

  // Packet container to be sent to lower layers.
  // Packet container models FPDU.
  SatPhy::PacketContainer_t packets;

  /**
   * Get new PPDUs from higher layer (LLC) until
   * - The payload is filled to the max OR
   * - The LLC returns NULL packet
   */
  while (payloadLeft > 0)
    {
      NS_LOG_LOGIC ("Tx opportunity: payloadLeft: " << payloadLeft);

      // TxOpportunity
      Ptr<Packet> p = m_txOpportunityCallback (payloadLeft, m_nodeInfo->GetMacAddress (), bytesLeftInBuffer);

      // A valid packet received
      if ( p )
        {
          NS_LOG_LOGIC ("Received a PPDU of size: " << p->GetSize ());

          // Add packet trace entry:
          m_packetTrace (Simulator::Now(),
                         SatEnums::PACKET_SENT,
                         m_nodeInfo->GetNodeType (),
                         m_nodeInfo->GetNodeId (),
                         m_nodeInfo->GetMacAddress (),
                         SatEnums::LL_MAC,
                         SatEnums::LD_RETURN,
                         SatUtils::GetPacketInfo (p));

          packets.push_back (p);
        }
      // LLC returned a NULL packet, break the loop
      else
        {
          break;
        }

      // Update the payloadLeft counter
      if (payloadLeft >= p->GetSize ())
        {
          payloadLeft -= p->GetSize ();
        }
      else
        {
          NS_FATAL_ERROR ("The PPDU was too big for the time slot!");
        }
    }

  NS_ASSERT (payloadLeft >= 0);
  NS_LOG_LOGIC ("The Frame PDU holds " << packets.size () << " packets");
  NS_LOG_LOGIC ("FPDU size:" << payloadBytes - payloadLeft);

  // If there are packets to send
  if (!packets.empty ())
    {
      // Decrease a guard time from time slot duration.
      Time duration (Time::FromDouble(durationInSecs, Time::S) - m_guardTime);
      NS_LOG_LOGIC ("Duration double: " << durationInSecs << " duration time: " << duration.GetSeconds ());
      NS_LOG_LOGIC ("UT: " << m_nodeInfo->GetMacAddress () << " send packet at time: " << Simulator::Now ().GetSeconds () << " duration: " << duration.GetSeconds ());

      SendPacket (packets, carrierId, duration);

    }
}

void
SatUtMac::ReceiveQueueEvent (SatQueue::QueueEvent_t event, uint8_t rcIndex)
{
  NS_LOG_FUNCTION (this << event << rcIndex);

  if (rcIndex == 0)
    {
      if (event == SatQueue::FIRST_BUFFERED_PKT || event == SatQueue::BUFFERED_PKT)
        {
          NS_LOG_LOGIC ("Buffered packet event received from queue: " << rcIndex);

          if (m_randomAccess != NULL)
            {
              DoRandomAccess (SatRandomAccess::RA_SLOTTED_ALOHA_TRIGGER);
            }
        }
    }
}

void
SatUtMac::Receive (SatPhy::PacketContainer_t packets, Ptr<SatSignalParameters> /*rxParams*/)
{
  NS_LOG_FUNCTION (this);

  // Add packet trace entry:
  m_packetTrace (Simulator::Now(),
                 SatEnums::PACKET_RECV,
                 m_nodeInfo->GetNodeType (),
                 m_nodeInfo->GetNodeId (),
                 m_nodeInfo->GetMacAddress (),
                 SatEnums::LL_MAC,
                 SatEnums::LD_FORWARD,
                 SatUtils::GetPacketInfo (packets));

  for (SatPhy::PacketContainer_t::iterator i = packets.begin (); i != packets.end (); i++ )
    {
      // Remove packet tag
      SatMacTag macTag;
      bool mSuccess = (*i)->PeekPacketTag (macTag);

      if (!mSuccess)
        {
          NS_FATAL_ERROR ("MAC tag was not found from the packet!");
        }

      NS_LOG_LOGIC("Packet from " << macTag.GetSourceAddress () << " to " << macTag.GetDestAddress ());
      NS_LOG_LOGIC("Receiver " << m_nodeInfo->GetMacAddress ());

      Mac48Address destAddress = Mac48Address::ConvertFrom (macTag.GetDestAddress ());
      if (destAddress == m_nodeInfo->GetMacAddress () || destAddress.IsBroadcast () || destAddress.IsGroup ())
        {
          // Remove control msg tag
          SatControlMsgTag ctrlTag;
          bool cSuccess = (*i)->PeekPacketTag (ctrlTag);

          if (cSuccess)
            {
              SatControlMsgTag::SatControlMsgType_t cType = ctrlTag.GetMsgType ();

              if ( cType != SatControlMsgTag::SAT_NON_CTRL_MSG )
                {
                  // Remove the mac tag
                  (*i)->RemovePacketTag (macTag);
                  ReceiveSignalingPacket (*i, ctrlTag);
                }
              else
                {
                  NS_FATAL_ERROR ("A control message received with not valid msg type!");
                }
            }
          else if (destAddress.IsBroadcast ())
            {
              // TODO: dummy frames and other broadcast needed to handle
              // dummy frames should ignored already in Phy layer
            }
          // Control msg tag not found, send the packet to higher layer
          else
            {
              // Pass the receiver address to LLC
              m_rxCallback (*i, destAddress);
            }
        }
    }
}

void
SatUtMac::ReceiveSignalingPacket (Ptr<Packet> packet, SatControlMsgTag ctrlTag)
{
  NS_LOG_FUNCTION (this);

  switch (ctrlTag.GetMsgType ())
  {
    case SatControlMsgTag::SAT_TBTP_CTRL_MSG:
      {
        uint32_t tbtpId = ctrlTag.GetMsgId ();

        Ptr<SatTbtpMessage> tbtp = DynamicCast<SatTbtpMessage> (m_readCtrlCallback (tbtpId));

        if ( tbtp == NULL )
          {
            NS_FATAL_ERROR ("TBTP not found, check that TBTP storage time is set long enough for superframe sequence!!!");
          }

        ScheduleTimeSlots (tbtp);
        break;
      }
    case SatControlMsgTag::SAT_RA_CTRL_MSG:
    case SatControlMsgTag::SAT_CR_CTRL_MSG:
      {
        NS_FATAL_ERROR ("SatUtMac received a non-supported control packet!");
        break;
      }
    default:
      {
        NS_FATAL_ERROR ("SatUtMac received a non-supported control packet!");
        break;
      }
  }
}

void
SatUtMac::DoRandomAccess (SatRandomAccess::RandomAccessTriggerType_t randomAccessTriggerType)
{
  NS_LOG_FUNCTION (this);

  SatRandomAccess::RandomAccessTxOpportunities_s txOpportunities;

  /// select the RA allocation channel
  uint32_t allocationChannel = GetNextRandomAccessAllocationChannel ();

  /// run random access algorithm
  txOpportunities = m_randomAccess->DoRandomAccess (allocationChannel, randomAccessTriggerType);

  /// process Slotted ALOHA Tx opportunities
  if (txOpportunities.txOpportunityType == SatRandomAccess::RA_SLOTTED_ALOHA_TX_OPPORTUNITY)
    {
      Time txOpportunity = Time::FromInteger (txOpportunities.slottedAlohaTxOpportunity, Time::MS);

      /// schedule the check for next available RA slot
      Simulator::Schedule (txOpportunity, &SatUtMac::FindNextAvailableRandomAccessSlot, this, allocationChannel);
    }
  /// process CRDSA Tx opportunities
  else if (txOpportunities.txOpportunityType == SatRandomAccess::RA_CRDSA_TX_OPPORTUNITY)
    {
      /// schedule CRDSA transmission
      ScheduleCrdsaTransmission (txOpportunities);
    }
}

uint32_t
SatUtMac::GetNextRandomAccessAllocationChannel ()
{
  NS_LOG_FUNCTION (this);

  /// at the moment allocation channel is only randomly selected
  return m_uniformRandomVariable->GetInteger (0, m_llsConf->GetRaServiceCount ());
}

void
SatUtMac::FindNextAvailableRandomAccessSlot (uint32_t allocationChannel)
{
  /// TODO find the next slot taking into account the already used time slots in this frame

  Time currentSuperframeStartTime = GetCurrentSuperFrameStartTime (0);
  uint32_t usedSlot = 10;

  /// update used slots
  UpdateUsedRandomAccessSlots (allocationChannel, usedSlot);

  /// TODO schedule transmission from the control queue at the matching slot
}

void
SatUtMac::ScheduleCrdsaTransmission (SatRandomAccess::RandomAccessTxOpportunities_s txOpportunities)
{
  /// TODO schedule CRDSA transmissions

  /// update used slots
  UpdateUsedRandomAccessSlots (txOpportunities);
}

void
SatUtMac::UpdateUsedRandomAccessSlots (SatRandomAccess::RandomAccessTxOpportunities_s txOpportunities)
{
  std::map < std::pair <uint32_t, uint32_t>, std::set<uint32_t> >::iterator iter;

  /// TODO this needs to be implemented
  uint32_t currentFrameId = 5;

  /// remove past RA Tx opportunity information
  RemovePastRandomAccessSlots (currentFrameId);

  std::pair <uint32_t, uint32_t> key = std::make_pair (currentFrameId, txOpportunities.crdsaTxOpportunities.first);

  iter = m_usedRandomAccessSlots.find (key);

  if (iter == m_usedRandomAccessSlots.end ())
    {
      std::pair <std::map < std::pair <uint32_t, uint32_t>, std::set<uint32_t> >::iterator, bool> result;

      result = m_usedRandomAccessSlots.insert (std::make_pair (key, txOpportunities.crdsaTxOpportunities.second));

      if (!result.second)
        {
          NS_FATAL_ERROR ("SatUtMac::UpdateUsedRandomAccessSlots - Insert failed");
        }
    }
  else
    {
      std::set<uint32_t>::iterator setIter;
      std::pair<std::set<uint32_t>::iterator,bool> result;

      for (setIter = txOpportunities.crdsaTxOpportunities.second.begin (); setIter != txOpportunities.crdsaTxOpportunities.second.end (); setIter++)
        {
          result = iter->second.insert (*setIter);

          if (!result.second)
            {
              NS_FATAL_ERROR ("SatUtMac::UpdateUsedRandomAccessSlots - Insert failed");
            }
        }
    }
}

void
SatUtMac::UpdateUsedRandomAccessSlots (uint32_t allocationChannelId, uint32_t slotId)
{
  std::map < std::pair <uint32_t, uint32_t>, std::set<uint32_t> >::iterator iter;

  /// TODO this needs to be implemented
  uint32_t currentFrameId = 5;

  /// remove past RA Tx opportunity information
  RemovePastRandomAccessSlots (currentFrameId);

  std::pair <uint32_t, uint32_t> key = std::make_pair (currentFrameId, allocationChannelId);

  iter = m_usedRandomAccessSlots.find (key);

  if (iter == m_usedRandomAccessSlots.end ())
    {
      std::set<uint32_t> txOpportunities;
      std::pair <std::map < std::pair <uint32_t, uint32_t>, std::set<uint32_t> >::iterator, bool> result;

      txOpportunities.insert (slotId);

      result = m_usedRandomAccessSlots.insert (std::make_pair (key, txOpportunities));

      if (!result.second)
        {
          NS_FATAL_ERROR ("SatUtMac::UpdateUsedRandomAccessSlots - Insert failed");
        }
    }
  else
    {
      std::pair<std::set<uint32_t>::iterator,bool> result;
      result = iter->second.insert (slotId);

      if (!result.second)
        {
          NS_FATAL_ERROR ("SatUtMac::UpdateUsedRandomAccessSlots - Insert failed");
        }
    }
}

void
SatUtMac::RemovePastRandomAccessSlots (uint32_t currentFrameId)
{
  std::map < std::pair <uint32_t, uint32_t>, std::set<uint32_t> >::iterator iter;

  /// TODO this functionality needs to be checked!
  for (iter = m_usedRandomAccessSlots.begin (); iter != m_usedRandomAccessSlots.end (); )
    {
      if (iter->first.first < currentFrameId)
        {
          m_usedRandomAccessSlots.erase(iter++);
        }
      else
        {
          ++iter;
        }
    }
}

} // namespace ns3
