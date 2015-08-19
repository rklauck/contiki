/*
 * Copyright (c) 2015, Brandenburg University of Technology - Computer
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
 *
 * TODO
 * - SASL authentification (SCRAM-SHA-1)
 *
 */

#include "xmpp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SIMPLE_QUOTE 0x27
#define DOUBLE_QUOTE 0x22
#define RETOUR_LIGNE 0xa
#define SPACE 0x20
#define OPENBALISE 0x3c
#define CLOSEBALISE 0x3e
#define SLASH 0x2f

enum {
  COMMAND_NONE, 
  COMMAND_SENDSTREAM,
  COMMAND_RECEIVESTREAM,
  COMMAND_NEGOTIATETLS,
  COMMAND_NEGOTIATESASL,
  COMMAND_BINDREQUEST,
  COMMAND_SESSIONREQUEST,
  COMMAND_SENDPRESENCE,
#if !XMPP_MIN
#if XMPP_MUC
  COMMAND_JOINCHATROOM,
#if !COOJA_SIM
  COMMAND_LEAVECHATROOM,
#endif /* COOJA_SIM */
  COMMAND_SENDCHATMSG,
#endif /* XMPP_MUC */
  COMMAND_SENDMSG,
#if !COOJA_SIM
  COMMAND_QUIT
#endif /* COOJA_SIM */
#endif /* XMPP_MIN */
};

struct xmpp_connection_state {
  char* msg;
  char* presence;
  char* outputbuf;
  char* server;
  char* from;
  char* to;
  char* xmlns;
  char* xmlns_stream;
  char* version;
  struct pt pt;
  struct psock s;
  struct uip_conn *conn;
  unsigned char command;
  uint8_t sasl_negotiation;
  uint8_t bind_request;
  uint8_t session_request;
  /* TODO use uip_mss() for a dynamic allocation? */
  char inputbuf[200];
  /* anon jid bare length 36 / 24 for domain */
  char jid[60];
};

static struct xmpp_connection_state s;
process_event_t xmpp_auth_done;
#if !XMPP_MIN
#if XMPP_MUC
process_event_t xmpp_joinmuc_done;
#endif
#if !XMPP_TEST
process_event_t xmpp_msg_received_or_send;
process_event_t xmpp_presence_received;
#endif
#endif /* XMPP_MIN */

#define XMPP_SEND_STRING(s,str) PSOCK_SEND(s,(uint8_t *)str,(unsigned char)strlen(str))

const char xmpp_presence_available[2] = {0x00,};
const char xmpp_presence_away[5] = {0x61,0x77,0x61,0x79,};
const char xmpp_presence_chat[5] = {0x63,0x68,0x61,0x74,};
const char xmpp_presence_dnd[4] = {0x64,0x6e,0x64,};
const char xmpp_presence_xa[4] = {0x78,0x61,};

PROCESS(xmpp_process,"Chatty Things - XMPP 4 IoT");
AUTOSTART_PROCESSES(&xmpp_process);

struct xmpp_connection_state *
xmpp_connection_init(struct xmpp_connection_state *s,char* server,char* JID,uip_ipaddr_t* ipaddr)
{
#if CONF_6TUNNEL
  int port=5223;
#else
  int port=5222;
#endif /* CONF_6TUNNEL */
  tcp_connect(ipaddr,UIP_HTONS(port),s);
  ((struct xmpp_connection_state *)s)->server = server;
  ((struct xmpp_connection_state *)s)->from = JID;
  ((struct xmpp_connection_state *)s)->xmlns = "'jabber:client'";
  ((struct xmpp_connection_state *)s)->xmlns_stream = "'http://etherx.jabber.org/streams'";
  ((struct xmpp_connection_state *)s)->version = "'1.0'";
  ((struct xmpp_connection_state *)s)->sasl_negotiation = 0;
  ((struct xmpp_connection_state *)s)->bind_request = 0;
#if XMPP_DEBUG
  if(s == NULL) {
    printf("XMPP state null, must be initialized");
  }
#endif
  return s;
}

