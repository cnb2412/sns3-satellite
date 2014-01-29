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
 * Author: Jani Puttonen <jani.puttonen@magister.fi>
 */

#ifndef SATELLITE_GSE_HEADERS_H
#define SATELLITE_GSE_HEADERS_H


#include <vector>
#include "ns3/header.h"

/**
 * \ingroup satellite
 *
 * \brief SatGseHeader implementations.
 */
namespace ns3 {

class SatGseHeader : public Header
{
public:

  /**
   * Constructor
   */
  SatGseHeader ();
  ~SatGseHeader ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual void Print (std::ostream &os) const;

  /**
   * Get start indicator of GSE header
   * \return uint8_t start indicator
   */
  uint8_t GetStartIndicator () const;

  /**
   * Get end indicator of GSE header
   * \return uint8_t end indicator
   */
  uint8_t GetEndIndicator () const;

  /**
   * Get GSE fragment length in bytes
   * \return uint32_t PPDU length
   */
  uint32_t GetGsePduLength () const;

  /**
   * Get GSE fragment id
   * \return uint32_t fragment id
   */
  uint32_t GetFragmentId () const;

  /**
   * Get total length of higher layer PDU.
   * \return uint32_t total length of HL PDU
   */
  uint32_t GetTotalLength () const;

  /**
   * Set start indicator to GSE header
   */
  void SetStartIndicator ();

  /**
   * Set end indicator to GSE header
   */
  void SetEndIndicator ();

  /**
   * Set GSE fragment length to PPDU header
   * \param bytes GSE length in bytes
   */
  void SetGsePduLength (uint32_t bytes);

  /**
   * Set fragment id to GSE header
   * \param id Fragment id
   */
  void SetFragmentId (uint32_t id);

  /**
   * Set total length of higher layer PDU. Set in
   * START_PPDU status type.
   */
  void SetTotalLength (uint32_t bytes);

  /**
   * Get the maximum GSE header size
   * \return uint32_t header size
   */
  uint32_t GetGseHeaderSizeInBytes (uint8_t type) const;

  /**
   * Get the maximum GSE header size
   * \return uint32_t header size
   */
  uint32_t GetMaxGseHeaderSizeInBytes () const;

private:

  /**
   * GSE header content
   */
  uint8_t m_startIndicator;
  uint8_t m_endIndicator;
  uint16_t m_gsePduLengthInBytes;
  uint8_t m_fragmentId;
  uint16_t m_totalLengthInBytes;
  uint16_t m_protocolType;
  uint8_t m_labelByte;
  uint32_t m_crc;

  /**
   * Constant variables for determining the header sizes
   * of different GSE fragments.
   */
  const uint32_t m_fullGseHeaderSize;
  const uint32_t m_startGseHeaderSize;
  const uint32_t m_endGseHeaderSize;
  const uint32_t m_continuationGseHeaderSize;

  static const uint32_t m_labelFieldLengthInBytes = 3;
};

}; // namespace ns3

#endif // SATELLITE_GSE_HEADERS_H