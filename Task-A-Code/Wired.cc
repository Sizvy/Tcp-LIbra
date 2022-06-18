/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, IMDEA Networks Institute
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
 * Author: Hany Assasa <hany.assasa@gmail.com>
.*
 * This is a simple example to test TCP over 802.11n (with MPDU aggregation enabled).
 *

// Default Network Topology
//
//   LAN 10.1.2.0
//                 
//  ================
//  |    |    |    |    10.1.1.0
//  n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.3.0

*/

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/tcp-westwood.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

#include "ns3/traffic-control-module.h"

#include "ns3/energy-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"

#include "ns3/netanim-module.h"

NS_LOG_COMPONENT_DEFINE ("fullwired");

using namespace ns3;

std::ofstream goodput("./Task_B/goodput_B_Bic.txt");
Ptr<PacketSink> sink;                         /* Pointer to the packet sink application */
uint64_t lastTotalRx = 0;                     /* The value of the last total received bytes */

uint32_t checkTimes;
double avgQueueSize;


std::stringstream filePlotQueue;
std::stringstream filePlotQueueAvg;

void
CalculateGoodput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
  goodput << now.GetSeconds () <<" "<< cur << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateGoodput);
}

void
CheckQueueSize (Ptr<QueueDisc> queue)
{
  uint32_t qSize = queue->GetCurrentSize ().GetValue ();

  avgQueueSize += qSize;
  checkTimes++;

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.01), &CheckQueueSize, queue);

  std::ofstream fPlotQueue (filePlotQueue.str ().c_str (), std::ios::out|std::ios::app);
  fPlotQueue << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueue.close ();

  std::ofstream fPlotQueueAvg (filePlotQueueAvg.str ().c_str (), std::ios::out|std::ios::app);
  fPlotQueueAvg << Simulator::Now ().GetSeconds () << " " << avgQueueSize / checkTimes << std::endl;
  fPlotQueueAvg.close ();
}