#if !XMPP_MIN
#if XMPP_DEBUG
void
xmpp_receive_msg(struct xmpp_connection_state *s,char* msg)
{
  //printf(">Receive message from %s : %s \n",((struct xmpp_connection_state *)s)->from,msg);

#if XMPP_TEST
#if XMPP_MUC
  xmpp_send_muc_msg("24CC", "'temp@conference.localhost'");
#else // MUC
  xmpp_send_msg(msg,"'123@localhost'");
#endif // MUC
#endif // TEST
}

void
xmpp_receive_presence(struct xmpp_connection_state *s,char* msg)
{
  //printf(">Receive presence from %s : %s \n",((struct xmpp_connection_state *)s)->from,msg);
#if XMPP_TEST
  xmpp_send_presence(xmpp_presence_available);
#endif // TEST
}
#endif
#endif /* XMPP_MIN */

#if XMPP_COMPAT_CHECK
/* parse the XML header - check the version */
static void
xmpp_parse_xml_version(char* c)
{
  if(strncmp(c,"<?xml version='1.0'?>",21) != 0) {
#if XMPP_DEBUG
    printf("xml version not received or wrong");
#endif
  }
}

/* parse the stream received from the server */
static void
xmpp_parse_stream(struct xmpp_connection_state *s)
{
  uint8_t j = 0;
  char* c = s->inputbuf;
  uint8_t stream_open = 0;
  uint8_t id = 0;
  uint8_t from = 0;
  /* uint8_t xmlnsDbOk = 0; */
  uint8_t xmlns = 0;
  uint8_t version = 0;
  uint8_t xmlns_stream = 0;
  while(j<strlen(s->inputbuf)) {
    if(strncmp(&c[j],"<stream:stream",15) == 0) {
      stream_open = 1;
    }
    if(strncmp(&c[j],"id=",3) == 0) {
      id = 1;  
    }
    if(strncmp(&c[j],"from=",5) == 0) {
      from = 1;
    }
    /*if(strncmp(&c[j],"xmlns:db='jabber:server:dialback'",33) == 0) {
      xmlnsDbOk = 1;
    }*/
    if(strncmp(&c[j],"xmlns='jabber:client'",21) == 0) {
      xmlns = 1;
    }
    if(strncmp(&c[j],"version='1.0'",13) == 0) {
      version = 1;
    }
    if(strncmp(&c[j],"xmlns:stream='http://etherx.jabber.org/streams'",47) == 0) {
      xmlns_stream = 1;
    }    
    while(c[j] != SPACE) {
      j++;
    }
    j++;
  }
  if((stream_open == 1) && (id == 1) && (from == 1) && (xmlns == 1) && (version == 1) && (xmlns_stream == 1)) {
#if XMPP_DEBUG
    //printf("stream parser ok");
#endif
  } else {
    s->command = COMMAND_QUIT;
  }
}
#endif /* XMPP_COMPAT_CHECK */

/* parse the stanza header received from the server */
static void
xmpp_parse_stanza_header(struct xmpp_connection_state *s)
{
  int i = 0;
  int j = 0;
  int size_alloc;
  char* c = s->inputbuf;
  char* tmp_c;
#if XMPP_COMPAT_CHECK
  uint8_t to;
  uint8_t from;
#endif /* XMPP_COMPAT_CHECK */

  if(s->from != NULL) free(s->from);

  while(j<strlen(s->inputbuf)) {
    if(strncmp(&c[j],"from=",5) == 0) {
      i = j + 5 + 1;
      tmp_c = c;
      size_alloc = 0;
      while(tmp_c[i] != SIMPLE_QUOTE) {
        size_alloc++;
        i++;
      }
      s->from = malloc((size_alloc + 1)*sizeof(char));
      if(s->from != NULL) {
        memcpy(s->from,&c[j + 5 + 1],size_alloc);
        s->from[size_alloc] = '\0';
      }
#if XMPP_COMPAT_CHECK
      from = 1;
    }
    if(strncmp(&c[j],"to=",3) == 0) {
      to = 1;
#endif /* XMPP_COMPAT_CHECK */
    }
    while(c[j] != SPACE) {
      j++;
    }
    j++;
  }
#if XMPP_COMPAT_CHECK
  /* FIXME version DB not included */
  if((from == 1) && (to == 1)) {
#if XMPP_DEBUG
    printf("Stanza header parsed");
#endif
  }
#if !COOJA_SIM
  else {
#if XMPP_DEBUG
    printf("Error parser stanza header");
#endif
    s->command = COMMAND_QUIT;
  }
#endif /* COOJA_SIM */
#endif /* XMPP_COMPAT_CHECK */
}

