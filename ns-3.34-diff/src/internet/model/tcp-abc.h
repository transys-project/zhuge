/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 NITK Surathkal
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
 * Author: Shravya K.S. <shravya.ks0@gmail.com>
 *
 */

#ifndef TCP_ABC_H
#define TCP_ABC_H

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-linux-reno.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv4-header.h"

namespace ns3 {

/**
 * \ingroup tcp
 *
 * \brief An implementation of DCTCP. This model implements all of the
 * endpoint capabilities mentioned in the DCTCP SIGCOMM paper.
 */

class TcpABC : public TcpCongestionOps
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Create an unbound tcp socket.
   */
  TcpABC ();

  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  TcpABC (const TcpABC& sock);

  /**
   * \brief Destructor
   */
  virtual ~TcpABC (void);

  // Documented in base class
  virtual std::string GetName () const;

  /**
   * \brief Set configuration required by congestion control algorithm,
   *        This method will force DctcpEcn mode and will force usage of
   *        either ECT(0) or ECT(1) (depending on the 'UseEct0' attribute),
   *        despite any other configuration in the base classes.
   *
   * \param tcb internal congestion state
   */
  virtual void Init (Ptr<TcpSocketState> tcb);

  /**
   * TracedCallback signature for DCTCP update of congestion state
   *
   * \param [in] bytesAcked Bytes acked in this observation window
   * \param [in] bytesMarked Bytes marked in this observation window
   * \param [in] alpha New alpha (congestion estimate) value
   */
  typedef void (* CongestionEstimateTracedCallback)(uint32_t bytesAcked, uint32_t bytesMarked, double alpha);

  // Documented in base class
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);
  virtual bool HasCongControl () const;
  virtual void CongControl (Ptr<TcpSocketState> tcb,const TcpRateOps::TcpRateConnection &rc,
                            const TcpRateOps::TcpRateSample &rs);
  virtual Ptr<TcpCongestionOps> Fork ();
private:

};

} // namespace ns3

#endif /* TCP_DCTCP_H */

