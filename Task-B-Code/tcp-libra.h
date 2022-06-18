/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
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
 */
#ifndef TCPLIBRA_H
#define TCPLIBRA_H

#include "tcp-congestion-ops.h"

namespace ns3 {

/**
 * \ingroup tcp
 * \defgroup congestionOps Congestion Control Algorithms.
 *
 * The various congestion control algorithms, also known as "TCP flavors".
 */

/**
 * \ingroup congestionOps
 *
 * \brief Congestion control abstract class
 *
 * The design is inspired by what Linux v4.0 does (but it has been
 * in place for years). The congestion control is split from the main
 * socket code, and it is a pluggable component. An interface has been defined;
 * variables are maintained in the TcpSocketState class, while subclasses of
 * TcpCongestionOps operate over an instance of that class.
 *
 * Only three methods have been implemented right now; however, Linux has many others,
 * which can be added later in ns-3.
 *
 * \see IncreaseWindow
 * \see PktsAcked
 */

/**
 * \brief The NewReno implementation
 *
 * New Reno introduces partial ACKs inside the well-established Reno algorithm.
 * This and other modifications are described in RFC 6582.
 *
 * \see IncreaseWindow
 */
class TcpLibra : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpLibra ();

  /**
   * \brief Copy constructor.
   * \param sock object to copy.
   */
  TcpLibra (const TcpLibra& sock);

  ~TcpLibra ();

  virtual std::string GetName () const;

  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);
  virtual Ptr<TcpCongestionOps> Fork ();
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);
  virtual void HandleWindowForDupAck(Ptr<TcpSocketState> tcb);

protected:
  virtual uint32_t SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual void CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
private:
  Time CalculateAvgDelay () const;
  Time CalculateMaxDelay () const;
  double CalculateScalabilityFactor() const;
  // We achieve scalability by adjusting the scalability factor to the capacity of the narrow
  // link. To compute the latter, we use packet pair techniques that are run embedded into the
  //  algorithm.
  double CalculatePenaltyFactor() const;
  // The penalty factor P has been designed in order to adapt the source sending rate
  // increase to the network congestion. As usual, congestion is measured by connection
  // backlog time (i.e. the difference between RTT and minimum RTT).
  void CalculateAlpha();
private:
  Time m_sumRtt;             //!< Sum of all RTT measurements during last RTT
  Time m_baseRtt;            //!< Minimum of all RTT measurements
  Time m_maxRtt;             //!< Maximum of all RTT measurements
  uint32_t m_cntRtt;         //!< Number of RTT measurements during last RTT
  double m_alpha;            //!< Additive increase factor
  Time m_lastRtt;               // Current rtt
};

} // namespace ns3

#endif