int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1000;                       /* Transport layer payload size in bytes. */
  std::string dataRate = "100Mbps";                  /* Application layer datarate. */
  std::string tcpVariant = "TcpNewReno";             /* TCP variant type. */
  std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */
  double simulationTime = 10;                        /* Simulation time in seconds. */

  std::string redLinkDataRate = "1.5Mbps";
  std::string redLinkDelay = "20ms";
  uint32_t redTest=0;

  uint32_t nCsma = 49;
  int flow = 50;

  /* Command line argument parser setup. */
  CommandLine cmd (__FILE__);
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("flow", "Number of flow", flow);
  cmd.AddValue ("redTest", "Do red test", redTest);

  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Application data ate", dataRate);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpNewReno, "
                "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat ", tcpVariant);
  cmd.AddValue ("phyRate", "Physical layer bitrate", phyRate);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.Parse (argc, argv);

  tcpVariant = std::string ("ns3::") + tcpVariant;
  // Select TCP variant
  TypeId tcpTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcpVariant, &tcpTid), "TypeId " << tcpVariant << " not found");
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));

  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

  uint32_t meanPktSize = 1000;

  //RED params
  Config::SetDefault ("ns3::RedQueueDisc::MaxSize", StringValue ("1000p"));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (meanPktSize));
  Config::SetDefault ("ns3::RedQueueDisc::Wait", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (0.002));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (5));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (15));

  //TrafficControlHelper tchPfifo;
  //uint16_t handle = tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc");
  //tchPfifo.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));

  TrafficControlHelper tchRed;
  tchRed.SetRootQueueDisc ("ns3::RedQueueDisc", "LinkBandwidth", StringValue (redLinkDataRate),
                          "LinkDelay", StringValue (redLinkDelay));

   
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  if(redTest==1){
    pointToPoint.SetQueue ("ns3::DropTailQueue");
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue (redLinkDataRate));
    pointToPoint.SetChannelAttribute ("Delay", StringValue (redLinkDelay));
  }
  else{
    pointToPoint.SetQueue ("ns3::DropTailQueue");
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue (dataRate));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));
  }
  
  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes_left;
  csmaNodes_left.Add (p2pNodes.Get (0));
  csmaNodes_left.Create (nCsma);

  NodeContainer csmaNodes_right;
  csmaNodes_right.Add (p2pNodes.Get (1));
  csmaNodes_right.Create (nCsma);

  CsmaHelper csma_left;
  csma_left.SetQueue("ns3::DropTailQueue");
  csma_left.SetChannelAttribute ("DataRate", StringValue (dataRate));
  csma_left.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  CsmaHelper csma_right;
  csma_right.SetQueue("ns3::DropTailQueue");
  csma_right.SetChannelAttribute ("DataRate", StringValue (dataRate));
  csma_right.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices_left;
  csmaDevices_left = csma_left.Install (csmaNodes_left);

  NetDeviceContainer csmaDevices_right;
  csmaDevices_right = csma_right.Install (csmaNodes_right);

  /* Internet stack */
  InternetStackHelper stack;
  //stack.Install (p2pNodes);
  stack.Install (csmaNodes_left);
  stack.Install (csmaNodes_right);

  QueueDiscContainer queueDiscs = tchRed.Install (p2pDevices);
  //tchPfifo.Install (p2pDevices);
  //tchPfifo.Install(csmaDevices);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces_left;
  csmaInterfaces_left = address.Assign (csmaDevices_left);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces_right;
  csmaInterfaces_right = address.Assign (csmaDevices_right);


  /* Populate routing table */
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("./Task_B/routingTable.cwnd");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Ipv4GlobalRoutingHelper::PrintRoutingTableAllAt(Seconds(1.5),stream,ns3::Time::S);

  /* Install TCP Receiver on the access point */
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.000000001));

  p2pDevices.Get(0)->SetAttribute("ReceiveErrorModel",PointerValue(em));
  p2pDevices.Get(1)->SetAttribute("ReceiveErrorModel",PointerValue(em));

  for(int i = 0; i < flow; i++){
    csmaDevices_left.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    csmaDevices_right.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
   
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9+i));
    ApplicationContainer sinkApp = sinkHelper.Install (csmaNodes_left.Get(i)); 
    sink = StaticCast<PacketSink> (sinkApp.Get (0));

    /* Install TCP/UDP Transmitter on the station */
    OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (csmaInterfaces_left.GetAddress (i), 9+i)));
    server.SetAttribute ("PacketSize", UintegerValue (payloadSize));
    server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
    ApplicationContainer serverApp = server.Install (csmaNodes_right.Get(i)); // server node assign

    /* Start Applications */
    sinkApp.Start (Seconds (0.0));
    serverApp.Start (Seconds (1.0));
  }

  std::string pathOut;
  pathOut = "./Task_B"; // Current directory

  Simulator::Schedule (Seconds (1.1), &CalculateGoodput);

  filePlotQueue << pathOut << "/" << "red-queue.plotme";
  filePlotQueueAvg << pathOut << "/" << "red-queue_avg.plotme";

  remove (filePlotQueue.str ().c_str ());
  remove (filePlotQueueAvg.str ().c_str ());
  Ptr<QueueDisc> queue = queueDiscs.Get (1);
  // Simulator::ScheduleNow (&CheckQueueSize, queue);

  /* Flow Monitor */
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor=flowHelper.InstallAll();

  flowMonitor->CheckForLostPackets ();

  Simulator::Stop (Seconds (simulationTime + 1));
  //AnimationInterface anim ("./lastFiles/update_hybrid.xml");
  Simulator::Run ();

   /* Flow Monitor File  */
  flowMonitor->SerializeToXmlFile("./Task_B/fullwired.flowmonitor",false,false);

  //See implementation of goodput here: https://www.nsnam.org/doxygen/traffic-control_8cc_source.html
  //delaySum: the sum of all end-to-end delays for all received packets of the flow
  //Fairness: https://github.com/urstrulymahesh/Networks-Simulator-ns3-
  //Queuing Policy: https://www.nsnam.org/docs/release/3.14/models/html/queue.html

  double averageGoodput = ((sink->GetTotalRx () * 8) / (1e6 * simulationTime));

  std::cout << "Average Goodput(Packets): "<<(averageGoodput*1e6/1000) <<std::endl;
   

  Simulator::Destroy ();

  return 0;
}
