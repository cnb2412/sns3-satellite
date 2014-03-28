/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Magister Solutions Ltd
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
 * Author: Jani Puttonen <jani.puttonen@magister.fi>
 */


#ifndef SATELLITE_CHANNEL_ESTIMATION_ERROR_CONTAINER_H_
#define SATELLITE_CHANNEL_ESTIMATION_ERROR_CONTAINER_H_

#include <map>
#include "ns3/object.h"
#include "satellite-channel-estimation-error.h"

namespace ns3 {

/**
 * \ingroup satellite
 * SatChannelEstimationErrorContainer is responsible of adding a channel
 * estimation error on top of raw measured SINR. Channel estimation error
 * functions in dB domain. Abstract base class SatChannelEstimationErrorContainer
 * defines the interface method, but the actual implementation is in inherited
 * classes:
 * - SatSimpleChannelEstimatinoErrorContainer - returns always SINR without errors
 * - SatFwdLinkChannelEstimationErrorContainer - uses FWD link error tables
 * - SatRtnLinkChannelEstimationErrorContainer - uses FWD link error tables
 */

class SatChannelEstimationErrorContainer : public SimpleRefCount<SatChannelEstimationErrorContainer>
{
public:
  SatChannelEstimationErrorContainer ();
  virtual ~SatChannelEstimationErrorContainer ();

  /**
   * Add channel estimation error to SINR. Base class does not actually
   * do anything, but returns only the sinrInDb as sinrOutDb.
   * \param sinrInDb Measured SINR in dB
   * @return double SINR including channel estimation error in dB
   */
  double AddError (double sinrInDb, uint32_t wfId = 0) const;

protected:
  virtual double DoAddError (double sinrInDb, uint32_t wfId) const = 0;

private:

};


class SatSimpleChannelEstimationErrorContainer : public SatChannelEstimationErrorContainer
{
public:
  SatSimpleChannelEstimationErrorContainer ();
  virtual ~SatSimpleChannelEstimationErrorContainer ();

protected:
  /**
   * Add channel estimation error to SINR. Base class does not actually
   * do anything, but returns only the sinrInDb as sinrOutDb.
   * \param sinrInDb Measured SINR in dB
   * @return double SINR including channel estimation error in dB
   */
  virtual double DoAddError (double sinrInDb, uint32_t wfId) const;

private:

};

class SatFwdLinkChannelEstimationErrorContainer : public SatChannelEstimationErrorContainer
{
public:
  SatFwdLinkChannelEstimationErrorContainer ();
  virtual ~SatFwdLinkChannelEstimationErrorContainer ();

protected:
  /**
   * Add channel estimation error to SINR
   * \param sinrInDb Measured SINR in dB
   * \return double SINR including channel estimation error in dB
   */
  virtual double DoAddError (double sinrInDb, uint32_t wfId) const;

private:

  /**
   * Only one channel estimator error configuration for
   * forward link.
   */
  Ptr<SatChannelEstimationError> m_channelEstimationError;

};

class SatRtnLinkChannelEstimationErrorContainer : public SatChannelEstimationErrorContainer
{
public:
  SatRtnLinkChannelEstimationErrorContainer (uint32_t minWfId, uint32_t maxWfId);
  virtual ~SatRtnLinkChannelEstimationErrorContainer ();

protected:
  /**
   * Add channel estimation error to SINR
   * \param sinrInDb Measured SINR in dB
   * \return double SINR including channel estimation error in dB
   */
  virtual double DoAddError (double sinrInDb, uint32_t wfId) const;

private:
  std::map<uint32_t, Ptr<SatChannelEstimationError> > m_channelEstimationErrors;
};

}



#endif /* SATELLITE_CHANNEL_ESTIMATION_ERROR_CONTAINER_H_ */