/*
 * Copyright (c) 2013, Brandenburg University of Technology - Computer
 * Networks and Communication Systems Group
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *         Project specific configuration defines for the XMPP client example.
 *
 * \author
 *         Ronny Klauck - <rklauck@informatik.tu-cottbus.de>
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Free some code and RAM space */
#undef UIP_CONF_DS6_NBR_NBU
#define UIP_CONF_DS6_NBR_NBU     8
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES      8
#undef UIP_CONF_MAX_CONNECTIONS
#define UIP_CONF_MAX_CONNECTIONS 1
#undef UIP_CONF_MAX_LISTENPORTS
#define UIP_CONF_MAX_LISTENPORTS 1
#undef UIP_CONF_UDP_CONNS
#define UIP_CONF_UDP_CONNS       1
/* NOTE: also replace in platform/X/contiki-conf.h */
#undef NETSTACK_CONF_MAC
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_MAC        nullmac_driver //csma_driver
#define NETSTACK_CONF_RDC        nullrdc_driver //contikimac_driver
/* TODO remove cfs-coffee.h from platform/X/Makefile.common */

/* uXMPP config */
#define XMPP_DEBUG               0
/* A minimalistic set to boot XMPP */
#define XMPP_MIN                 0
/* MUC (XEP-0045) */
#define XMPP_MUC                 1
/* Check if received XMPP messages are formated correctly (advanced parsing) */
#define XMPP_COMPAT_CHECK        0
/* anonymous login - if disabled nodes will login with user=node_xmpp and pw=paul */
#define XMPP_LOGIN_ANON          0
/* COOJA sim options */
#define COOJA_SIM                1
/* #root: 6tunnel -46 5223 localhost 5222 */
#define CONF_6TUNNEL             0

/* Sets client in testing mode */
#define XMPP_TEST                0

#endif /* PROJECT_CONF_H_ */
