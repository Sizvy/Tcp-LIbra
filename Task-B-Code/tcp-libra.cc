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
#include "tcp-libra.h"
#include "ns3/log.h"
#include "tcp-socket-state.h"

namespace ns3 {

// Libra
NS_LOG_COMPONENT_DEFINE ("TcpLibra");
NS_OBJECT_ENSURE_REGISTERED (TcpLibra);

TypeId
TcpLibra::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpLibra")
    .SetParent<TcpNewReno> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpLibra> ()
  ;
  return tid;
}

TcpLibra::TcpLibra (void) 
: TcpNewReno (),
    m_sumRtt (Time (0)),
    m_baseRtt (Time::Max ()),
    m_maxRtt (Time::Min ()),
    m_cntRtt (0),
    m_alpha (10.0),
    m_lastRtt (Seconds (0.0))
{
  NS_LOG_FUNCTION (this);
}

TcpLibra::TcpLibra (const TcpLibra& sock)
  : TcpNewReno (sock),
    m_sumRtt (sock.m_sumRtt),
    m_baseRtt (sock.m_baseRtt),
    m_maxRtt (sock.m_maxRtt),
    m_cntRtt (sock.m_cntRtt),
    m_alpha (sock.m_alpha),
    m_lastRtt(sock.m_lastRtt)
{
  NS_LOG_FUNCTION (this);
}

TcpLibra::~TcpLibra (void)
{
}

uint32_t
TcpLibra::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked >= 1)
    {
      tcb->m_cWnd += tcb->m_segmentSize;
      NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
      return segmentsAcked - 1;
    }

  return 0;
}

void
TcpLibra::CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (segmentsAcked > 0)
    {
      //std::cout<<"Delay: "<<CalculateMaxDelay()<<std::endl;
      //CalculatePenaltyFactor();
      CalculateAlpha();
      //std::cout<<"Congestion Avoidance Updated baseRtt = " << m_baseRtt << " maxRtt = " << m_maxRtt <<
      //         " sumRtt = " << m_sumRtt<<" lastRtt: "<<m_lastRtt<<std::endl;
      double T0 = 1.0;
      double RTT = static_cast<double>(m_lastRtt.GetSeconds());
      // double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ();
      // std::cout<<"In cong avoid alpha: "<<m_alpha<<std::endl;
      double myAdder = (m_alpha*RTT*RTT)/((T0+RTT)*tcb->m_cWnd.Get());
      // std::cout<<" My adder: "<<myAdder<<" Window: "<<tcb->m_cWnd.Get()<<std::endl;
      //adder = std::max (1.0, myAdder);
      tcb->m_cWnd += static_cast<uint32_t> (myAdder);
      NS_LOG_INFO ("In CongAvoid, updated to cwnd " << tcb->m_cWnd <<
                   " ssthresh " << tcb->m_ssThresh);
    }
}

void
TcpLibra::HandleWindowForDupAck(Ptr<TcpSocketState> tcb)
{

  NS_LOG_FUNCTION (this << tcb);

  double T1 = 1.0;
  double T0 = 1.0;
  double RTT = static_cast<double>(m_lastRtt.GetSeconds());
  double minus = (T1*tcb->m_cWnd)/(2*(T0+RTT));
  tcb->m_cWnd = tcb->m_cWnd - static_cast<uint32_t>(minus);
  // std::cout<<"Minus: "<<minus<<" CWND: "<<tcb->m_cWnd<<std::endl;
}

void
TcpLibra::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (tcb->m_cWnd < tcb->m_ssThresh)
  {
    segmentsAcked = SlowStart (tcb, segmentsAcked);
  }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
  {
    CongestionAvoidance (tcb, segmentsAcked);
  }
}

std::string
TcpLibra::GetName () const
{
  return "TcpLibra";
}

