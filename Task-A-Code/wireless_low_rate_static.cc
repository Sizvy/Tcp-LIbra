#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-flow-classifier.h"
#include "ns3/flow-monitor-helper.h"
#include <ns3/lr-wpan-error-model.h>
#include "ns3/netanim-module.h"



//
//   2002:d00d::
//               
//  *    *    *    *
//  |    |    |    |    2002:d00e::
// n5   n6   n7   n0 -------N------ n1   n2   n3   n4
//                         csma      |    |    |    |
//                                   *    *    *    *
//                                     2002:d00e::

using namespace ns3;


int main (int argc, char** argv) {
  uint32_t simulationTime = 100;
  int range = 10;
  uint32_t nCsma=1;
  uint32_t Lrwpan_nodes = 4;
  std::string dataRate = "200Kbps"; 
  uint32_t payload = 100;
  TypeId tcpTid;
  std::string tcpVariant = "ns3::TcpNewReno";
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcpVariant, &tcpTid), "TypeId " << tcpVariant << " not found");
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));


  NodeContainer Lw_nodes_left;
  Lw_nodes_left.Create (Lrwpan_nodes);

  NodeContainer Lw_nodes_right;
  Lw_nodes_right.Create (Lrwpan_nodes);

  Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (range));
  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<RangePropagationLossModel> propModel = CreateObject<RangePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);  


  LrWpanHelper lh_left;
  LrWpanHelper lh_right;              

  NetDeviceContainer lr_devices_left = lh_left.Install (Lw_nodes_left);
  lh_left.SetChannel(channel);
  lh_left.AssociateToPan (lr_devices_left, 0);

  
  NetDeviceContainer lr_devices_right = lh_right.Install (Lw_nodes_right);
  lh_right.SetChannel(channel); 
  lh_right.AssociateToPan (lr_devices_right, 0);

  NodeContainer Csma_Nodes;
  Csma_Nodes.Create (nCsma);
  Csma_Nodes.Add (Lw_nodes_left.Get (0));
  Csma_Nodes.Add (Lw_nodes_right.Get (0));

  MobilityHelper mobility;
  
  
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (800),
                                 "DeltaY", DoubleValue (800),
                                 "GridWidth", UintegerValue (100),
                                 "LayoutType", StringValue ("RowFirst"));

  // creating a channel with range propagation loss model 
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");              
                
  mobility.Install (Lw_nodes_left);
  mobility.Install (Lw_nodes_right);


  InternetStackHelper ISv6;
  ISv6.Install (Lw_nodes_left);
  ISv6.Install (Lw_nodes_right);
  ISv6.Install (Csma_Nodes.Get (0));


  SixLowPanHelper sl_left;
  NetDeviceContainer sl_devices_left = sl_left.Install (lr_devices_left);
  // sl_left.SetDeviceAttribute ("DataRate", StringValue (dataRate));

  SixLowPanHelper sl_right;
  NetDeviceContainer sl_devices_right = sl_right.Install (lr_devices_right);
  // sl_right.SetDeviceAttribute ("DataRate", StringValue (dataRate));

  CsmaHelper csmahelper;
  NetDeviceContainer csma_devices = csmahelper.Install (Csma_Nodes);
  csmahelper.SetChannelAttribute ("DataRate", StringValue (dataRate));

  Ipv6AddressHelper ipv6;

  ipv6.SetBase (Ipv6Address ("2002:d00d::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer lr_interfaces_left;
  lr_interfaces_left = ipv6.Assign (sl_devices_left);
  lr_interfaces_left.SetForwarding (0, true);
  lr_interfaces_left.SetDefaultRouteInAllNodes (0);

  ipv6.SetBase (Ipv6Address ("2002:f00d::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer csma_interfaces;
  csma_interfaces = ipv6.Assign (csma_devices);
  csma_interfaces.SetForwarding (1, true);
  csma_interfaces.SetDefaultRouteInAllNodes (1);

  ipv6.SetBase (Ipv6Address ("2002:d00e::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer lr_interfaces_right;
  lr_interfaces_right = ipv6.Assign (sl_devices_right);
  lr_interfaces_right.SetForwarding (2, true);
  lr_interfaces_right.SetDefaultRouteInAllNodes (2);

  for (uint32_t i = 0; i < sl_devices_left.GetN (); i++) {
    Ptr<NetDevice> device = sl_devices_left.Get (i);
    device->SetAttribute ("UseMeshUnder", BooleanValue (true));
    device->SetAttribute ("MeshUnderRadius", UintegerValue (10));
  }

  for (uint32_t i = 0; i < sl_devices_right.GetN (); i++) {
    Ptr<NetDevice> device = sl_devices_right.Get (i);
    device->SetAttribute ("UseMeshUnder", BooleanValue (true));
    device->SetAttribute ("MeshUnderRadius", UintegerValue (10));
  }

  uint32_t ports = 9;

  for( uint32_t i=1; i<Lrwpan_nodes; i++ ) {
    // BulkSendHelper sourceApp ("ns3::TcpSocketFactory",
    //                          Inet6SocketAddress (csma_interfaces.GetAddress (0, 1), 
    //                          ports+i));
    OnOffHelper sourceApp ("ns3::TcpSocketFactory", (Inet6SocketAddress (csma_interfaces.GetAddress (0,1), ports+i)));
    sourceApp.SetAttribute("DataRate", DataRateValue (DataRate (dataRate)));
    sourceApp.SetAttribute ("PacketSize", UintegerValue (payload));
    ApplicationContainer sourceApps = sourceApp.Install (Lw_nodes_left.Get (i));
    sourceApps.Start (Seconds(0));
    sourceApps.Stop (Seconds(simulationTime));

    PacketSinkHelper sinkApp ("ns3::TcpSocketFactory",
    Inet6SocketAddress (Ipv6Address::GetAny (), ports+i));
    sinkApp.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
    ApplicationContainer sinkApps = sinkApp.Install (Csma_Nodes.Get(0));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (simulationTime));
    
    ports++;
  }
  

  for( uint32_t i=1; i<Lrwpan_nodes; i++ ) {
    // BulkSendHelper sourceApp ("ns3::TcpSocketFactory",
    //                           Inet6SocketAddress (lr_interfaces_right.GetAddress (i, 1), 
    //                          ports+i));
    OnOffHelper sourceApp ("ns3::TcpSocketFactory", (Inet6SocketAddress (lr_interfaces_right.GetAddress (i, 1), ports+i)));
    sourceApp.SetAttribute("DataRate", DataRateValue (DataRate (dataRate)));
    sourceApp.SetAttribute ("PacketSize", UintegerValue (payload));
    ApplicationContainer sourceApps = sourceApp.Install (Csma_Nodes.Get (0));
    sourceApps.Start (Seconds(0));
    sourceApps.Stop (Seconds(simulationTime));

    PacketSinkHelper sinkApp ("ns3::TcpSocketFactory",
    Inet6SocketAddress (Ipv6Address::GetAny (), ports+i));
    sinkApp.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
    ApplicationContainer sinkApps = sinkApp.Install (Lw_nodes_right.Get(i));
    sinkApps.Start (Seconds (10.0));
    sinkApps.Stop (Seconds (simulationTime));
    
    ports++;
  }


  FlowMonitorHelper flowHelper;
  flowHelper.InstallAll ();

  Simulator::Stop (Seconds (simulationTime));
  // AnimationInterface anim ("./Task_A/low.xml");

  Simulator::Run ();

  flowHelper.SerializeToXmlFile ("./Task_A/LowRate.flowmonitor", false, false);

  Simulator::Destroy ();

  return 0;
}

