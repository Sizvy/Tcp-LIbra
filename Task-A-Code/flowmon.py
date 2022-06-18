from xml.etree import ElementTree as ET 
import sys
et=ET.parse(sys.argv[1])

total_received_bits = 0
total_received_packets = 0
total_transmitted_packets = 0
total_delay = 0.0
simulation_start = 0.0
simulation_stop = 0.0
sumT = 0.0
sumTsq = 0.0
flowCount = 0
total_lost_packets = 0

for flow in et.findall("FlowStats/Flow"):
    
    flowCount+=1
    total_received_bits += int(flow.get('rxBytes'))
    total_received_packets += int(flow.get('rxPackets'))
    total_transmitted_packets += int(flow.get('txPackets'))
    total_delay += float(flow.get('delaySum')[:-2])
    tx_start = float(flow.get('timeFirstRxPacket')[:-2])
    tx_stop = float(flow.get('timeLastRxPacket')[:-2])
    total_lost_packets+=int(flow.get('lostPackets'))
    

    if simulation_start == 0.0:
       simulation_start = tx_start

    elif tx_start < simulation_start:
        simulation_start = tx_start

    if tx_stop > simulation_stop:
        simulation_stop = tx_stop

print("\nThrougput Calculation : ")

print("Total Received bits = "+str(total_received_bits*8))
print("Receiving Packet Start time = %.3f sec" % (simulation_start*1e-9,))
print("Receiving Packet Stop time = %.3f sec" % (simulation_stop*1e-9,))

duration = (simulation_stop-simulation_start)*1e-9
print("Receiving Packet Duration time = %.3f sec" % (duration,))
print("Network Throughput = %.7f Kbps" % (total_received_bits*8/(duration*1e3),))


print("\nRatio Calculation : ")

print("Total Transmitted Packets = "+str(total_transmitted_packets))
print("Total Received Packets = "+str(total_received_packets))

print("Packet Delivery Ratio = %.7f " % (total_received_packets*1.0/total_transmitted_packets,))
print("Packet Drop Ratio = %.7f " % (total_lost_packets*1.0/total_transmitted_packets,))

print("Network Mean Delay = %.7f ms" % (total_delay*1e-6/total_received_packets,))




