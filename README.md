# iPRP
IP Parallel Redudancy Protocol. This project is based on the protocol described in the following paper.

`Popovic, Miroslav, Maaz Mohiuddin, Dan-Cristian Tomozei, and Jean-Yves Le Boudec. "iPRP—The parallel redundancy protocol for IP networks: Protocol design and operation." IEEE Transactions on Industrial Informatics 12, no. 5 (2016): 1842-1854.`

Any use of this software must be referenced as mentioned above.

## Compilation

Three versions of the protocol can be compiled:
- `compile.sh`: Unicast, IPv4
- `compile_multicast.sh`: Multicast, IPv4
- `compile_ipv6.sh`: Unicast, IPv6

## Execution

`run.sh n ind1 addr1 ... indn addrn`
- `n`: number of interfaces for the host
- `ind1`, ... `indn`: Network subcloud Discriminator (IND) of each interface (between 0 and 15)
- `addr1`, ..., `addrn`: IP address of each interface

Note that iPRP hosts will only send messages to interfaces having the same IND as one of their interfaces.

Example: `run.sh 2 4 10.0.1.1 10 10.0.2.1`

Important: Reverse path filtering can prevent iPRP from working, make sure that it is disabled on the interfaces you want to use.

## Structure

This iPRP implementation consists of four programs running in collaboration:
- Control daemon (ICD): Establishes and monitors the connections between senders and receivers
- Monitoring daemon (IMD): Monitors the incoming traffic to detect potential iPRP senders
- Sending daemona (ISD): Catch packets sent and duplicate them over the separate network subclouds
- Receiving daemon (IRD): Receives the duplicated packets and forwards the first one to the application

The IMD and IRD are started as soon as a port number is added to the monitored ports file (see below).
One ISD is started for each connection established with a remote host.

## Configuration

The following parameters can be tuned by modifying values in the file `inc/config.h`

### Global values

You can change the ports iPRP uses for control, data and statistics messages. Make sure that these are not used by another process.
You can also change the location of the IRD, IMD and ISD binaries, so iPRP can launch them correctly.
Finally, you can change the location of the monitored ports file.

### Active senders

The active senders are detected by the IMD and transmitted to the ICD for connection establishment via a file. The following timers can be set:
- `ICD_T_CAP`: Frequency at which the ICD polls this file to transmit capability messages to the potential senders
- `IMD_T_AS_CACHE`: Frequency at which the IMD stores its latest data to the file
- `IMD_T_AS_EXP`: Time after which the IMD considers the sender as inactive when not receiving packets

### Peerbases

Peerbases are created by the ICD to give the ISDs information about their respective receivers. The following timers can be set:
- `ISD_T_PB_CACHE`: Frequency at which the ISDs poll their file to update their information on their receiver
- `ICD_T_PB_CACHE`: Frequency at which the ICD stores its latest data to the file
- `ICD_T_PB_EXP`: Time after which the ICD closes the connection when not receiving capability messages

### Receiver links

The IRD internally stores information about the iPRP hosts sending messages to it. The following timers affect it:
- `IRD_T_CLEANUP`: Frequency at which the IRD checks the validity of its data (no data is received during that cleanup)
- `IRD_T_EXP`: Time after which the IRD drops the information when not receiving packets

### Monitored ports

You can set the interval at which the ICD will poll the ports file to update its information by modifying the `ICD_T_PORTS` value.

### Sender interfaces (multicast only)

The sender interfaces structure is used by the ICD to communicate the addresses of the various interfaces of the senders to the IRD, in order to setup source-specific multicast (SSM). The following timers can be set:
- `IRD_T_SI_CACHE`: Frequency at which the IRD poll the file to update its information on the senders
- `ICD_T_SI_CACHE`: Frequency at which the ICD stores its latest data to the file
- `ICD_T_SI_EXP`: Time after which the ICD deletes its information when not receiving capability messages

### Multicast in-flight join capability

When a multicast connection is established, the messages are no longer visible to a host that would like to join the multicast group. To allow such a host to join the group, some messages are sent every few seconds on the regular channel in addition to the iPRP channels, which allows the IMD at the receiver to notice that there is a joinable group. The parameter `ISD_T_ALLOW` can be used to set this interval.

## Diagnostics tools

iPRP comes with five tools to help diagnose connectivity issues.

### `iPRPTest`

This tool is used to check if a session has been established with the given host. If it is the case, it prints the linked local and remote interface addresses. If not, it creates a temporary connection with the remote hosts and sends `num_packets` packets, one every `interval` seconds, and then closes the connection.

Usage: `iprptest remote_addr remote_port num_packets interval`

### `iPRPPing`

This tool tries to ping the remote host on all of its interfaces in round robin fashion. It can also be used to detect the MTU by using the `packet_size` argument.

Usage: `iprpping host count packet_size`

### `iPRPTraceroute`

This tool prints the paths taken by packets to reach all remote interfaces of a given host. This can be useful to detect if the network subclouds are not really disjoint.

Usage: `iprptracert host`

### `iPRPReceiverStats`

This tool prints statistics (number of received messages, number of accepted messages, time of last message seen and number of packets received on the wrong interface) about messages received from a given host.

Usage: `iprprecvstats host port`

### `iPRPSenderStats`

This tool prints the same statistics as `iPRPReceiverStats`, but from a remote host relative to messages received from the local host.

Usage: `iprpsendstats host port`
