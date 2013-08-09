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

#include "contiki.h"
#include "xmpp.h"

#if UIP_UDP

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

#if !XMPP_TEST
/*#if defined (PLATFORM_HAS_BATT)
#include "dev/battery-sensor.h"
#endif*/
static struct etimer READ_SENSOR_TIMEOUT;
static uint8_t xmpp_sensor_timer = 15;
static char buf[5];

#if defined (ZOLERTIA_Z1)
#include "dev/temperature-sensor.h"
static float get_temp(void){ return (float) (((temperature_sensor.value(0)*2.500)/4096)-0.986)*282;}
#else
static float get_temp(void){ return (float) ((rand() % 5) + 25);}
#endif

#endif /* !TEST */


PROCESS(example_process, "XMPP external");
AUTOSTART_PROCESSES(&example_process, &xmpp_process);


PROCESS_THREAD(example_process, ev, data)
{

  PROCESS_BEGIN();

#if UIP_CONF_IPV6
  uip_ipaddr_t ipaddr = {.u8 = { 
    0xaa, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,}
  };
#else
  uip_ipaddr_t ipaddr;
  uip_ipaddr(&ipaddr,192,168,1,35); // my IPv4 address
#endif // UIP_CONF_IPV6

#if XMPP_LOGIN_ANON
  xmpp_connect(&ipaddr,"'anon.localhost'","'node_xmpp@localhost'");
#else
  /* TODO parameter pw - pw is a static hash: AG5vZGVfeG1wcABwYXVs */ 
  xmpp_connect(&ipaddr,"'localhost'","'node_xmpp@localhost'");
#endif
    
  while(1) {
    
    PROCESS_WAIT_EVENT();
    
    if(ev == xmpp_auth_done) {
      printf("EVENT AUTH DONE\n");
#if XMPP_MUC
#if !XMPP_MIN
#if XMPP_LOGIN_ANON
      xmpp_join_muc("'temp@conference.localhost");
#else
      xmpp_join_muc("'temp@conference.localhost/node_xmpp'");
#endif /* XMPP_LOGIN_ANON */
#endif /* XMPP_MIN */
#else /* MUC */
      xmpp_send_presence(xmpp_presence_available);
#if !XMPP_MIN
      xmpp_send_msg("location=Cottbus, unit=°C","'123@localhost'");
#endif /* XMPP_MIN */
#endif /* MUC */
      }
#if !XMPP_MIN
#if XMPP_MUC
    else if(ev == xmpp_joinmuc_done) {
      printf("EVENT JOIN DONE\n");
      xmpp_send_muc_msg("location=Cottbus, unit=°C", "'temp@conference.localhost'");

#if !XMPP_TEST
      etimer_set(&READ_SENSOR_TIMEOUT, CLOCK_SECOND*xmpp_sensor_timer);
#endif
    }
    /*if(ev == sensors_event) {  // If the event it's provoked by the user button, then...
       if(data == &button_sensor) {
printf("EVENT LEAVE DONE");
#if XMPP_LOGIN_ANON
      xmpp_leave_muc(s,"'testroom@conference.localhost");
#else  
      xmpp_leave_muc(s,"'testroom@conference.localhost/node_xmpp'");
#endif // XMPP_LOGIN_ANON
       }
    }*/

#if !XMPP_TEST
    else if(ev == xmpp_msg_received_or_send) {

      if (data != NULL) { // msg received
#if XMPP_DEBUG
	printf(">Received message: %s\n",(char *) data);
#endif

        // what command is it?
	if(strncmp("int++",(char*) data, 5) == 0) {
#if XMPP_DEBUG
          printf("int++ command\n");
#endif
	  xmpp_sensor_timer = xmpp_sensor_timer + 10;
          etimer_set(&READ_SENSOR_TIMEOUT, CLOCK_SECOND*xmpp_sensor_timer);
	} else if(strncmp("int--",(char*) data, 5) == 0) {
#if XMPP_DEBUG
          printf("int-- command\n");
#endif
          if (xmpp_sensor_timer - 10 > 0) {
	    xmpp_sensor_timer = xmpp_sensor_timer - 10;
            etimer_set(&READ_SENSOR_TIMEOUT, CLOCK_SECOND*xmpp_sensor_timer);
          }
	}

      }

    } else if(ev == PROCESS_EVENT_TIMER) {

      if(data == &READ_SENSOR_TIMEOUT && etimer_expired(&READ_SENSOR_TIMEOUT)) {
#if XMPP_DEBUG
        printf("READ_SENSOR_TIMEOUT\n");
#endif

        // read sensor data
        snprintf(&buf[0], sizeof(buf), "%d\n", (int) get_temp());
        xmpp_send_muc_msg(buf, "'temp@conference.localhost'");
	
	etimer_reset(&READ_SENSOR_TIMEOUT);
      }
  
    }
#endif /* !TEST */

#endif /* MUC */
#endif /* XMPP_MIN */
  }
  
  PROCESS_END();
}

#endif /* UIP_UDP */
