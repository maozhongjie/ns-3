/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
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

// 
// This script configures two nodes on an 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000 
// (application) bytes to the other node.  The physical layer is configured
// to receive at a fixed RSS (regardless of the distance and transmit
// power); therefore, changing position of the nodes has no effect. 
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc --help"
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when rss drops below -97 dBm.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --rss=-97 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-98 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-99 --numPackets=20"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
// 
// ./waf --run "wifi-simple-adhoc --verbose=1"
//
// When you are done, you will notice two pcap trace files in your directory.
// If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-0-0.pcap -nn -tt
//

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-module.h"
#include "ns3/flow-monitor-module.h"
//#include "ns3/ysxroute-module.h"

#include <iostream>

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <vector>
#include <string>
#define random(a,b) (rand()%(b-a+1)+a)
#define node_number 30
#define hop_number 8
#define tdma_msg_slot 1 //每个节点信息子帧的时间
#define netform_finish_time 1+(32*node_number+3)*hop_number+(node_number+3)*hop_number*2+node_number+3+    2000
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhoc");

typedef enum  {tod=1,ri,broadcast,tdma_msg,rts,cts,ack,null} message_type;
struct msg {
	int send_freq;
	message_type msg_type;
	int hop_info ;
	bool hvy_flag;
	int state_info_ ;
	int time_count_ ;
	int time_info_ ;
	double time_MilliSeconds;
	double time_Seconds;
	bool late_netform_flag_;
	Ipv4Address source_ipv4Address;
	int source_id;
	//
	int online_node_list[node_number]={0};
	int dst_id;
	Ipv4Address dst_ipv4Address;
	int src_id;
	Ipv4Address src_ipv4Address;
	double start_time;
	double end_time;
	int priority;
};

struct {
	int send_freq;
	message_type msg_type;
	int hop_info ;
	bool hvy_flag;
	int state_info_ ;
	int time_count_ ;
	int time_info_ ;
	double time_MilliSeconds;
	double time_Seconds;
	bool late_netform_flag_;
	Ipv4Address source_ipv4Address;
	int source_id;
	int online_node_list[node_number]={0};
	int dst_id;
	Ipv4Address dst_ipv4Address;
	int src_id;
	Ipv4Address src_ipv4Address;
	double start_time;
	double end_time;
	int priority;
} frame_send_block[node_number];

struct {
	int send_freq;
	message_type msg_type;
	int hop_info ;
	bool hvy_flag;
	int state_info_ ;
	int time_count_ ;
	int time_info_ ;
	double time_MilliSeconds;
	double time_Seconds;
	bool late_netform_flag_;
	Ipv4Address source_ipv4Address;
	int source_id;
	int online_node_list[node_number]={0};
	int dst_id;
	Ipv4Address dst_ipv4Address;
	int src_id;
	Ipv4Address src_ipv4Address;
	double start_time;
	double end_time;
	int priority;
} frame_receive_block[node_number];

typedef struct {
	int send_freq;
	message_type msg_type;
	int hop_info ;
	bool hvy_flag;
	int state_info_ ;
	int time_count_ ;
	int time_info_ ;
	double time_MilliSeconds;
	double time_Seconds;
	bool late_netform_flag_;
	Ipv4Address source_ipv4Address;
	int source_id;
	int online_node_list[node_number]={0};
	int dst_id;
	Ipv4Address dst_ipv4Address;
	int src_id;
	Ipv4Address src_ipv4Address;
	double start_time;
	double end_time;
	int priority;
} DT;

typedef struct {

    DT data[3000];
    int front,rear;
} SEQUEUE;
SEQUEUE TDMA_QUEUE[node_number];


TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
Ptr<Socket> socket[node_number]={};
InetSocketAddress remote = InetSocketAddress (Ipv4Address ("10.1.1.255"), 80);
int topo[node_number][node_number]={{0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 1,0,0,0,0,0,0,0,0,0},//1
									{1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//2
									{0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//3
									{0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//4
									{0,0,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//5
									{0,0,0,0,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//6
									{0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//7
									{0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//8
									{0,0,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//9
									{0,0,0,0,0,0,0,0,1,0,0,1,1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//10
									{0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//11
									{0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,1,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//12
									{0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,1, 0,0,0,0,0,0,0,0,0,0},//13
									{0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0, 0,0,0,0,0,0,0,0,0,0},//14
									{0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//15
									{0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//16
									{0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//17
									{0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//18
									{0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//19
									{0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0},//20
									{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,1,0,0,0,0,0,0,0},//21
									{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 1,0,0,1,0,0,0,0,0,0},//22
									{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 1,0,0,1,0,0,0,0,0,0},//23
									{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,1,0,1,0,0,0,0,0},//24
									{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,1,0,1,1,1,0,0},//25
									{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,1,0,0,0,0,0},//26
									{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,1,0,0,0,0,0},//27
									{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,1,0,0,0,1,1},//28
									{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,0,0},//29
									{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,0,0},//30
									};//测试时修改
int node_slot_priority[node_number][node_number]={{19, 4, 8, 14, 6, 18, 12, 10, 2, 15, 1, 7, 9, 11, 0, 3, 5, 13, 16, 17},//node 0
		{17, 19, 4, 8, 14, 6, 18, 12, 10, 2, 15, 1, 7, 9, 11, 0, 3, 5, 13, 16},//1
		{16, 17, 19, 4, 8, 14, 6, 18, 12, 10, 2, 15, 1, 7, 9, 11, 0, 3, 5, 13},//2
		{13, 16, 17, 19, 4, 8, 14, 6, 18, 12, 10, 2, 15, 1, 7, 9, 11, 0, 3, 5},//3
		{5, 13, 16, 17, 19, 4, 8, 14, 6, 18, 12, 10, 2, 15, 1, 7, 9, 11, 0, 3},//4
		{3, 5, 13, 16, 17, 19, 4, 8, 14, 6, 18, 12, 10, 2, 15, 1, 7, 9, 11, 0},//5
		{0, 3, 5, 13, 16, 17, 19, 4, 8, 14, 6, 18, 12, 10, 2, 15, 1, 7, 9, 11},//6
		{11, 0, 3, 5, 13, 16, 17, 19, 4, 8, 14, 6, 18, 12, 10, 2, 15, 1, 7, 9},//7
		{9, 11, 0, 3, 5, 13, 16, 17, 19, 4, 8, 14, 6, 18, 12, 10, 2, 15, 1, 7},
		{7, 9, 11, 0, 3, 5, 13, 16, 17, 19, 4, 8, 14, 6, 18, 12, 10, 2, 15, 1},
		{1, 7, 9, 11, 0, 3, 5, 13, 16, 17, 19, 4, 8, 14, 6, 18, 12, 10, 2, 15},
		{15, 1, 7, 9, 11, 0, 3, 5, 13, 16, 17, 19, 4, 8, 14, 6, 18, 12, 10, 2},
		{2, 15, 1, 7, 9, 11, 0, 3, 5, 13, 16, 17, 19, 4, 8, 14, 6, 18, 12, 10},
		{10, 2, 15, 1, 7, 9, 11, 0, 3, 5, 13, 16, 17, 19, 4, 8, 14, 6, 18, 12},
		{12, 10, 2, 15, 1, 7, 9, 11, 0, 3, 5, 13, 16, 17, 19, 4, 8, 14, 6, 18},
		{18, 12, 10, 2, 15, 1, 7, 9, 11, 0, 3, 5, 13, 16, 17, 19, 4, 8, 14, 6},
		{6, 18, 12, 10, 2, 15, 1, 7, 9, 11, 0, 3, 5, 13, 16, 17, 19, 4, 8, 14},
		{14, 6, 18, 12, 10, 2, 15, 1, 7, 9, 11, 0, 3, 5, 13, 16, 17, 19, 4, 8},
		{8, 14, 6, 18, 12, 10, 2, 15, 1, 7, 9, 11, 0, 3, 5, 13, 16, 17, 19, 4},
		{4, 8, 14, 6, 18, 12, 10, 2, 15, 1, 7, 9, 11, 0, 3, 5, 13, 16, 17, 19}};//测试时修改

int node_hop_info[node_number]={0};
int node_hop_send_ri[hop_number+1]={0};
int node_send_ri[node_number]={0};
int node_recv_tod[node_number]={0};
double node_last_recv_tod_time[node_number]={0};
//vector<double> node_last_recv_tod_time(node_number,-1);
int node_send_tod[node_number]={0};
int node_recv_broadcast[node_number]={1};
int node_send_broadcast[node_number]={0};
int node_load[node_number]={0};
int tdma_slot=0;
int random_add_tdma_data=0;

int netform_state=0;
double local_time[node_number]={0};
int freq;

//time delay
int packet_num=0;
int packet_num_1=0;
int packet_num_2=0;
double ttl_delay_1=0;
double ttl_delay_2=0;
double avg_delay_1=0;
double avg_delay_2=0;


int online_node[node_number][node_number];
int node_rts_recv[node_number][node_number];

SEQUEUE initQueue();
SEQUEUE inQueue(SEQUEUE Q,DT x);
SEQUEUE insertQueue(SEQUEUE Q,DT x);
SEQUEUE outQueue(SEQUEUE Q);
int Queuelength(SEQUEUE Q);
void random_add_data();


void timer ();
void listener(Ipv4Address center,int center_id);
void clean_frame_receive_block(int i);
void clean_frame_send_block(int i);
int get_sourceid(Ipv4Address ipv4Address);
static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval, uint8_t *buf_ptr );
void Send_cts();
void Send_rts();
void Send_tdma_msg();
int TDMA_get_heaviest_node();
void TDMA (int a);
void netform_frame_receive();
void netform_state_update ();
void Netform_stage_1 (Ipv4Address center,int center_id);
void Netform_stage_2 (int count);
void Netform_stage_3 (int count);
void Netform_stage_4 (int count);
void Netform_finished();
void Recv_tod (Ptr<Socket> selfsocket,msg *test);
void Recv_ri (Ptr<Socket> selfsocket,msg *test);
void Recv_broadcast (Ptr<Socket> selfsocket,msg *test);
void Recv_tdma_msg (Ptr<Socket> selfsocket,msg *test);
void Recv_ack (Ptr<Socket> selfsocket,msg *test);
void Recv_rts (Ptr<Socket> selfsocket,msg *test);
void Recv_cts (Ptr<Socket> selfsocket,msg *test);
void ReceivePacket (Ptr<Socket> selfsocket);
void random_start_time();
bool netform_stage3_ri_send_judge(int node,int hop,int node_hop_send[],int node_send_ri[]);
bool CheckAllNodes(NodeContainer nd);
void stage5check(NodeContainer nd);
int main (int argc, char *argv[])
{

  std::string phyMode ("DsssRate1Mbps");
  double rss = -96;  // -dBm
  bool verbose = false;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
//  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
//  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
//  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);

  cmd.Parse (argc, argv);
  // Convert to time object


  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer c;
  c.Create (node_number);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
//  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength.
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));//1
  positionAlloc->Add (Vector (1300.0, 0.0, 0.0));//2
  positionAlloc->Add (Vector (2500.0, 1200.0, 0.0));//3
  positionAlloc->Add (Vector (2500.0, -1200.0, 0.0));//4
  positionAlloc->Add (Vector (3700.0, 0.0, 0.0));//5
  positionAlloc->Add (Vector (4900.0, 0.0, 0.0));//6
  positionAlloc->Add (Vector (4900.0, 1900.0, 0.0));//7
  positionAlloc->Add (Vector (4900.0, -1900.0, 0.0));//8
  positionAlloc->Add (Vector (6400.0, 0.0, 0.0));//9
  positionAlloc->Add (Vector (8000.0, 0.0, 0.0));//10
  positionAlloc->Add (Vector (6400.0, 0.0, 1400.0));//11
  positionAlloc->Add (Vector (9500.0, 0.0, 0.0));//12
  positionAlloc->Add (Vector (8000.0, 1500.0, 0.0));//13
  positionAlloc->Add (Vector (6400.0, 0.0, 3000.0));//14
  positionAlloc->Add (Vector (11000.0, 0.0, 0.0));//15
  positionAlloc->Add (Vector (9500.0, 1500.0, 0.0));//16
  positionAlloc->Add (Vector (6400.0, 0.0, 4500.0));//17
  positionAlloc->Add (Vector (8000.0, 3000.0, 0.0));//18
  positionAlloc->Add (Vector (8000.0, 1500.0, 1500.0));//19
  positionAlloc->Add (Vector (8000.0, 1500.0, -1500.0));//20
  positionAlloc->Add (Vector (-1300.0, 0.0, 0.0));//21
  positionAlloc->Add (Vector (-2500.0, 1200.0, 0.0));//22
  positionAlloc->Add (Vector (-2500.0, -1200.0, 0.0));//23
  positionAlloc->Add (Vector (-3700.0, 0.0, 0.0));//24
  positionAlloc->Add (Vector (-4900.0, 0.0, 0.0));//25
  positionAlloc->Add (Vector (-4900.0, 1900.0, 0.0));//26
  positionAlloc->Add (Vector (-4900.0, -1900.0, 0.0));//27
  positionAlloc->Add (Vector (-6400.0, 0.0, 0.0));//28
  positionAlloc->Add (Vector (-8000.0, 0.0, 0.0));//29
  positionAlloc->Add (Vector (-6400.0, 0.0, 1400.0));//30
  //根据拓扑图确定节点位置，测试时做修改
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);


	OlsrHelper olsr;
    Ipv4StaticRoutingHelper staticRouting;

    Ipv4ListRoutingHelper list;
    list.Add (staticRouting, 0);
    list.Add (olsr, 10);

    InternetStackHelper internet;
    internet.SetRoutingHelper (list); // has effect on the next Install ()
    internet.Install (c);
  // Tracing
  wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);
  //Ipv4Address::GetAny ()


  //255.255.255.255 is a broadcast address;

  for (int i=0;i<node_number;i++)
  {
	  socket[i]=Socket::CreateSocket (c.Get (i), tid);
	  InetSocketAddress local = InetSocketAddress (c.Get (i)->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal(), 80);
	  socket[i]->Bind (local);
	  socket[i]->Connect (remote);
	  socket[i]->SetAllowBroadcast (true);
	  socket[i]->SetRecvCallback (MakeCallback (&ReceivePacket));
	  TDMA_QUEUE[i]=initQueue();
  }

//  DT x;
//  x.msg_type=tdma_msg;
//  x.dst_id=5;
//  x.dst_ipv4Address=socket[x.dst_id-1]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
//  x.src_id=1;
//  x.src_ipv4Address=socket[x.src_id-1]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
//  x.start_time=local_time[0]+200.0;
//  x.priority=2;
//
//  TDMA_QUEUE[0]=inQueue(TDMA_QUEUE[0],x);

  Ipv4Address center=c.Get (0)->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();

  int center_id = get_sourceid(center);
  memset(online_node,0,sizeof(online_node));
  memset(node_rts_recv,0,sizeof(node_rts_recv));
  random_start_time();
  node_recv_tod[0]=1;//簇头发起tod
  //timer();
  listener(center,center_id);
  Simulator::Schedule (Seconds (8500), &stage5check,c);


  //netform finish at 170ms,so TDMA Start_time >170, here set it to 200
  //TDMA(250);
//  Simulator::Stop(Seconds(300));
//  Simulator::Run ();
  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  flowmon = flowmonHelper.InstallAll ();


  NS_LOG_INFO ("Run Simulation.");

  //CheckThroughput ();

  Simulator::Stop (Seconds (netform_finish_time));
  Simulator::Run ();

//  flowmon->SerializeToXmlFile ((tr_name + ".flowmon").c_str(), false, false);
  flowmon->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats ();
  std::string flowtrace="ysx flowtrace";
  std::string flod="flow overload" ;

  std::ofstream flowy("output1.txt");
//  flowy.open(flowtrace);//,std::ios::app);
  std::ofstream flowovld("output2.txt");
//  flowovld.open(flod);//,std::ios::app);
  //flowy<<"Protocol:"<<m_protocol<<std::endl;
  //flowovld<<"Protocol:"<<m_protocol<<std::endl;
  Time sumdelay;
  uint32_t sumpkttx=0;
  uint32_t sumpktrx=0;
  double sumthrput=0.0;
 // uint64_t overload=0;


  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
    {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);


    	 flowy<<"Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress<<std::endl;
    	      	  flowy<<"Tx Packets = " << iter->second.txPackets<<std::endl;
    	      	  sumpkttx+=iter->second.txPackets;
    	      	  flowy<<"Rx Packets = " << iter->second.rxPackets<<std::endl;
    	      	sumpktrx+=iter->second.rxPackets;
    	      	  flowy<<"Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()
    	      			  -iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps"<<std::endl;
    	      	  sumthrput+=iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()
    	      			  -iter->second.timeFirstTxPacket.GetSeconds()) / 1024 ;
    	      	 flowy<<"Delay Expectation"<<iter->second.delaySum/(iter->second.rxPackets*1000000)<<std::endl;
    	      	 sumdelay+=iter->second.delaySum/1000000;
    }







  flowy<<"PDR: "<<double(sumpktrx)/double(sumpkttx)<<std::endl;
  flowy<<"rx: "<<sumpktrx<<std::endl;
  flowy<<"Delay Expectation: "<<sumdelay/sumpktrx<<std::endl;
  flowy<<"Throughput: "<<sumthrput<<std::endl;

  flowy.close();
  flowovld.close();

  Simulator::Destroy ();

  return 0;
}

SEQUEUE initQueue(){
    SEQUEUE Q;
    //1.初始化队列,队头指针=队尾指针=0
    Q.front=Q.rear=0;
    return Q;
}

SEQUEUE inQueue(SEQUEUE Q,DT x){
    //1.判断队列是上溢,就是队尾指针是否等于最大申请的空间
    if(Q.rear==1000){
    	NS_LOG_UNCOND("Up Overflow\n");
    }else{
         //2.从队尾插入结点
        Q.rear++;
        Q.data[Q.rear]=x;
//        NS_LOG_UNCOND("in  success");
    }
   return Q;
}

SEQUEUE insertQueue(SEQUEUE Q,DT x){
    //1.判断队列是上溢,就是队尾指针是否等于最大申请的空间
    if(Q.rear==1000){
    	NS_LOG_UNCOND("Up Overflow\n");
    }else{
         //2.从队尾插入结点
        Q.rear++;
        for (int i=0;i<Q.rear-Q.front-1;i++)
        {
        	Q.data[Q.rear-i]=Q.data[Q.rear-i-1];
        }

        Q.data[Q.front+1]=x;

    }
   return Q;
}

SEQUEUE outQueue(SEQUEUE Q){
    //1.首先判断是否是空队列
    if(Q.front==Q.rear){
    	NS_LOG_UNCOND("queue is empty");
    }else{
        //2.删除结点是从队头删除
        Q.front++;
//        NS_LOG_UNCOND("out success");
    }
    return Q;
}

int Queuelength(SEQUEUE Q){
	int length=Q.rear-Q.front;
	return length;
}

void random_add_data() {

	srand((unsigned)time(NULL));
//	int data_num=random(0,10);

	double tdma_time=1.4*node_number;
	double data_time=tdma_time-rand()/double(RAND_MAX);
	while (data_time>0)
	{
//当前情况为1号往5号节点发消息
		int data_priority=random(1,100);
		DT x;
		x.msg_type=tdma_msg;
		x.dst_id=random(1,30);
		x.dst_ipv4Address=socket[x.dst_id-1]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
		x.src_id=(random(1,30)+x.dst_id)%30;
		x.src_ipv4Address=socket[x.src_id-1]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
		x.start_time=local_time[0]-data_time;
		data_time=data_time-0.2*rand()/double(RAND_MAX)-0.3;
		if (data_priority>50)
		{
			x.priority=1;
			TDMA_QUEUE[0]=inQueue(TDMA_QUEUE[0],x);
		}
		else
		{
			x.priority=2;
			TDMA_QUEUE[0]=inQueue(TDMA_QUEUE[0],x);
			//TDMA_QUEUE[0]=insertQueue(TDMA_QUEUE[0],x);
		}

	}

//	if (data_num==0)
//	{
//		NS_LOG_UNCOND("No new data inqueue");
//	}
//	else
//	{
//		for (int i=0;i<data_num;i++)
//		{
//
//			double data_time=time*rand()/double(RAND_MAX);
//			int data_priority=random(1,100);
//			DT x;
//			x.msg_type=tdma_msg;
//			x.dst_id=5;
//			x.dst_ipv4Address=socket[x.dst_id-1]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
//			x.src_id=1;
//			x.src_ipv4Address=socket[x.src_id-1]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
//			x.start_time=local_time[0]-data_time;
//			if (data_priority>20)
//			{
//				x.priority=1;
//				TDMA_QUEUE[0]=inQueue(TDMA_QUEUE[0],x);
//			}
//			else
//			{
//				x.priority=2;
//				//TDMA_QUEUE[0]=inQueue(TDMA_QUEUE[0],x);
//				TDMA_QUEUE[0]=insertQueue(TDMA_QUEUE[0],x);
//			}
//
//
//			time=data_time;
//		}
//	}

}

void Netform_stage_1 (Ipv4Address center,int center_id)
{

	uint32_t packetSize = 3000; // bytes
	uint32_t numPackets = 1;
	double interval = 1.0; // seconds
	Time interPacketInterval = Seconds (interval);

	for (int i=0;i<4;i++)
	{
		for (int j=0;j<4;j++)
		{
			uint8_t *buffer=new uint8_t[packetSize];
			msg *test=reinterpret_cast<msg *>(buffer);
			test->send_freq=j;
			test->msg_type=tod;
			test->hop_info=1;
			test->state_info_=1;
			test->time_count_=1;
			test->time_info_=2;
			test->time_MilliSeconds= Simulator::Now ().GetMilliSeconds ();
			test->time_Seconds= Simulator::Now ().GetSeconds ();
			test->late_netform_flag_=false;
			test->source_ipv4Address=center;
			test->source_id=center_id;
			uint8_t *buf_ptr=reinterpret_cast<uint8_t *>(test);
			Simulator::Schedule (Seconds (i*8+2*j), &GenerateTraffic,
												socket[0], packetSize, numPackets, interPacketInterval,buf_ptr);

			Simulator::Schedule (Seconds (i*8+2*j+0.9), &netform_frame_receive);
			//every freq send 2 tod
			Simulator::Schedule (Seconds (i*8+2*j+1), &GenerateTraffic,
															socket[0], packetSize, numPackets, interPacketInterval,buf_ptr);

			Simulator::Schedule (Seconds (i*8+2*j+1+0.9), &netform_frame_receive);
		}
	}


	node_send_tod[0]=1;
}

void Netform_stage_2 (int count)
{

	// "count" shows the num of nodes which have not send tod
	if (count>0)
	{
		for (int l=0;l<node_number;l++)
		{
			if (node_recv_tod[l]==1 && node_send_tod[l]==0 && node_hop_info[l]==8-count+1)
			{
				uint32_t packetSize = 3000; // bytes
				uint32_t numPackets = 1;
				double interval = 1.0; // seconds
				Time interPacketInterval = Seconds (interval);

				for (int i=0;i<4;i++)
					{
						for (int j=0;j<4;j++)
						{
							uint8_t *buffer=new uint8_t[packetSize];
							msg *test=reinterpret_cast<msg *>(buffer);
							test->send_freq=j;
							test->msg_type=frame_send_block[l].msg_type;
							test->hop_info=frame_send_block[l].hop_info;
							test->state_info_=frame_send_block[l].state_info_;
							test->time_count_=frame_send_block[l].time_count_;
							test->time_info_=frame_send_block[l].time_info_;
							test->time_MilliSeconds= Simulator::Now ().GetMilliSeconds ();
							test->time_Seconds= Simulator::Now ().GetSeconds ();
							test->late_netform_flag_=frame_send_block[l].late_netform_flag_;
							test->source_ipv4Address=socket[l]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
							test->source_id=frame_send_block[l].source_id;
							uint8_t *buf_ptr=reinterpret_cast<uint8_t *>(test);



							Simulator::Schedule (Seconds (32*l+i*8+2*j), &GenerateTraffic,
																socket[l], packetSize, numPackets, interPacketInterval,buf_ptr);

							Simulator::Schedule (Seconds (32*l+i*8+2*j+0.9), &netform_frame_receive);
							//every freq send 2 tod
							Simulator::Schedule (Seconds (32*l+i*8+2*j+1), &GenerateTraffic,
																			socket[l], packetSize, numPackets, interPacketInterval,buf_ptr);

							Simulator::Schedule (Seconds (32*l+i*8+2*j+1+0.9), &netform_frame_receive);
						}
					}
				clean_frame_send_block(l);

				node_send_tod[l]=1;
				//count--;
				//break;
			}
		}
		count--;
		Simulator::Schedule (Seconds (32*node_number+3), &Netform_stage_2,count);

	}
}

void Netform_stage_3 (int count)
{
	//initialize the array
	//
	if (count>0)
	{
		for (int i=0;i<node_number;i++)
		{
			if (node_hop_info[i]==count && node_send_ri[i]==0)
			{
				//send ri
				//update the info
				node_send_ri[i]=1;
				online_node[i][i]=1;
				//create a new message
				uint32_t packetSize = 3000; // bytes
				uint32_t numPackets = 1;
				double interval = 1.0; // seconds
				Time interPacketInterval = Seconds (interval);
				uint8_t *buffer=new uint8_t[packetSize];
				msg *test=reinterpret_cast<msg *>(buffer);
				test->msg_type=ri;
				test->hop_info=count;
				test->source_ipv4Address=frame_send_block[i].source_ipv4Address;
				test->source_id=frame_send_block[i].source_id;
				for (int k=0;k<node_number;k++)
				{
					test->online_node_list[k]=online_node[i][k];
				}

				uint8_t *buf_ptr=reinterpret_cast<uint8_t *>(test);
				clean_frame_send_block(i);
				Simulator::Schedule (Seconds (i), &GenerateTraffic,
													socket[i], packetSize, numPackets, interPacketInterval,buf_ptr);

				Simulator::Schedule (Seconds (i+0.9), &netform_frame_receive);
			}
		}

		count--;
		Simulator::Schedule (Seconds (node_number+3), &Netform_stage_3,count);
	}

	//
//	if (count>0)
//	{
//		count--;
//		Simulator::Schedule (Seconds (1), &Netform_stage_3,count);
//		for (int i=0;i<hop_number+1;i++)
//		{
//			node_hop_send_ri[i]=1;
//		}
//		// scan the array to refresh the "node_hop_send_ri"
//		for (int i=0;i<node_number;i++)
//		{
//			for(int u=1;u<hop_number+1;u++)
//			{
//			if (node_hop_info[i]==u && node_send_ri[i]==0)
//			{
//				node_hop_send_ri[u]=0;
//			}
////			if (node_hop_info[i]==2 && node_send_ri[i]==0)
////			{
////				node_hop_send_ri[2]=0;
////			}
////			if (node_hop_info[i]==1 && node_send_ri[i]==0)
////			{
////				node_hop_send_ri[1]=0;
////			}
//			}
//		}
//
//		//bool netform_stage3_ri_send_judge(int node,int hop,int node_hop_send[],int node_send_ri[])
//		for (int i=0;i<node_number;i++)
//		{
//			for(int j=hop_number;j>0;j--)
//			{
//				if (node_hop_info[i]==j)
//								{
//									if (netform_stage3_ri_send_judge(i,j,node_hop_send_ri,node_send_ri))
//									{
//										//send ri
//										//update the info
//										node_send_ri[i]=1;
//
//										online_node[i][i]=1;
//
//										//create a new message
//										uint32_t packetSize = 3000; // bytes
//										uint32_t numPackets = 1;
//										double interval = 1.0; // seconds
//										Time interPacketInterval = Seconds (interval);
//										uint8_t *buffer=new uint8_t[packetSize];
//										msg *test=reinterpret_cast<msg *>(buffer);
//										test->msg_type=ri;
//										test->hop_info=j;
//										test->source_ipv4Address=frame_send_block[i].source_ipv4Address;
//										test->source_id=frame_send_block[i].source_id;
//										for (int k=0;k<node_number;k++)
//										{
//											test->online_node_list[k]=online_node[i][k];
//										}
//
//										uint8_t *buf_ptr=reinterpret_cast<uint8_t *>(test);
//										clean_frame_send_block(i);
//										Simulator::Schedule (Seconds (0), &GenerateTraffic,
//																			socket[i], packetSize, numPackets, interPacketInterval,buf_ptr);
//
//										Simulator::Schedule (Seconds (0.9), &netform_frame_receive);
//
//										return;
//									}
//								}
//			}
//
//
//				bool center_flag=true;
//				for (int p=hop_number;p>0;p--)
//				{
//					if (node_hop_send_ri[p]!=1)
//					{
//						center_flag=false;
//						break;
//					}
//				}
//				if(node_hop_info[i]!=0)
//				{
//					center_flag=false;
//				}
//				if (center_flag)
//				{
//					//update the info
//					node_send_ri[i]=1;
//
//					clean_frame_send_block(i);
//
//					return;
//				}
//
//		}
//	}
}


void Netform_stage_4 (int count)
{

	// "count" shows the num of nodes which have not send tod
	if (count>-1)
	{
		for (int i=0;i<node_number;i++)
		{
			if (node_recv_broadcast[i]==1 && node_send_broadcast[i]==0 && node_hop_info[i]==8-count)
			{
				uint32_t packetSize = 3000; // bytes
				uint32_t numPackets = 1;
				double interval = 1.0; // seconds
				Time interPacketInterval = Seconds (interval);
				uint8_t *buffer=new uint8_t[packetSize];
				msg *test=reinterpret_cast<msg *>(buffer);
				test->send_freq=((int)local_time[i])%4;
				test->msg_type=broadcast;
				test->hop_info=node_hop_info[i]+1;
				test->state_info_=4;

				test->time_MilliSeconds= Simulator::Now ().GetMilliSeconds ();
				test->time_Seconds= Simulator::Now ().GetSeconds ();
				test->late_netform_flag_=frame_send_block[i].late_netform_flag_;
				test->source_ipv4Address=frame_send_block[i].source_ipv4Address;
				test->source_id=frame_send_block[i].source_id;
				for (int j=0;j<node_number;j++)
				{
					test->online_node_list[j]=online_node[i][j];
				}

				uint8_t *buf_ptr=reinterpret_cast<uint8_t *>(test);

				clean_frame_send_block(i);

				Simulator::Schedule (Seconds (i), &GenerateTraffic,
												socket[i], packetSize, numPackets, interPacketInterval,buf_ptr);

				Simulator::Schedule (Seconds (i+0.005 ), &netform_frame_receive);

				node_send_broadcast[i]=1;
//				count--;
//				break;
			}
		}
		count--;
		Simulator::Schedule (Seconds (node_number+3), &Netform_stage_4,count);
	}
}

void Netform_finished()
{
	int time=local_time[0];
	NS_LOG_UNCOND (time<<"ms,Netform finished! print online_node[30][30]");//根据测试修改
    for (int i=0;i<node_number;i++)
    {
    	NS_LOG_UNCOND (i+1<<"["<<online_node[i][0]<<online_node[i][1]<<online_node[i][2]<<online_node[i][3]<<online_node[i][4]<<online_node[i][5]<<online_node[i][6]<<online_node[i][7]<<online_node[i][8]<<online_node[i][9]<<online_node[i][10]<<online_node[i][11]<<online_node[i][12]<<online_node[i][13]<<online_node[i][14]<<online_node[i][15]<<online_node[i][16]<<online_node[i][17]<<online_node[i][18]<<online_node[i][19]<<online_node[i][20]<<online_node[i][21]<<online_node[i][22]<<online_node[i][23]<<online_node[i][24]<<online_node[i][25]<<online_node[i][26]<<online_node[i][27]<<online_node[i][28]<<online_node[i][29]<<"]");
    }
}

void listener (Ipv4Address center,int center_id)
{
	int l =(int) local_time[0];

	if (local_time[0]<netform_finish_time)
	{
		Simulator::Schedule(Seconds(0.5),&listener,center,center_id);
		netform_state_update ();

	}

	if (l==1 && netform_state==0)
	{
		netform_state=1;
		NS_LOG_UNCOND ("Netform begin, print local_time[]");
		for (int i=0;i<node_number;i++)
		{
			NS_LOG_UNCOND ("local_time["<<i<<"]="<<local_time[i]);
		}
		//netform_stage_1
		NS_LOG_UNCOND ("*****Netform_stage1 begin*****");
		Simulator::Schedule(Seconds(0),&Netform_stage_1,center,center_id);
		return;
	}
	if (l==33+3 && netform_state==1)
	{
		netform_state=2;
		NS_LOG_UNCOND ("*****Netform_stage2 begin*****");
		Simulator::Schedule(Seconds(0),&Netform_stage_2,hop_number);
		return;
	}
	if (l==1+(32*node_number+3)*hop_number && netform_state==2)
	{
		netform_state=3;
		Simulator::Schedule(Seconds(0),&Netform_stage_3,hop_number);
//		NS_LOG_UNCOND ("Netform_stage_2 end, print local_time[]");
//		for (int i=0;i<node_number;i++)
//		{
//			NS_LOG_UNCOND ("local_time["<<i<<"]="<<local_time[i]);
//		}
		NS_LOG_UNCOND ("*****Netform_stage3 begin*****");
		return;
	}
	if (l==1+(32*node_number+3)*hop_number+(node_number+3)*hop_number && netform_state==3)
	{
		netform_state=4;
		NS_LOG_UNCOND ("*****Netform_stage4 begin*****");
		Simulator::Schedule(Seconds(0),&Netform_stage_4,hop_number);
		return;
	}
	if (l==1+(32*node_number+3)*hop_number+(node_number+3)*hop_number*2+node_number+3 && netform_state==4)
	{
		netform_state=5;
		Simulator::Schedule(Seconds(0),&Netform_finished);
		return;
	}



}

void netform_state_update ()
{
	for (int i=0;i<node_number;i++)
	{
	local_time[i]=local_time[i]+0.5;
	}
	if (local_time[0]>=netform_finish_time)
	{
		NS_LOG_UNCOND ("power shut down at"<<local_time[0]<< "ms");
	}
}

void timer ()
{
	if (local_time[0]<200)
	{
	Simulator::Schedule(Seconds(0.05),&timer);
	}
	else {
		NS_LOG_UNCOND ("power shut down at"<<local_time[0]<< "ms");
	}
	for (int i=0;i<5;i++)
	{
	local_time[i]=local_time[i]+0.05;
	}
}

void netform_frame_receive()
{
	for (int i=0;i<node_number;i++)
	{
		switch (frame_receive_block[i].msg_type)
		{
			case null:
			{
			}
			break;
			case tod:
			{
				frame_send_block[i].msg_type=frame_receive_block[i].msg_type;
				frame_send_block[i].hop_info=frame_receive_block[i].hop_info+1;
				frame_send_block[i].hvy_flag=frame_receive_block[i].hvy_flag;
				frame_send_block[i].state_info_=frame_receive_block[i].state_info_;
				frame_send_block[i].time_count_=frame_receive_block[i].time_count_;
				frame_send_block[i].time_info_=frame_receive_block[i].time_info_;
				frame_send_block[i].time_MilliSeconds=frame_receive_block[i].time_MilliSeconds;
				frame_send_block[i].time_Seconds=frame_receive_block[i].time_Seconds;
				frame_send_block[i].late_netform_flag_=frame_receive_block[i].late_netform_flag_;
				frame_send_block[i].source_ipv4Address=socket[i]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
				frame_send_block[i].source_id=get_sourceid(frame_send_block[i].source_ipv4Address);
				for (int j=0;j<node_number;j++)
				{
					frame_send_block[i].online_node_list[j]=frame_receive_block[i].online_node_list[j];
				}

				clean_frame_receive_block(i);
			}
			break;
			case ri:
			{
				frame_send_block[i].msg_type=frame_receive_block[i].msg_type;
				frame_send_block[i].source_ipv4Address=socket[i]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
				frame_send_block[i].source_id=get_sourceid(frame_send_block[i].source_ipv4Address);
				for (int j=0;j<node_number;j++)
				{
					frame_send_block[i].online_node_list[j]=frame_receive_block[i].online_node_list[j];
				}

				clean_frame_receive_block(i);
			}
			break;
			case broadcast:
			{
				frame_send_block[i].msg_type=frame_receive_block[i].msg_type;
				frame_send_block[i].source_ipv4Address=socket[i]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
				frame_send_block[i].source_id=get_sourceid(frame_send_block[i].source_ipv4Address);
				for (int j=0;j<node_number;j++)
				{
					frame_send_block[i].online_node_list[j]=frame_receive_block[i].online_node_list[j];
				}

				clean_frame_receive_block(i);
			}
			break;
		}
	}
}

void clean_frame_receive_block(int i)
{
	if (i>-1 && i<node_number)
	{
		frame_receive_block[i].send_freq=-1;
		frame_receive_block[i].msg_type=null;
		frame_receive_block[i].hop_info=-1;
		frame_receive_block[i].hvy_flag=false;
		frame_receive_block[i].state_info_=-1;
		frame_receive_block[i].time_count_=-1;
		frame_receive_block[i].time_info_=-1;
		frame_receive_block[i].time_MilliSeconds=-1;
		frame_receive_block[i].time_Seconds=-1;
		frame_receive_block[i].late_netform_flag_=false;
		frame_receive_block[i].source_ipv4Address=socket[i]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
		frame_receive_block[i].source_id=get_sourceid(frame_receive_block[i].source_ipv4Address);
		for (int j=0;j<node_number;j++)
		{
			frame_receive_block[i].online_node_list[j]=0;
		}
	}
}

void clean_frame_send_block(int i)
{
	if (i>-1 && i<node_number)
	{
		frame_send_block[i].send_freq=-1;
		frame_send_block[i].msg_type=null;
		frame_send_block[i].hop_info=-1;
		frame_send_block[i].hvy_flag=false;
		frame_send_block[i].state_info_=-1;
		frame_send_block[i].time_count_=-1;
		frame_send_block[i].time_info_=-1;
		frame_send_block[i].time_MilliSeconds=-1;
		frame_send_block[i].time_Seconds=-1;
		frame_send_block[i].late_netform_flag_=false;
		frame_send_block[i].source_ipv4Address=socket[i]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
		frame_send_block[i].source_id=get_sourceid(frame_receive_block[i].source_ipv4Address);
		for (int j=0;j<node_number;j++)
		{
			frame_send_block[i].online_node_list[j]=0;
		}
	}
}

int get_sourceid(Ipv4Address ipv4Address)
{
	uint8_t buf[4]={0};
	ipv4Address.Serialize(buf);
	int id = (int) buf[3];
	return id;
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval, uint8_t *buf_ptr )
{
  if (pktCount > 0)
    {
	  Ptr<Packet> pkt = Create<Packet> (buf_ptr,pktSize);

	  socket->Send (pkt);
	  Ipv4Address ipv4Address=socket->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
	  int id = get_sourceid(ipv4Address);
	  double Seconds = Simulator::Now ().GetSeconds ();
	  //after 700 ms is  the TDMA trainmission, type "double" of data is needed.
//	  if (Seconds<netform_finish_time)
//	  {
//		  NS_LOG_UNCOND (int(Seconds)<<"ms, Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<"  is sending");
//	  }
//	  else
//	  {
//		  NS_LOG_UNCOND (Seconds<<"ms, Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<"  is sending");
//	  }
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount-1, pktInterval,buf_ptr);
    }
  else
    {
      //socket->Close ();
    }
}

void Send_cts()
{
	NS_LOG_UNCOND ("*****Sending CTS*****");
	bool rts_recv_flag[node_number]={false};
	for (int i=0;i<node_number;i++)
	{
		for (int j=0;j<node_number;j++)
		{
			if (node_rts_recv[i][j]!=0)
			{
				rts_recv_flag[i]=true;
				break;
			}
		}

		if (rts_recv_flag[i])
		{
			rts_recv_flag[i]=false;
			uint32_t packetSize = 3000; // bytes
			uint32_t numPackets = 1;
			double interval = 0.02*i; // seconds
			Time interPacketInterval = Seconds (interval);
			uint8_t *buffer=new uint8_t[packetSize];
			msg *tmp=reinterpret_cast<msg *>(buffer);
			for (int q=0;q<node_number;q++)
			{
				tmp->online_node_list[q]=node_rts_recv[i][q];
			}
			tmp->msg_type=cts;
			tmp->hop_info=node_hop_info[i];
			tmp->hvy_flag=0;
			tmp->state_info_=5;
			tmp->time_count_=0;
			tmp->time_info_=0;
			tmp->time_MilliSeconds=Simulator::Now ().GetMilliSeconds ();
			tmp->time_Seconds=Simulator::Now ().GetSeconds () ;
//			tmp->late_netform_flag_=false;
			tmp->source_ipv4Address=socket[i]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
			tmp->source_id=i+1;
			uint8_t *buf=reinterpret_cast<uint8_t *>(tmp);

			socket[i]->Connect (remote);
			socket[i]->SetAllowBroadcast (true);
			Simulator::Schedule (Seconds(0.2*i), &GenerateTraffic,
											  socket[i], packetSize, numPackets, interPacketInterval,buf);
		}
	}
}

void Send_rts()
{
	for (int i=0;i<node_number;i++)
	{
		node_load[i]=Queuelength(TDMA_QUEUE[i]);
	}
	NS_LOG_UNCOND ("*****A TDMA slot begin*****");
	NS_LOG_UNCOND ("*****print node_load[20]*****");//测试时修改
	NS_LOG_UNCOND ("*****["<<node_load[0]<<" "<<node_load[1]<<" "<<node_load[2]<<" "<<node_load[3]<<" "<<node_load[4]<<node_load[5]<<node_load[6]<<node_load[7]<<node_load[8]<<node_load[9]<<node_load[10]<<node_load[11]<<node_load[12]<<node_load[13]<<node_load[14]<<node_load[15]<<node_load[16]<<node_load[17]<<node_load[18]<<node_load[19]<<"]*****");
	NS_LOG_UNCOND ("*****Sending RTS*****");
	for (int i=0;i<node_number;i++)
	{
		if (node_load[i]>0)
		{
			uint32_t packetSize = 3000; // bytes
			uint32_t numPackets = 1;
			double interval = 0.02*i; // seconds
			Time interPacketInterval = Seconds (interval);
			uint8_t *buffer=new uint8_t[packetSize];
			msg *tmp=reinterpret_cast<msg *>(buffer);
			tmp->msg_type=rts;
			tmp->hop_info=node_hop_info[i];
			if (node_load[i]>5)
			{
				tmp->hvy_flag=1;
			}
			else tmp->hvy_flag=0;
			tmp->state_info_=5;
			tmp->time_count_=0;
			tmp->time_info_=0;
			tmp->time_MilliSeconds=Simulator::Now ().GetMilliSeconds ();
			tmp->time_Seconds=Simulator::Now ().GetSeconds () ;
//			tmp->late_netform_flag_=false;
			tmp->source_ipv4Address=socket[i]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
			tmp->source_id=i+1;
			uint8_t *buf=reinterpret_cast<uint8_t *>(tmp);

			socket[i]->Connect (remote);
			socket[i]->SetAllowBroadcast (true);
			Simulator::Schedule (Seconds(0.2*i), &GenerateTraffic,
											  socket[i], packetSize, numPackets, interPacketInterval,buf);
		}
	}
}

void Send_tdma_msg()
{
	NS_LOG_UNCOND ("*****Sending tdma_msg*****");


	for (int i=0;i<node_number;i++)
	{


		if (node_load[i]>0)
		{
			int num=node_load[i];
			if (num>5)
			{
				num=5;
			}
			node_load[i]=node_load[i]-num;
			uint32_t packetSize = 3000; // bytes
			uint32_t numPackets = 1;
			double interval = 0.2; // no use
			Time interPacketInterval = Seconds (interval);

			for (int j=0;j<num;j++)
			{
				TDMA_QUEUE[i]=outQueue(TDMA_QUEUE[i]);

				uint8_t *buffer=new uint8_t[packetSize];
				msg *tmp=reinterpret_cast<msg *>(buffer);
				tmp->msg_type=TDMA_QUEUE[i].data[TDMA_QUEUE[i].front].msg_type;
				tmp->hop_info=node_hop_info[i];
				tmp->hvy_flag=0;
				tmp->state_info_=5;
				tmp->time_count_=0;
				tmp->time_info_=0;

	//			tmp->late_netform_flag_=false;
				tmp->source_ipv4Address=socket[i]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
				tmp->source_id=i+1;
				tmp->dst_id=TDMA_QUEUE[i].data[TDMA_QUEUE[i].front].dst_id;
				tmp->dst_ipv4Address=TDMA_QUEUE[i].data[TDMA_QUEUE[i].front].dst_ipv4Address;
				tmp->src_id=TDMA_QUEUE[i].data[TDMA_QUEUE[i].front].src_id;
				tmp->src_ipv4Address=TDMA_QUEUE[i].data[TDMA_QUEUE[i].front].src_ipv4Address;
				tmp->start_time=TDMA_QUEUE[i].data[TDMA_QUEUE[i].front].start_time;
				tmp->priority=TDMA_QUEUE[i].data[TDMA_QUEUE[i].front].priority;
				uint8_t *buf=reinterpret_cast<uint8_t *>(tmp);

				Ipv4Address m_ipv4=socket[i]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
				Ipv4Address next_addr=socket[i]->GetNode()->GetObject<ns3::olsr::RoutingProtocol>()->YRouteOutput(m_ipv4,tmp->dst_ipv4Address);
				//NS_LOG_UNCOND(next_addr);
				InetSocketAddress addr = InetSocketAddress (next_addr, 80);
				socket[i]->Connect(addr);
				socket[i]->SetAllowBroadcast (false);

				Simulator::Schedule (Seconds(i*tdma_msg_slot+0.2*j), &GenerateTraffic,
												  socket[i], packetSize, numPackets, interPacketInterval,buf);
			}
//			if (i==0)
//			{
//				Ipv4Address next_addr=socket[4]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
//				InetSocketAddress addr = InetSocketAddress (next_addr, 80);
//				socket[i]->Connect(addr);
//				socket[i]->SetAllowBroadcast (false);
//
//				Simulator::Schedule (Seconds(i), &GenerateTraffic,
//												  socket[i], packetSize, numPackets, interPacketInterval,buf);
//			}
//			if (i==4)
//			{
//				Ipv4Address next_addr=socket[2]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
//				InetSocketAddress addr = InetSocketAddress (next_addr, 80);
//				socket[i]->Connect(addr);
//				socket[i]->SetAllowBroadcast (false);
//
//				Simulator::Schedule (Seconds(i), &GenerateTraffic,
//												  socket[i], packetSize, numPackets, interPacketInterval,buf);
//			}
//			if (i==2)
//			{
//				Ipv4Address next_addr=socket[1]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
//				InetSocketAddress addr = InetSocketAddress (next_addr, 80);
//				socket[i]->Connect(addr);
//				socket[i]->SetAllowBroadcast (false);
//
//				Simulator::Schedule (Seconds(i), &GenerateTraffic,
//												  socket[i], packetSize, numPackets, interPacketInterval,buf);
//			}
//			if (i==1)
//			{
//				Ipv4Address next_addr=socket[0]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
//				InetSocketAddress addr = InetSocketAddress (next_addr, 80);
//				socket[i]->Connect(addr);
//				socket[i]->SetAllowBroadcast (false);
//
//				Simulator::Schedule (Seconds(i), &GenerateTraffic,
//												  socket[i], packetSize, numPackets, interPacketInterval,buf);
//			}
		} else {
			if (node_load[i]==0)
			{
				int m=-1,n=-1;
				for (int j=0;j<node_number;j++)
				{
					if (node_rts_recv[i][j]==2 && node_load[j]>0)
					{
						if (node_slot_priority[i][j]>n)
						{
							m=j;
							n=node_slot_priority[i][j];

						}
					}
				}

				if (m>-1)
				{
					int num=node_load[m];
					if (num>5)
					{
						num=5;
					}
					node_load[m]=node_load[m]-num;
					//NS_LOG_UNCOND ("node_load["<<m<<"]="<<node_load[m]);
					uint32_t packetSize = 3000; // bytes
					uint32_t numPackets = 1;
					double interval = 0.2; // no use
					Time interPacketInterval = Seconds (interval);

					for (int j=0;j<num;j++)
					{
						TDMA_QUEUE[m]=outQueue(TDMA_QUEUE[m]);

						uint8_t *buffer=new uint8_t[packetSize];
						msg *tmp=reinterpret_cast<msg *>(buffer);
						tmp->msg_type=TDMA_QUEUE[m].data[TDMA_QUEUE[m].front].msg_type;
						tmp->hop_info=node_hop_info[m];
						tmp->hvy_flag=0;
						tmp->state_info_=5;
						tmp->time_count_=0;
						tmp->time_info_=0;

			//			tmp->late_netform_flag_=false;
						tmp->source_ipv4Address=socket[m]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
						tmp->source_id=m+1;
						tmp->dst_id=TDMA_QUEUE[m].data[TDMA_QUEUE[m].front].dst_id;
						tmp->dst_ipv4Address=TDMA_QUEUE[m].data[TDMA_QUEUE[m].front].dst_ipv4Address;
						tmp->src_id=TDMA_QUEUE[m].data[TDMA_QUEUE[m].front].src_id;
						tmp->src_ipv4Address=TDMA_QUEUE[m].data[TDMA_QUEUE[m].front].src_ipv4Address;
						tmp->start_time=TDMA_QUEUE[m].data[TDMA_QUEUE[m].front].start_time;
						tmp->priority=TDMA_QUEUE[m].data[TDMA_QUEUE[m].front].priority;
						uint8_t *buf=reinterpret_cast<uint8_t *>(tmp);

						Ipv4Address m_ipv4=socket[m]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
						Ipv4Address next_addr=socket[m]->GetNode()->GetObject<ns3::olsr::RoutingProtocol>()->YRouteOutput(m_ipv4,tmp->dst_ipv4Address);
						//NS_LOG_UNCOND(next_addr);
						InetSocketAddress addr = InetSocketAddress (next_addr, 80);
						socket[m]->Connect(addr);
						socket[m]->SetAllowBroadcast (false);

						Simulator::Schedule (Seconds(i*tdma_msg_slot+0.2*j), &GenerateTraffic,
														  socket[m], packetSize, numPackets, interPacketInterval,buf);
					}

//					uint8_t *buffer=new uint8_t[packetSize];
//					msg *tmp=reinterpret_cast<msg *>(buffer);
//					tmp->msg_type=tdma_msg;
//					tmp->hop_info=node_hop_info[m];
//					tmp->hvy_flag=0;
//					tmp->state_info_=5;
//					tmp->time_count_=0;
//					tmp->time_info_=0;
//					tmp->time_MilliSeconds=Simulator::Now ().GetMilliSeconds ();
//					tmp->time_Seconds=Simulator::Now ().GetSeconds () ;
//		//			tmp->late_netform_flag_=false;
//					tmp->source_ipv4Address=socket[m]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
//					tmp->source_id=m+1;
//					uint8_t *buf=reinterpret_cast<uint8_t *>(tmp);
//
//					Ipv4Address next_addr=socket[4]->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
//					InetSocketAddress addr = InetSocketAddress (next_addr, 80);
//					socket[m]->Connect(addr);
//					socket[m]->SetAllowBroadcast (false);
//					Simulator::Schedule (Seconds(i), &GenerateTraffic,
//													  socket[m], packetSize, numPackets, interPacketInterval,buf);
//					//m=-1;
				}
			}
		}
	}

	for (int p=0;p<node_number;p++)
	{
		for (int q=0;q<node_number;q++)
		{
			node_rts_recv[p][q]=0;
			//NS_LOG_UNCOND (node_rts_recv[p][q]);
		}
	}
	Simulator::Schedule (Seconds(node_number*tdma_msg_slot), &TDMA,0);
}

//no use
int TDMA_get_heaviest_node()
{
	int a=node_load[0];
	int b=0;
	for (int i=1;i<5;i++)
	{
		if (node_load[i]>a)
		{
			a=node_load[i];
			b=i;
		}
	}
	return b;
}

void TDMA (int a)
{
	bool flag=false;

	//add a function that can add packet to Queue
	if (random_add_tdma_data<5)
	{
		random_add_data();
		random_add_tdma_data++;
	}
	//
	for (int i=0;i<node_number;i++)
	{
		if (Queuelength(TDMA_QUEUE[i])>0 && tdma_slot <50)
		{
			flag=true;
			break;
		}
	}
	if (flag)
	{
		Simulator::Schedule(Seconds(a),&Send_rts);
		Simulator::Schedule(Seconds(a+0.2*node_number),&Send_cts);
		Simulator::Schedule(Seconds(a+0.4*node_number),&Send_tdma_msg);
		tdma_slot++;
		flag=false;
	}
	else {
		for (int i=0;i<node_number;i++)
		{
			socket[i]->Close();
		}
		avg_delay_1=ttl_delay_1/packet_num_1;
		avg_delay_2=ttl_delay_2/packet_num_2;
		packet_num=packet_num_1+packet_num_2;
		//NS_LOG_UNCOND("packet_num="<<packet_num);
		NS_LOG_UNCOND("packet_num_1="<<packet_num_1<<",packet_num_2="<<packet_num_2);
		NS_LOG_UNCOND("ttl_delay_1="<<ttl_delay_1<<",avg_delay_1="<<avg_delay_1);
		NS_LOG_UNCOND("ttl_delay_2="<<ttl_delay_2<<",avg_delay_2="<<avg_delay_2);
	}

}


void Recv_tod (Ptr<Socket> selfsocket,msg *test)
{
	Ipv4Address ipv4Address=selfsocket->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
	int id = get_sourceid(ipv4Address);
	int recv_freq=(((int)local_time[id-1])/8)%4;
	  if (topo[test->source_id-1][id-1]==1 && node_recv_tod[id-1]==0 && recv_freq==test->send_freq)
	  {
		  if ((local_time[id-1]-node_last_recv_tod_time[id-1])<2.0)
		  {

			  NS_LOG_UNCOND ("hop="<<test->hop_info);
			  NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving tod from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
			  node_recv_tod[id-1]=1;
			  node_hop_info[id-1]=test->hop_info;
			  //
			  frame_receive_block[id-1].msg_type=test->msg_type;
			  frame_receive_block[id-1].hop_info=test->hop_info;
			  frame_receive_block[id-1].hvy_flag=test->hvy_flag;
			  frame_receive_block[id-1].state_info_=test->state_info_;
			  frame_receive_block[id-1].time_count_=test->time_count_;
			  frame_receive_block[id-1].time_info_=test->time_info_;
			  frame_receive_block[id-1].time_MilliSeconds=test->time_MilliSeconds;
			  frame_receive_block[id-1].time_Seconds=test->time_Seconds;
			  frame_receive_block[id-1].late_netform_flag_=test->late_netform_flag_;
			  frame_receive_block[id-1].source_ipv4Address=test->source_ipv4Address;
			  frame_receive_block[id-1].source_id=test->source_id;
			  for (int j=0;j<node_number;j++)
			  {
				frame_receive_block[id-1].online_node_list[j]=test->online_node_list[j];
			  }
			  //update the message
			  local_time[id-1]=local_time[0];

			  return;


//			  if (test->hop_info==1)
//			  {
//				  NS_LOG_UNCOND ("hop=1");
//				  NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving tod from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
//
//				  node_recv_tod[id-1]=1;
//				  node_hop_info[id-1]=test->hop_info;
//				  //
//				  frame_receive_block[id-1].msg_type=test->msg_type;
//				  frame_receive_block[id-1].hop_info=test->hop_info;
//				  frame_receive_block[id-1].hvy_flag=test->hvy_flag;
//				  frame_receive_block[id-1].state_info_=test->state_info_;
//				  frame_receive_block[id-1].time_count_=test->time_count_;
//				  frame_receive_block[id-1].time_info_=test->time_info_;
//				  frame_receive_block[id-1].time_MilliSeconds=test->time_MilliSeconds;
//				  frame_receive_block[id-1].time_Seconds=test->time_Seconds;
//				  frame_receive_block[id-1].late_netform_flag_=test->late_netform_flag_;
//				  frame_receive_block[id-1].source_ipv4Address=test->source_ipv4Address;
//				  frame_receive_block[id-1].source_id=test->source_id;
//				  for (int j=0;j<5;j++)
//				  {
//					frame_receive_block[id-1].an[j]=test->an[j];
//				  }
//				  //update the message
//				  local_time[id-1]=local_time[0];
//
//				  return;
//			  }
//			  if (test->hop_info==2)
//			  {
//				  NS_LOG_UNCOND ("hop=2");
//				  NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving tod from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
//				  node_recv_tod[id-1]=1;
//				  node_hop_info[id-1]=test->hop_info;
//				  //
//				  frame_receive_block[id-1].msg_type=test->msg_type;
//				  frame_receive_block[id-1].hop_info=test->hop_info;
//				  frame_receive_block[id-1].hvy_flag=test->hvy_flag;
//				  frame_receive_block[id-1].state_info_=test->state_info_;
//				  frame_receive_block[id-1].time_count_=test->time_count_;
//				  frame_receive_block[id-1].time_info_=test->time_info_;
//				  frame_receive_block[id-1].time_MilliSeconds=test->time_MilliSeconds;
//				  frame_receive_block[id-1].time_Seconds=test->time_Seconds;
//				  frame_receive_block[id-1].late_netform_flag_=test->late_netform_flag_;
//				  frame_receive_block[id-1].source_ipv4Address=test->source_ipv4Address;
//				  frame_receive_block[id-1].source_id=test->source_id;
//				  for (int j=0;j<5;j++)
//				  {
//					frame_receive_block[id-1].an[j]=test->an[j];
//				  }
//				  //update the message
//				  local_time[id-1]=local_time[0];
//
//				  return;
//			  }
//			  if (test->hop_info==3)
//			  {
//				  NS_LOG_UNCOND ("hop=3");
//				  NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving tod from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
//
//				  node_recv_tod[id-1]=1;
//				  node_hop_info[id-1]=test->hop_info;
//				  //
//				  frame_receive_block[id-1].msg_type=test->msg_type;
//				  frame_receive_block[id-1].hop_info=test->hop_info;
//				  frame_receive_block[id-1].hvy_flag=test->hvy_flag;
//				  frame_receive_block[id-1].state_info_=test->state_info_;
//				  frame_receive_block[id-1].time_count_=test->time_count_;
//				  frame_receive_block[id-1].time_info_=test->time_info_;
//				  frame_receive_block[id-1].time_MilliSeconds=test->time_MilliSeconds;
//				  frame_receive_block[id-1].time_Seconds=test->time_Seconds;
//				  frame_receive_block[id-1].late_netform_flag_=test->late_netform_flag_;
//				  frame_receive_block[id-1].source_ipv4Address=test->source_ipv4Address;
//				  frame_receive_block[id-1].source_id=test->source_id;
//				  for (int j=0;j<5;j++)
//				  {
//					frame_receive_block[id-1].an[j]=test->an[j];
//				  }
//				  //update the message
//				  local_time[id-1]=local_time[0];
//
//				  return;
//			  }
		  }
		  else {
			  node_last_recv_tod_time[id-1]=local_time[id-1];
		  }
	  }
}

void Recv_ri (Ptr<Socket> selfsocket,msg *test)
{
	Ipv4Address ipv4Address=selfsocket->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
	int id = get_sourceid(ipv4Address);
	if (topo[test->source_id-1][id-1]==1 && test->hop_info==node_hop_info[id-1]+1)
	{
		NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving ri from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
		frame_receive_block[id-1].msg_type=test->msg_type;
		frame_receive_block[id-1].source_ipv4Address=test->source_ipv4Address;
		frame_receive_block[id-1].source_id=test->source_id;
		for (int j=0;j<node_number;j++)
		{
			frame_receive_block[id-1].online_node_list[j]=test->online_node_list[j];
		}


		online_node[id-1][id-1]=1;
	    for (int i=0;i<node_number;i++)
	    {
	    	online_node[id-1][i]= online_node[id-1][i] || test->online_node_list[i];
	    }

	    NS_LOG_UNCOND ("Print online_node[30][30]");//测试时修改
	    for (int i=0;i<node_number;i++)
	    {
	    	NS_LOG_UNCOND (i+1<<"["<<online_node[i][0]<<online_node[i][1]<<online_node[i][2]<<online_node[i][3]<<online_node[i][4]<<online_node[i][5]<<online_node[i][6]<<online_node[i][7]<<online_node[i][8]<<online_node[i][9]<<online_node[i][10]<<online_node[i][11]<<online_node[i][12]<<online_node[i][13]<<online_node[i][14]<<online_node[i][15]<<online_node[i][16]<<online_node[i][17]<<online_node[i][18]<<online_node[i][19]<<online_node[i][20]<<online_node[i][21]<<online_node[i][22]<<online_node[i][23]<<online_node[i][24]<<online_node[i][25]<<online_node[i][26]<<online_node[i][27]<<online_node[i][28]<<online_node[i][29]<<"]");
	    }
	    return;

	}

}

void Recv_broadcast (Ptr<Socket> selfsocket,msg *test)
{
	Ipv4Address ipv4Address=selfsocket->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
	int id = get_sourceid(ipv4Address);

    if (topo[test->source_id-1][id-1]==1 && node_recv_broadcast[id-1]==0)
    {

    	for (int j=1;j<hop_number+1;j++)
	    {
	    	if (test->hop_info==j)
			{
				for(int i=0;i<node_number;i++)
						{
							online_node[(test->source_id)-1][i]= online_node[id-1][i]=1 || online_node[(test->source_id)-1][i];
						}
				node_recv_broadcast[id-1]=1;
				NS_LOG_UNCOND ("hop="<<j);
				NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving broadcast from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);

			  return;
			}
	    }

//    	if (test->hop_info==1)
//	    {
//	    	for(int i=0;i<node_number;i++)
//	    		    {
//	    		    	online_node[(test->source_id)-1][i]= online_node[id-1][i]=1 || online_node[(test->source_id)-1][i];
//	    		    }
//	    	node_recv_broadcast[id-1]=1;
//	    	NS_LOG_UNCOND ("Netform_stage4 begin");
//	    	NS_LOG_UNCOND ("hop=1");
//	    	NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving broadcast from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
//
//		  return;
//	    }
//
//	    if (test->hop_info==2)
//		{
//			for(int i=0;i<node_number;i++)
//					{
//						online_node[(test->source_id)-1][i]= online_node[id-1][i]=1 || online_node[(test->source_id)-1][i];
//					}
//			node_recv_broadcast[id-1]=1;
//			NS_LOG_UNCOND ("hop=2");
//			NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving broadcast from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
//
//		  return;
//		}
//
//	    if (test->hop_info==3)
//		{
//			for(int i=0;i<node_number;i++)
//					{
//						online_node[(test->source_id)-1][i]= online_node[id-1][i]=1 || online_node[(test->source_id)-1][i];
//					}
//			node_recv_broadcast[id-1]=1;
//			NS_LOG_UNCOND ("hop=3");
//			NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving broadcast from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
//
//
//		  return;
//		}

    }
}

void Recv_tdma_msg (Ptr<Socket> selfsocket,msg *test)
{
	Ipv4Address ipv4Address=selfsocket->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
	int id = get_sourceid(ipv4Address);
//	if (topo[test->source_id-1][id-1]==1)
//	{
//		NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving tdma_msg from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
//	}

	NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving tdma_msg from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
	if (id!=test->dst_id)
	{
		DT x;
		x.msg_type=test->msg_type;
		x.dst_id=test->dst_id;
		x.dst_ipv4Address=test->dst_ipv4Address;
		x.src_id=test->src_id;
		x.src_ipv4Address=test->src_ipv4Address;
		x.start_time=test->start_time;
		x.priority=test->priority;
		//NS_LOG_UNCOND("priority="<<x.priority);
		TDMA_QUEUE[id-1]=inQueue(TDMA_QUEUE[id-1],x);
	}
	if (id==test->dst_id)
	{
		test->end_time=local_time[0];
		//NS_LOG_UNCOND("priority="<<test->priority);
		if (test->priority==1)
		{
			packet_num_1++;
			ttl_delay_1=ttl_delay_1+test->end_time-test->start_time;
		}
		if (test->priority==2)
		{
			packet_num_2++;
			ttl_delay_2=ttl_delay_2+test->end_time-test->start_time;
		}

		//add ack
		DT x;
		x.msg_type=ack;
		x.dst_id=test->src_id;
		x.dst_ipv4Address=test->src_ipv4Address;
		x.src_id=test->dst_id;
		x.src_ipv4Address=test->dst_ipv4Address;
		x.priority=3;
		TDMA_QUEUE[id-1]=inQueue(TDMA_QUEUE[id-1],x);
	}
}

void Recv_ack (Ptr<Socket> selfsocket,msg *test)
{
	Ipv4Address ipv4Address=selfsocket->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
	int id = get_sourceid(ipv4Address);


	NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving ack from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
	if (id!=test->dst_id)
	{
		DT x;
		x.msg_type=test->msg_type;
		x.dst_id=test->dst_id;
		x.dst_ipv4Address=test->dst_ipv4Address;
		x.src_id=test->src_id;
		x.src_ipv4Address=test->src_ipv4Address;
		TDMA_QUEUE[id-1]=inQueue(TDMA_QUEUE[id-1],x);

	}
}
void Recv_rts (Ptr<Socket> selfsocket,msg *test)
{
	Ipv4Address ipv4Address=selfsocket->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
	int id = get_sourceid(ipv4Address);
	if (topo[test->source_id-1][id-1]==1)
	{
		if (test->hvy_flag==1)
		{
			node_rts_recv[id-1][test->source_id-1]=2;
		}
		else node_rts_recv[id-1][test->source_id-1]=1;
		NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving rts from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);
	}
}

void Recv_cts (Ptr<Socket> selfsocket,msg *test)
{
	Ipv4Address ipv4Address=selfsocket->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
	int id = get_sourceid(ipv4Address);
	if (topo[test->source_id-1][id-1]==1)
	{
		for (int i=0;i<node_number;i++)
		{
			if (node_rts_recv[id-1][i]==0)
			{
				node_rts_recv[id-1][i]=test->online_node_list[i];
			}

		}
		NS_LOG_UNCOND ("Node_id:"<<id<<"  Node_ipv4address:"<<ipv4Address<<" is receiving cts from "<<"source_id:"<<test->source_id<<"  source_ipv4Address:"<<test->source_ipv4Address);

	}
}

void ReceivePacket (Ptr<Socket> selfsocket)
{
	Ptr<Packet> packet = selfsocket->Recv();
	//Ipv4Address ipv4Address=selfsocket->GetNode()->GetObject<ns3::Ipv4>()->GetAddress(1,0).GetLocal();
	//int id = get_sourceid(ipv4Address);
	//
	if (packet)
    {
		  uint8_t *buffer_ptr=new uint8_t[packet->GetSize()];
		  int len;
		  len = packet->CopyData(buffer_ptr, packet->GetSize());
		  msg *test=reinterpret_cast<msg *>(buffer_ptr);
		  //NS_LOG_UNCOND (id<<" msg_type:"<<test->msg_type);
		  if (test->msg_type==tod)
		  {
			  Recv_tod(selfsocket,test);
		  }
		  if (test->msg_type==ri)
		  {
			  Recv_ri(selfsocket,test);
		  }
		  if (test->msg_type==broadcast)
		  {
			  Recv_broadcast(selfsocket,test);
		  }
		  if (test->msg_type==tdma_msg)
		  {
			  Recv_tdma_msg(selfsocket,test);
		  }
		  if (test->msg_type==ack)
		  {
			  Recv_ack(selfsocket,test);
		  }
		  if (test->msg_type==rts)
		  {
			  Recv_rts(selfsocket,test);
		  }
		  if (test->msg_type==cts)
		  {
			  Recv_cts(selfsocket,test);
		  }
    }
}
void random_start_time()
{
	srand((unsigned)time(NULL));
	for(int i=1;i<node_number;i++)
	{
		local_time[i]=rand()/double(RAND_MAX)*10;
	}
	local_time[0]=0;
}
bool netform_stage3_ri_send_judge(int node,int hop,int node_hop_send[],int node_send_ri[])
{
	bool flag=true;
	for(int i=hop_number;i>hop;i--)
	{
		if(node_hop_send[i]==1)
			continue;
		else
		{
			flag=false;
			return flag;
		}
	}
	if(node_hop_send[hop]!=0)
	{
		flag=false;
		return flag;
	}
	if(node_send_ri[node]!=0)
	{
		flag=false;
		return flag;
	}
	return flag;
}


bool CheckAllNodes(NodeContainer nd)
{
	for (int i=0;i<nd.GetN();i++)
	{
		int nodesize=nd.Get(i)->GetObject<ns3::olsr::RoutingProtocol>()->CheckTableSize();
		NS_LOG_UNCOND(i<<" rt table size"<<nodesize);
		if(nodesize<node_number-1)
		{
			NS_LOG_UNCOND(Simulator::Now()<<"Not yet");
			return false;
		}
	}
	return true;
}
void stage5check(NodeContainer nd)
{
	bool temp=CheckAllNodes(nd);
	if(temp)
	{
		NS_LOG_UNCOND("route table initialized");
		NS_LOG_UNCOND(local_time[0]);
		return ;
	}
	else
	{
		NS_LOG_UNCOND(local_time[0]<<"ms,route table initialing");
		Simulator::Schedule (Seconds(10),&stage5check,nd);
		return ;
	}
}
