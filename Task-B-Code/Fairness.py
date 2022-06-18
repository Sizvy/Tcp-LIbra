from xml.etree import ElementTree as ET 
import sys
et=ET.parse(sys.argv[1])

total_received_bits = 0
total_received_packets = 0
total_transmitted_packets = 0
total_delay = 0.0
simulation_start = 0.0
simulation_stop = 0.0
sumTBic = 0.0
sumTsqBic = 0.0
sumTLibra = 0.0
sumTsqLibra = 0.0
flowCountBic = 0
flowCountLibra = 0
total_lost_packets = 0
total_flow = 50

for flow in et.findall("FlowStats/Flow"):
    
    total_received_bits += int(flow.get('rxBytes'))
    total_received_packets += int(flow.get('rxPackets'))
    total_transmitted_packets += int(flow.get('txPackets'))
    total_delay += float(flow.get('delaySum')[:-2])
    tx_start = float(flow.get('timeFirstRxPacket')[:-2])
    tx_stop = float(flow.get('timeLastRxPacket')[:-2])
    total_lost_packets+=int(flow.get('lostPackets'))
    flowId = int(flow.get('flowId'))
    

    if simulation_start == 0.0:
       simulation_start = tx_start

    elif tx_start < simulation_start:
        simulation_start = tx_start

    if tx_stop > simulation_stop:
        simulation_stop = tx_stop
        
    time = simulation_stop - simulation_start
    throughput = ((int(flow.get('rxBytes'))*8.0)/time)/1024
    if flowId<=total_flow:
    	if (flowId%2)==0:
    		flowCountLibra+=1
    		sumTLibra += throughput
    		sumTsqLibra += throughput * throughput
    	else:
    		flowCountBic+=1
    		sumTBic += throughput
    		sumTsqBic += throughput * throughput
   	
print("Fairness Calculation : ")
FairnessIndexBic = (sumTBic * sumTBic)/ (flowCountBic * sumTsqBic)
FairnessIndexLibra = (sumTLibra * sumTLibra)/ (flowCountLibra * sumTsqLibra)
print("Jain's Index for BIC: "+str(FairnessIndexBic))
print("Jain's Index for Libra: "+str(FairnessIndexLibra))





