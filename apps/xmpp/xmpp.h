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
 *
 * \author
 *         Ronny Klauck - <rklauck@informatik.tu-cottbus.de>
 */

#ifndef __XMPP_H__
#define __XMPP_H__
#include "contiki-net.h"

extern const char xmpp_presence_available[2];
extern const char xmpp_presence_away[5];
extern const char xmpp_presence_chat[5];
extern const char xmpp_presence_dnd[4];
extern const char xmpp_presence_xa[4];

void xmpp_connect(uip_ipaddr_t* ipaddr,char* server,char* JID);
void xmpp_send_presence(char* presence);
#if !XMPP_MIN
void xmpp_send_msg(char* msg,char* to);
#if XMPP_MUC
void xmpp_join_muc(char* to);
void xmpp_send_muc_msg(char* msg,char* to);
void xmpp_leave_muc(char* to);
#endif /* XMPP_MUC */
#endif /* XMPP_MIN */

CCIF extern process_event_t xmpp_auth_done;

#if XMPP_MUC
CCIF extern process_event_t xmpp_joinmuc_done;
#endif /* XMPP_MUC */

#if !XMPP_TEST
CCIF extern process_event_t xmpp_msg_received_or_send; // send: data == NULL
CCIF extern process_event_t xmpp_presence_received;
#endif

PROCESS_NAME(xmpp_process);

#endif
