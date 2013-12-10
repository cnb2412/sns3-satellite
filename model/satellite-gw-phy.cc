/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions Ltd.
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
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"

#include "satellite-utils.h"
#include "satellite-gw-phy.h"
#include "satellite-phy-rx.h"
#include "satellite-phy-tx.h"
#include "satellite-channel.h"
#include "satellite-mac.h"
#include "satellite-signal-parameters.h"


NS_LOG_COMPONENT_DEFINE ("SatGwPhy");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SatGwPhy);

TypeId
SatGwPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SatGwPhy")
    .SetParent<SatPhy> ()
    .AddConstructor<SatGwPhy> ()
    .AddAttribute ("PhyRx", "The PhyRx layer attached to this phy.",
                    PointerValue (),
                    MakePointerAccessor (&SatPhy::GetPhyRx, &SatPhy::SetPhyRx),
                    MakePointerChecker<SatPhyRx> ())
    .AddAttribute ("PhyTx", "The PhyTx layer attached to this phy.",
                    PointerValue (),
                    MakePointerAccessor (&SatPhy::GetPhyTx, &SatPhy::SetPhyTx),
                    MakePointerChecker<SatPhyTx> ())
    .AddAttribute( "RxTemperatureDbk",
                   "RX noise temperature in GW in dBK.",
                    DoubleValue (24.62),  // ~290K
                    MakeDoubleAccessor (&SatPhy::GetRxNoiseTemperatureDbk, &SatPhy::SetRxNoiseTemperatureDbk),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("RxMaxAntennaGainDb", "Maximum RX gain in dB",
                    DoubleValue (61.50),
                    MakeDoubleAccessor (&SatPhy::GetRxAntennaGainDb, &SatPhy::SetRxAntennaGainDb),
                    MakeDoubleChecker<double_t> ())
    .AddAttribute ("TxMaxAntennaGainDb", "Maximum TX gain in dB",
                    DoubleValue (65.20),
                    MakeDoubleAccessor (&SatPhy::GetTxAntennaGainDb, &SatPhy::SetTxAntennaGainDb),
                    MakeDoubleChecker<double_t> ())
    .AddAttribute ("TxMaxPowerDbw", "Maximum TX power in dB",
                    DoubleValue (8.97),
                    MakeDoubleAccessor (&SatPhy::GetTxMaxPowerDbw, &SatPhy::SetTxMaxPowerDbw),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxOutputLossDb", "TX Output loss in dB",
                    DoubleValue (2.00),
                    MakeDoubleAccessor (&SatPhy::GetTxOutputLossDb, &SatPhy::SetTxOutputLossDb),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxPointingLossDb", "TX Pointing loss in dB",
                    DoubleValue (1.10),
                    MakeDoubleAccessor (&SatPhy::GetTxPointingLossDb, &SatPhy::SetTxPointingLossDb),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxOboLossDb", "TX OBO loss in dB",
                    DoubleValue (6.00),
                    MakeDoubleAccessor (&SatPhy::GetTxOboLossDb, &SatPhy::SetTxOboLossDb),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("TxAntennaLossDb", "TX Antenna loss in dB",
                    DoubleValue (0.00),
                    MakeDoubleAccessor (&SatPhy::GetTxAntennaLossDb, &SatPhy::SetTxAntennaLossDb),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("RxAntennaLossDb", "RX Antenna loss in dB",
                    DoubleValue (0.00),
                    MakeDoubleAccessor (&SatPhy::GetRxAntennaLossDb, &SatPhy::SetRxAntennaLossDb),
                    MakeDoubleChecker<double> ())
    .AddAttribute ("DefaultFadingValue", "Default value for fading",
                    DoubleValue (1.00),
                    MakeDoubleAccessor (&SatPhy::GetDefaultFading, &SatPhy::SetDefaultFading),
                    MakeDoubleChecker<double_t> ())
    .AddAttribute( "ImIfCOverIDb",
                   "Intermodultation interference, C over I in dB.",
                    DoubleValue (22.0),
                    MakeDoubleAccessor (&SatGwPhy::m_imInterferenceCOverIDb),
                    MakeDoubleChecker<double> ())
    .AddAttribute( "AciIfWrtNoisePercent",
                   "Adjacent channel interference wrt white noise in percents.",
                    DoubleValue (10.0),
                    MakeDoubleAccessor (&SatGwPhy::m_aciIfWrtNoisePercent),
                    MakeDoubleChecker<double> (0, 100))
  ;
  return tid;
}

TypeId
SatGwPhy::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);

  return GetTypeId ();
}

SatGwPhy::SatGwPhy (void)
{
  NS_LOG_FUNCTION (this);
  NS_FATAL_ERROR ("SatGwPhy default constructor is not allowed to use");
}

SatGwPhy::SatGwPhy (SatPhy::CreateParam_t& params, ErrorModel errorModel, Ptr<SatLinkResults> linkResults,
                    InterferenceModel ifModel, CarrierBandwidthConverter converter, uint32_t carrierCount)
  : SatPhy (params)
{
  NS_LOG_FUNCTION (this);

  ObjectBase::ConstructSelf(AttributeConstructionList ());

  m_imInterferenceCOverI = SatUtils::DbToLinear (m_imInterferenceCOverIDb);

  Ptr<SatPhyRxCarrierConf> carrierConf =
              CreateObject<SatPhyRxCarrierConf> (SatPhy::GetRxNoiseTemperatureDbk(),
                                                 errorModel,
                                                 ifModel,
                                                 SatPhyRxCarrierConf::NORMAL,
                                                 SatEnums::RETURN_FEEDER_CH,
                                                 converter,
                                                 carrierCount);

  carrierConf->SetAttribute ("RxAciIfWrtNoise", DoubleValue (m_aciIfWrtNoisePercent) );

  if (linkResults)
    {
       carrierConf->SetLinkResults (linkResults);
    }

  carrierConf->SetSinrCalculatorCb (MakeCallback (&SatGwPhy::CalculateSinr, this));

  SatPhy::ConfigureRxCarriers (carrierConf);
}

SatGwPhy::~SatGwPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
SatGwPhy::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  SatPhy::DoDispose ();
}

void
SatGwPhy::DoStart ()
{
  NS_LOG_FUNCTION (this);
  SatPhy::DoStart ();
}

double
SatGwPhy::CalculateSinr (double sinr)
{
  NS_LOG_FUNCTION (this << sinr);

  if ( sinr <= 0  )
    {
      NS_FATAL_ERROR ( "Calculated own SINR is expected to be greater than zero!!!");
    }

  // calculate final SINR taken into account configured additional interferences (C over I)
  // in addition to CCI which is included in given SINR

  double finalSinr = 1 / ( (1 / sinr) + (1 / m_imInterferenceCOverI) );

  return (finalSinr);
}


} // namespace ns3