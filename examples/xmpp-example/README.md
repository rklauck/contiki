Chatty Things - XMPP 4 IoT
==========================

This code is based on Contiki-XMPP 0.1 - beta version - 20-08-2009, which was
written by Eloi Bail & Adrian Hornsby (Tampere University of Technology
 - Multimedia Group).

The code was improved to work more memory-efficient while allowing to integrate
more XEPs for Smart Objects. Added Features:
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
Configurations can be found in the subfolder external-conf.


SIM setup for COOJA
-------------------
prosody <-> 6tunnel <-> tunslip6 <-> cooja (rpl <-> uxmpp)
#define COOJA_SIM               1
#define CONF_6TUNNEL            1


Running the simulation
----------------------

STEP1
prosody (0.8.2) does not support IPv6, use 6tunnel instead
6tunnel -46 5223 localhost 5222
test with: telnet ::1 5223

STEP2
JUST LOAD simulation with rpl and uxmpp4ipv6: make TARGET=cooja xmpp-ipv6-rpl-*.csc
uxmpp:
server ip= ::1 , port= 5223

STEP3
make connect-router-cooja or
contiki/tools$ sudo ./tunslip6 -a 127.0.0.1 -p 60001 aaaa::1/64

STEP4
START prosody

STEP5
START SIM: hit start button

STEP7
STOP SIM: kill prosody (it will send packets to all clients
 -> THIS can cause strange 'off LINK' errors for new SIM)


IMPORTANT:
when restarting simualtion, make sure to restart prosody, because old session for IPs musst be deleted,
else nodes will kicked with "unavailable" and no login accures!!!
