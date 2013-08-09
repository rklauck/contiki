Chatty Things - XMPP 4 IoT
==========================

This code is based on the rudimentary prototype Contiki-XMPP 0.1 - beta
version - 20-08-2009, which was written by Eloi Bail & Adrian Hornsby
(Tampere University of Technology - Multimedia Group).

The code was heavily restructed and improved to work more memory-efficient
while allowing to integrate more XEPs for Smart Objects. Added Features:
- IPv6 support
- Message flow optimizations
- Reduced definition of functions
- Extended API
- Partial MUC support

More information can be found in the paper:
Klauck, R.; Kirsche, M.: Chatty Things - Making the Internet of Things Readily
Usable for the Masses with XMPP, in Proceedings of the 8th International
Conference on Collaborative Computing: Networking, Applications and Worksharing
(CollaborateCom 2012), Pittsburgh, Pennsylvania, USA, October, 2012.

It is to note that this code only implements a XMPP client. An external XMPP server
is needed to couple Chatty Things with each other and computational devices.
Configurations can be found in the subfolder external-conf for prosody.im.

The second example (xmpp-extended) automatically sends data values to a chat room.
Therefore, the XMPP client joins the room 'temp' and reads the values via
"dev/temperature-sensor.h" if available for the platform (i.e., Z1), else a
random value is generated and pushed every 15 seconds.
Furthermore, the intervall of the send data can be controlled via 'remote commands':
- int++: increment the current value by 10 seconds
- int--: decrement the current value by 10 seconds
The 'remote commands' 

All experiments were tested with the prosody XMPP server and the Pidgin client.


Setup for prosody & Pidgin
--------------------------
1) create the accounts: '123@localhost' and 'node_xmpp@localhost' with pw: 'paul'
   on the XMPP server (Pidgin can be used for that)
2) login with Pidgin into both accounts and subscribe to each other (this
   will ensure that presence stanzas can be received from Chatty Things)
3) disable the 'node_xmpp' account (just make sure your Pidgin client is not
   logged in with that account anymore)
4) join the chatrooms 'room' & 'temp' with the account '123@localhost'


SIM setup for COOJA
-------------------
Since prosody 0.9 IPv6 via LuaSocket 3.0rc1 is supported.
Packages can be found here: http://prosody.im/doc/ipv6
# apt-get install prosody-0.9 lua-sec-prosody lua-socket-prosody
from the http://packages.prosody.im/debian/ repository

prosody tunslip6 <-> cooja (rpl <-> uxmpp)
#define COOJA_SIM               1


if prosody 0.8 is used:
prosody <-> 6tunnel <-> tunslip6 <-> cooja (rpl <-> uxmpp)
#define COOJA_SIM               1
#define CONF_6TUNNEL            1


Running the simulation
----------------------

STEP 0
if prosody <= 0.8.2 is used, start 6tunnel: 
6tunnel -46 5223 localhost 5222
test with: telnet ::1 5223

STEP 1
JUST LOAD simulation with rpl and uxmpp4ipv6: make TARGET=cooja xmpp-ipv6-rpl-*.csc
uxmpp:
server ip= ::1

STEP 2
make connect-router-cooja or
contiki/tools$ sudo ./tunslip6 -a 127.0.0.1 -p 60001 aaaa::1/64

STEP 4
START prosody (if not already running)

STEP 5
START SIM: hit start button

STEP 6
STOP SIM: kill prosody (it will send packets to all clients
 -> THIS can cause strange 'off LINK' errors for new SIM)


IMPORTANT:
when restarting simualtion, make sure to restart prosody, because old session for IPs musst be deleted,
else nodes will kicked with "unavailable" and no login accures!!!