void
TcpLibra::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t packetsAcked,
                        const Time &rtt)
{
  NS_LOG_FUNCTION (this << tcb << packetsAcked << rtt);

  if (rtt.IsZero ())
    {
      return;
    }

  //std::cout<<"Before Updated baseRtt = " << m_baseRtt << " maxRtt = " << m_maxRtt <<
  //             " sumRtt = " << m_sumRtt<<" lastRtt: "<<m_lastRtt<<std::endl;

  // Keep track of minimum RTT
  m_baseRtt = std::min (m_baseRtt, rtt);

  // Keep track of maximum RTT
  m_maxRtt = std::max (rtt, m_maxRtt);

  m_sumRtt += rtt;

  ++m_cntRtt;
  // keep track of the last rtt sample
  m_lastRtt = rtt;

  //std::cout<<"After Updated baseRtt = " << m_baseRtt << " maxRtt = " << m_maxRtt <<
  //             " sumRtt = " << m_sumRtt<<" lastRtt: "<<m_lastRtt<<" "<<m_cntRtt<<" "<<packetsAcked<<std::endl;

  NS_LOG_INFO ("Updated baseRtt = " << m_baseRtt << " maxRtt = " << m_maxRtt <<
               " sumRtt = " << m_sumRtt<<" lastRtt: "<<m_lastRtt);
}

Time
TcpLibra::CalculateAvgDelay () const
{
  NS_LOG_FUNCTION (this);

   return (m_sumRtt / m_cntRtt - m_baseRtt);
  //return (m_lastRtt - m_baseRtt);
}

Time
TcpLibra::CalculateMaxDelay () const
{
  NS_LOG_FUNCTION (this);
  //std::cout << "Max: "<<m_maxRtt<<"Min: "<<m_baseRtt<<std::endl;
  return (m_maxRtt - m_baseRtt);
}

double
TcpLibra::CalculateScalabilityFactor() const
{
    NS_LOG_FUNCTION (this);

    double k1 = 2.0;
    double Cr = 100000000/8; // bottleneck capacity = 100Mbps

    double S = k1*Cr;
    //std::cout<<"Scale: "<<S<<std::endl;
    return S;
}

double
TcpLibra::CalculatePenaltyFactor() const
{
    NS_LOG_FUNCTION (this);
    double Qavg = 1.0;
    double Qmax = 1.0;
    if(m_cntRtt > 0){
      Qavg = static_cast<double> (CalculateMaxDelay ().GetMilliSeconds ());
      Qmax = static_cast<double> (CalculateAvgDelay ().GetMilliSeconds ());
    }

    //std::cout<<"Qavg: "<<Qavg<<" Qmax: "<<Qmax<<std::endl;

    double k2 = 2.0;
    double powerExp = k2*Qavg/Qmax;

    double P = exp(-powerExp);
    //std::cout<<"Penalty: "<<P<<std::endl;
    return P;
}

void
TcpLibra::CalculateAlpha()
{
    NS_LOG_FUNCTION (this);

    m_alpha = (CalculatePenaltyFactor() * CalculateScalabilityFactor());   
    //std::cout<<"In calc alpha: "<<m_alpha<<std::endl;
}

uint32_t
TcpLibra::GetSsThresh (Ptr<const TcpSocketState> state,
                         uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);
  double T1 = 1.0;
  double T0 = 1.0;
  uint32_t temp = state->m_cWnd;
  double RTT = static_cast<double>(m_lastRtt.GetSeconds());
  double minus = (T1*state->m_cWnd)/(2*(T0+RTT));
  temp = temp - static_cast<uint32_t>(minus);
  // std::cout<<"bytesInFlight "<< bytesInFlight <<" max: "<< std::max (2 * state->m_segmentSize, bytesInFlight / 2)<<" "<<temp<<std::endl;
  // return temp;
  return std::max (temp, bytesInFlight / 2);
  // return std::max (2 * state->m_segmentSize, bytesInFlight / 2);
}

Ptr<TcpCongestionOps>
TcpLibra::Fork ()
{
  return CopyObject<TcpLibra> (this);
}

} // namespace ns3