/* TODO TLS support */
static void
xmpp_parse_stream_feature_tls(struct xmpp_connection_state *s)
{
}

/* TODO XMPP features */
static void
xmpp_parse_stream_feature_mechanism(struct xmpp_connection_state *s)
{
}

/* PROTOTHREADS */
/* Send a stream auth */
PT_THREAD(PT_xmpp_send_stream(struct xmpp_connection_state *s))
{
  memset(s->inputbuf,'\0',sizeof(s->inputbuf));
  strcpy(s->inputbuf,"<?xml version='1.0'?> <stream:stream to=");
  strcat(s->inputbuf,s->server);
  strcat(s->inputbuf," xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams' version='1.0'>");
  PSOCK_BEGIN(&s->s);
  XMPP_SEND_STRING(&s->s,s->inputbuf);  
  s->command = COMMAND_RECEIVESTREAM;
  PSOCK_END(&s->s);
}

/* Receive a stream auth and check fields */
PT_THREAD(PT_xmpp_receive_stream(struct xmpp_connection_state *s))
{
  uint8_t stream_feature_mechanism = 0;
  PSOCK_BEGIN(&s->s);
  PSOCK_READTO(&s->s,CLOSEBALISE);
#if XMPP_COMPAT_CHECK
  xmpp_parse_xml_version(s->inputbuf);
#endif /* XMPP_COMPAT_CHECK */
  PSOCK_READTO(&s->s,CLOSEBALISE);
#if XMPP_COMPAT_CHECK
  xmpp_parse_stream(s);
#endif /* XMPP_COMPAT_CHECK */
  PSOCK_READTO(&s->s,CLOSEBALISE);
  if(strncmp(s->inputbuf,"<stream:error>",13) == 0) {
#if XMPP_DEBUG
    printf("Stream received error");
#endif
    PSOCK_CLOSE(&s->s);
    PSOCK_EXIT(&s->s);
  }

  /* no stream error received */
  if(strncmp(s->inputbuf,"<stream:features",16) == 0) {
    PSOCK_READTO(&s->s,CLOSEBALISE);
    while(strncmp(s->inputbuf,"</stream:features>",18) != 0) {
      //memset(s->inputbuf,'\0',sizeof(s->inputbuf));
#if XMPP_COMPAT_CHECK
      if(strncmp(s->inputbuf,"<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'>",51) == 0) {
#else
      if(strncmp(s->inputbuf,"<starttls xmlns=",16) == 0) {
#endif /* XMPP_COMPAT_CHECK */
        xmpp_parse_stream_feature_tls(s);
      }
#if XMPP_COMPAT_CHECK
      if(strncmp(s->inputbuf,"<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'",46) == 0) {
#else
      if(strncmp(s->inputbuf,"<bind xmlns=",12) == 0) {
#endif /* XMPP_COMPAT_CHECK */
        s->bind_request = 1;
      }
#if XMPP_COMPAT_CHECK
      if(strncmp(s->inputbuf,"<session xmlns='urn:ietf:params:xml:ns:xmpp-session'",52) == 0) {
#else
      if(strncmp(s->inputbuf,"<session xmlns=",15) == 0) {
#endif /* XMPP_COMPAT_CHECK */
        s->session_request = 1;
      }
      if(strncmp(s->inputbuf,"<mechanisms",11) == 0) {
        stream_feature_mechanism = 1;
      }
      if(strncmp(s->inputbuf,"<mechanisms>",12) == 0) {
        if(stream_feature_mechanism == 1) {
          PSOCK_READTO(&s->s,CLOSEBALISE);
          xmpp_parse_stream_feature_mechanism(s);
          PSOCK_READTO(&s->s,CLOSEBALISE);
        }
      }      
      PSOCK_READTO(&s->s,CLOSEBALISE);
    }
  }

  //memset(s->inputbuf,'\0',sizeof(s->inputbuf));
  if(s->sasl_negotiation == 1) {
    if(s->bind_request == 1) {
      s->command = COMMAND_BINDREQUEST;
    }  
  } else {
    s->command = COMMAND_NEGOTIATESASL;
  }  
  
  PSOCK_END(&s->s);
}

/* Send a Bind */ 
PT_THREAD(PT_xmpp_send_bind_request(struct xmpp_connection_state *s))
{
  PSOCK_BEGIN(&s->s);
  XMPP_SEND_STRING(&s->s,"<iq type='set' id='bind_1'><bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/></iq>");

  PSOCK_READTO(&s->s,CLOSEBALISE);
  
  while(strncmp(s->inputbuf,"</iq>",5) != 0) {
    PSOCK_READTO(&s->s,CLOSEBALISE);
    if (strncmp(s->inputbuf,"<jid>",5) == 0) {
      memset(s->inputbuf,'\0',sizeof(s->inputbuf));
      PSOCK_READTO(&s->s,SLASH);
      strcpy(s->jid,"'");
      strcat(s->jid,s->inputbuf);
      s->jid[(strlen(s->jid)) - 1] = 0x27;
    }
  }

#if XMPP_COMPAT_CHECK
  if(s->session_request == 1) {
    s->command = COMMAND_SESSIONREQUEST;
  }
#else
  /* if there was a successful bind we should always send a session request (keep code slim) */
  s->command = COMMAND_SESSIONREQUEST;
#endif /* XMPP_COMPAT_CHECK */
  PSOCK_END(&s->s);
}

/* Send a Session */
PT_THREAD(PT_xmpp_send_session_request(struct xmpp_connection_state *s))
{
  PSOCK_BEGIN(&s->s);
  XMPP_SEND_STRING(&s->s,"<iq type='set' id='session_1'><session xmlns='urn:ietf:params:xml:ns:xmpp-session'/></iq>");
  PSOCK_READTO(&s->s,CLOSEBALISE);
  /*while(strncmp(s->inputbuf,"<\iq>",5) != 0) {
    // TO DO : HANDLE DATA RECEIVED
    printf("%s\n",s->inputbuf);
    memset(s->inputbuf,'\0',sizeof(s->inputbuf));
    PSOCK_READTO(&s->s,CLOSEBALISE);
  }*/
  xmpp_send_presence((char*)xmpp_presence_available);
  PSOCK_END(&s->s);
}
 
#if XMPP_LOGIN_ANON
/* Negotiate a SASL anonymous authentication - http://xmpp.org/extensions/xep-0175.html */
PT_THREAD(PT_xmpp_negotiate_sasl_anonymous(struct xmpp_connection_state *s))
{
  PSOCK_BEGIN(&s->s);
  XMPP_SEND_STRING(&s->s,"<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='ANONYMOUS'/>");
  PSOCK_READTO(&s->s,CLOSEBALISE);
#if XMPP_COMPAT_CHECK
  /* FIXME server: '<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'></success>' but reads 's>' */
  if(strncmp(s->inputbuf,"<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>",50) != 0) {
#if XMPP_DEBUG
    printf("SASL anonymous failed");
#endif
    PSOCK_CLOSE(&s->s);
    PSOCK_EXIT(&s->s);
  }
#endif /* XMPP_COMPAT_CHECK */
  s->command = COMMAND_SENDSTREAM;
  s->sasl_negotiation = 1;
  PSOCK_END(&s->s);
}
#else 
/* Negotiate a SASL plain authentication */
PT_THREAD(PT_xmpp_negotiate_sasl_plain(struct xmpp_connection_state *s))
{
  PSOCK_BEGIN(&s->s);
  XMPP_SEND_STRING(&s->s,"<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'>AG5vZGVfeG1wcABwYXVs</auth>");
  /* <0>node_xmpp<0>paul AG5vZGVfeG1wcABwYXVs */

  PSOCK_READTO(&s->s,CLOSEBALISE);

#if XMPP_COMPAT_CHECK
  /* -> DEACTIVATE if-statement when parsing problem exist! */
  if(strncmp(s->inputbuf,"<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>",50) != 0) {
#if XMPP_DEBUG
    printf("SASL plain failed");
#endif
    PSOCK_CLOSE(&s->s);
    PSOCK_EXIT(&s->s);
  }
#endif /* XMPP_COMPAT_CHECK */
  s->command = COMMAND_SENDSTREAM;
  s->sasl_negotiation = 1;

  PSOCK_END(&s->s);  
}
#endif /* XMPP_LOGIN_ANON */

/* Send a Stanza presence */
PT_THREAD(PT_xmpp_send_presence(struct xmpp_connection_state *s))
{
  memset(s->inputbuf,'\0',sizeof(s->inputbuf));
  strcpy(s->inputbuf,"<presence><show>");
  strcat(s->inputbuf,((struct xmpp_connection_state *)s)->presence);
  strcat(s->inputbuf,"</show></presence>");

  PSOCK_BEGIN(&s->s);
  XMPP_SEND_STRING(&s->s,s->inputbuf);
  PSOCK_END(&s->s);
}

#if !XMPP_MIN

/* Send a Stanza message */
PT_THREAD(PT_xmpp_send_msg(struct xmpp_connection_state *s))
{
  memset(s->inputbuf,'\0',sizeof(s->inputbuf));
  strcpy(s->inputbuf,"<message from=");
  strcat(s->inputbuf,((struct xmpp_connection_state *)s)->jid);
  strcat(s->inputbuf," to=");
  strcat(s->inputbuf,((struct xmpp_connection_state *)s)->to);
  strcat(s->inputbuf," xml:lang='en'><body>");
  strcat(s->inputbuf,((struct xmpp_connection_state *)s)->outputbuf);
  strcat(s->inputbuf,"</body></message>");

  PSOCK_BEGIN(&s->s);
  XMPP_SEND_STRING(&s->s,s->inputbuf);
  PSOCK_END(&s->s);

  ((struct xmpp_connection_state *)s)->to = NULL;
}

#if XMPP_MUC
/* Send a Stanza presence to join a chatroom (XEP-0045) */
PT_THREAD(PT_xmpp_join_muc(struct xmpp_connection_state *s))
{
  memset(s->inputbuf,'\0',sizeof(s->inputbuf));
  strcpy(s->inputbuf,"<presence from=");
  strcat(s->inputbuf,((struct xmpp_connection_state *)s)->jid);
  strcat(s->inputbuf," to=");
  strcat(s->inputbuf,((struct xmpp_connection_state *)s)->to);
#if XMPP_LOGIN_ANON
  char from_new[18];
  strncpy(from_new,((struct xmpp_connection_state *)s)->jid,16);
  /* overwrite,because from begins with ' */
  from_new[0] = '/';
  /* end with ' */
  from_new[16] = 0x27;
  from_new[17] = '\0';
  strcat(s->inputbuf,from_new);
#endif /* XMPP_LOGIN_ANON */
#if XMPP_MUC_TSP
  strcat(s->inputbuf," type='tsp'>");
#else
  strcat(s->inputbuf,">");
#endif /* XMPP_MUC_TSP */
  strcat(s->inputbuf,"<x xmlns='http://jabber.org/protocol/muc'/>");
  strcat(s->inputbuf,"</presence>");

  PSOCK_BEGIN(&s->s);
  XMPP_SEND_STRING(&s->s,s->inputbuf);
  PSOCK_END(&s->s);

  ((struct xmpp_connection_state *)s)->to = NULL;
}

/* Send a Stanza chat message */
PT_THREAD(PT_xmpp_send_muc_msg(struct xmpp_connection_state *s))
{
  memset(s->inputbuf,'\0',sizeof(s->inputbuf));
  strcpy(s->inputbuf,"<message to=");
  strcat(s->inputbuf,((struct xmpp_connection_state *)s)->to);
  strcat(s->inputbuf," from=");
  strcat(s->inputbuf,((struct xmpp_connection_state *)s)->jid);
  strcat(s->inputbuf," type='groupchat'><body>");
  strcat(s->inputbuf,((struct xmpp_connection_state *)s)->outputbuf);
  strcat(s->inputbuf,"</body></message>");

  PSOCK_BEGIN(&s->s);
  XMPP_SEND_STRING(&s->s,s->inputbuf);
  PSOCK_END(&s->s);
}

#if !COOJA_SIM || !XMPP_MIN
/* Send a Stanza presence to leave a chatroom (XEP-0045) */
PT_THREAD(PT_xmpp_leave_muc(struct xmpp_connection_state *s))
{
  memset(s->inputbuf,'\0',sizeof(s->inputbuf));
  strcpy(s->inputbuf,"<presence from=");
  strcat(s->inputbuf,((struct xmpp_connection_state *)s)->jid);
  strcat(s->inputbuf," to=");
  strcat(s->inputbuf,((struct xmpp_connection_state *)s)->to);
#if XMPP_LOGIN_ANON
  char from_new[18];
  strncpy(from_new,((struct xmpp_connection_state *)s)->jid,16);
  /* overwrite, because from begins with ' */
  from_new[0] = '/';
  /* end with ' */
  from_new[16] = 0x27;
  from_new[17] = '\0';
  strcat(s->inputbuf,(char*) from_new);
#endif /* XMPP_LOGIN_ANON */
  strcat(s->inputbuf," type='unavailable'/>");

  PSOCK_BEGIN(&s->s);
  XMPP_SEND_STRING(&s->s,s->inputbuf);
  PSOCK_END(&s->s);

  ((struct xmpp_connection_state *)s)->to = NULL;
}
#endif /* COOJA_SIM */
#endif /* XMPP_MUC */

#if !COOJA_SIM
/* Close client stream and check if the server close his stream as well */
PT_THREAD(PT_xmpp_close_stream(struct xmpp_connection_state *s))
{  
  PSOCK_BEGIN(&s->s);
  XMPP_SEND_STRING(&s->s,"</stream:stream>");
  PSOCK_READTO(&s->s,RETOUR_LIGNE);
  if(strncmp(s->inputbuf,"</stream:stream>",16) != 0) {
#if XMPP_DEBUG
    printf("</stream> missing");
#endif
    PSOCK_CLOSE(&s->s);
    PSOCK_EXIT(&s->s);
  }
  PSOCK_CLOSE(&s->s);
  PSOCK_END(&s->s);
}
#endif /* COOJA_SIM */

/* handle input stanza */
static
PT_THREAD(PT_xmpp_handle_input_stanza(struct xmpp_connection_state *s))
{
  char *ptr;
  PSOCK_BEGIN(&s->s);
  PSOCK_READTO(&s->s,CLOSEBALISE);
  if(strncmp(s->inputbuf,"<message ",9) == 0) {
    PSOCK_READTO(&s->s,CLOSEBALISE);
    xmpp_parse_stanza_header(s);
    while(strncmp(s->inputbuf,"</message>",10) != 0) {
      if(strncmp(s->inputbuf,"<body>",6) == 0) {
        PSOCK_READTO(&s->s,OPENBALISE);
        ptr = malloc(PSOCK_DATALEN(&s->s) - 1);
        if(ptr != NULL) {
          memcpy(ptr,s->inputbuf,PSOCK_DATALEN(&s->s) - 1);
          ptr[PSOCK_DATALEN(&s->s) - 1] = '\0';
        }
        PSOCK_READTO(&s->s,CLOSEBALISE);
        if(strncmp(s->inputbuf,"/body>",6) == 0) {
#if XMPP_TEST
#if XMPP_DEBUG
          xmpp_receive_msg(s,ptr);
#endif // DEBUG
#else
          process_post(PROCESS_BROADCAST, xmpp_msg_received_or_send, ptr);
#endif
        }
        if(ptr != NULL) free(ptr);
      }
      PSOCK_READTO(&s->s,CLOSEBALISE);
    }
    //free(s->from);
  }

  if(strncmp(s->inputbuf,"<presence",9) == 0) {
    PSOCK_READTO(&s->s,CLOSEBALISE);
    xmpp_parse_stanza_header(s);
    while(strncmp(s->inputbuf,"</presence>",11) != 0) {
      if(strncmp(s->inputbuf,"<show>",6) == 0) {
        PSOCK_READTO(&s->s,OPENBALISE);
        if(ptr != NULL) {
          ptr = malloc(PSOCK_DATALEN(&s->s) - 1);
          memcpy(ptr,s->inputbuf,PSOCK_DATALEN(&s->s) - 1);
          ptr[PSOCK_DATALEN(&s->s) - 1] = '\0';
        }
        PSOCK_READTO(&s->s,CLOSEBALISE);
        if(strncmp(s->inputbuf,"/show>",6) == 0) {
#if XMPP_TEST
#if XMPP_DEBUG
          xmpp_receive_presence(s,ptr);
#endif // DEBUG
#else
          process_post(PROCESS_BROADCAST, xmpp_presence_received, ptr);
#endif
        }
        if(ptr != NULL) free(ptr);
      }

      if(strncmp(s->inputbuf,"<show />",8) == 0) { // avail
#if XMPP_TEST
#if XMPP_DEBUG
        xmpp_receive_presence(s,ptr);
#endif // DEBUG
#else
        process_post(PROCESS_BROADCAST, xmpp_presence_received, ptr);
#endif
      }
      PSOCK_READTO(&s->s,CLOSEBALISE);
    }
    //free(s->from);
  }
  PSOCK_END(&s->s);
}
#endif /* XMPP_MIN */

/* wait until a new data or a command occur */
static
PT_THREAD(PT_xmpp_data_or_command(struct xmpp_connection_state *s))
{
  PSOCK_BEGIN(&s->s);
  PSOCK_WAIT_UNTIL(&s->s,PSOCK_NEWDATA(&s->s) || (s->command != COMMAND_NONE));
  PSOCK_END(&s->s);
}

/* connection handler */
static
PT_THREAD(PT_xmpp_handle_connection(struct xmpp_connection_state *s))
{
  PT_BEGIN(&s->pt);
  PSOCK_INIT(&s->s,(uint8_t *)s->inputbuf,sizeof(s->inputbuf) - 1);
  while(1) {
    PT_WAIT_UNTIL(&s->pt,PT_xmpp_data_or_command(s));
    if(s->command == COMMAND_SENDSTREAM) {
#if XMPP_DEBUG
      printf("SEND STREAM \n");
#endif
      PT_WAIT_THREAD(&s->pt,PT_xmpp_send_stream(s));
    } else if(s->command == COMMAND_RECEIVESTREAM) {
#if XMPP_DEBUG
      printf("RECEIVE STREAM \n");
#endif
      PT_WAIT_THREAD(&s->pt,PT_xmpp_receive_stream(s));
    } else if(s->command == COMMAND_NEGOTIATESASL) {
#if XMPP_LOGIN_ANON
      PT_WAIT_THREAD(&s->pt,PT_xmpp_negotiate_sasl_anonymous(s));
#else
      PT_WAIT_THREAD(&s->pt,PT_xmpp_negotiate_sasl_plain(s));
#endif /* XMPP_LOGIN_ANON */
    } else if(s->command == COMMAND_BINDREQUEST) {
      PT_WAIT_THREAD(&s->pt,PT_xmpp_send_bind_request(s));
    } else if(s->command == COMMAND_SESSIONREQUEST) {
      PT_WAIT_THREAD(&s->pt,PT_xmpp_send_session_request(s));
    } else if(s->command == COMMAND_SENDPRESENCE) {
      PT_WAIT_THREAD(&s->pt,PT_xmpp_send_presence(s));
      process_post(PROCESS_BROADCAST,xmpp_auth_done,NULL);
      s->command = COMMAND_NONE;
    }
#if !XMPP_MIN
#if XMPP_MUC
    else if(s->command == COMMAND_JOINCHATROOM) {
      PT_WAIT_THREAD(&s->pt,PT_xmpp_join_muc(s));
      process_post(PROCESS_BROADCAST,xmpp_joinmuc_done,NULL);
      s->command = COMMAND_NONE;
    } else if(s->command == COMMAND_SENDCHATMSG) {
      PT_WAIT_THREAD(&s->pt,PT_xmpp_send_muc_msg(s));
      s->command = COMMAND_NONE;
    }
#if !COOJA_SIM
    else if(s->command == COMMAND_LEAVECHATROOM) {
      PT_WAIT_THREAD(&s->pt,PT_xmpp_leave_muc(s));
      s->command = COMMAND_QUIT;
    }
#endif /* COOJA_SIM */
#endif /* XMPP_MUC */
    else if(s->command == COMMAND_SENDMSG) {
      PT_WAIT_THREAD(&s->pt,PT_xmpp_send_msg(s));
      s->command = COMMAND_NONE;
#if !XMPP_TEST
      process_post(PROCESS_BROADCAST, xmpp_msg_received_or_send, NULL);
#endif // TEST
    }
#if !COOJA_SIM
    else if(s->command == COMMAND_QUIT) {
      PT_WAIT_THREAD(&s->pt,PT_xmpp_close_stream(s));
      process_exit(&xmpp_process);
    }
#endif /* COOJA_SIM */
    else if(PSOCK_NEWDATA(&s->s) && s->command == COMMAND_NONE) {
      PT_WAIT_THREAD(&s->pt,PT_xmpp_handle_input_stanza(s));
    }
#endif /* XMPP_MIN */
  }
  PT_END(&s->pt);
}
/* END PROTOTHREAD */

/* API STANZA FUNCTIONS */
void xmpp_connect(uip_ipaddr_t* ipaddr,char* server,char* JID)
{
  PROCESS_CONTEXT_BEGIN(&xmpp_process);
#if XMPP_DEBUG
#if UIP_CONF_IPV6
  printf(">IP: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x \n",((uint8_t*)ipaddr)[0],((uint8_t*)ipaddr)[1],((uint8_t*)ipaddr)[2],((uint8_t*)ipaddr)[3],((uint8_t*)ipaddr)[4],((uint8_t*)ipaddr)[5],((uint8_t*)ipaddr)[6],((uint8_t*)ipaddr)[7],((uint8_t*)ipaddr)[8],((uint8_t*)ipaddr)[9],((uint8_t*)ipaddr)[10],((uint8_t*)ipaddr)[11],((uint8_t*)ipaddr)[12],((uint8_t*)ipaddr)[13],((uint8_t*)ipaddr)[14],((uint8_t*)ipaddr)[15]);
#else
  printf(">IP: %u.%u.%u.%u\n",uip_ipaddr_to_quad(ipaddr));
#endif /* UIP_CONF_IPV6 */
#endif /* XMPP_DEBUG */
  xmpp_connection_init(&s,server,JID,ipaddr);
  PROCESS_CONTEXT_END(&xmpp_process);
}

#if !XMPP_MIN
void
xmpp_send_msg(char* msg,char* to)
{
  if(&s != NULL) {
    s.to = to;
    s.outputbuf = msg;
    s.command = COMMAND_SENDMSG;
  }
}
#endif /* XMPP_MIN */

void
xmpp_send_presence(char* presence)
{
  if(&s != NULL) {
    s.presence = presence;
    s.command = COMMAND_SENDPRESENCE;
  }
}

#if !XMPP_MIN
#if XMPP_MUC
void
xmpp_join_muc(char* to)
{
  if(&s != NULL) {
    s.to = to;
    s.command = COMMAND_JOINCHATROOM;
  }
}

void
xmpp_send_muc_msg(char* msg,char* to)
{
  if(&s != NULL) {
    s.to = to;
    s.outputbuf = msg;
    s.command = COMMAND_SENDCHATMSG;
  }
}

void
xmpp_leave_muc(char* to)
{
#if !COOJA_SIM
  if(&s != NULL) {
    s.to = to;
    s.command = COMMAND_LEAVECHATROOM;
  }
#endif /* COOJA_SIM */
}
#endif /* XMPP_MUC */
#endif /* XMPP_MIN */

PROCESS_THREAD(xmpp_process,ev,data)
{
  PROCESS_BEGIN(); 

#if !XMPP_MIN
#if XMPP_MUC
  xmpp_joinmuc_done = process_alloc_event();
#endif
#endif /* XMPP_MIN */
  xmpp_auth_done = process_alloc_event();

#if XMPP_DEBUG
  printf(">Connected \n");
#endif

  while(1) {
    PROCESS_WAIT_EVENT(); 
      if(ev == tcpip_event) {
#if !XMPP_MIN
#if XMPP_DEBUG
      if(uip_aborted()) {
        printf("connection aborted\n");
      } else if(uip_timedout()) {
        printf("timedout\n");
      } else if(uip_closed()) {
        printf("connection closed");
      } else
#endif /* XMPP_DEBUG */
#endif /* XMPP_MIN */
      if(uip_connected()) {
        PT_INIT(&((struct xmpp_connection_state *)data)->pt);
        ((struct xmpp_connection_state *)data)->command = COMMAND_SENDSTREAM;      
        PT_xmpp_handle_connection(data);
      } else if(data != NULL) {
        PT_xmpp_handle_connection(data);
      }
    }
  }

  PROCESS_END();
}

