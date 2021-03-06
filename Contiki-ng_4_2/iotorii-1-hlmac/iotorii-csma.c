/*
 * Copyright (C) 2018 Elisa Rojas(1), Hedayat Hosseini(2), and David Carrascal(1);
 *                    (1) GIST, University of Alcala, Spain.
 *                    (2) CEIT, Amirkabir University of Technology (Tehran
 *                        Polytechnic), Iran.
 * Adapted to use IoTorii, a link layer protocol for Low pawer and Lossy Network
 * (LLN), over the IEEE 802.15.4 standard CSMA protocol (nonbeacon-enabled).
 *
 */

/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 */

/**
 * \file
*         The 802.15.4 standard CSMA protocol (nonbeacon-enabled)
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Simon Duquennoy <simon.duquennoy@inria.fr>
 */

//EXTRA BEGIN
//#include "net/mac/csma/csma.h"
//#include "net/mac/csma/csma-output.h"
#include "csma-output.h"
#include "iotorii-csma.h"
#include "sys/ctimer.h"
#include "lib/list.h"
#include <stdlib.h> //For malloc()
#include "lib/random.h"
//EXTRA END
#include "net/mac/mac-sequence.h"
#include "net/packetbuf.h"
#include "net/netstack.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSMA"
#define LOG_LEVEL LOG_LEVEL_MAC

//EXTRA BEGIN
#if IOTORII_NODE_TYPE == 1 //only root
static struct ctimer sethlmac_timer;
#endif
#if IOTORII_NODE_TYPE > 0 //root and common node
static struct ctimer hello_timer;
#endif

#if IOTORII_NODE_TYPE > 0 //root and common node
//Create Neighbour table
LIST(neighbour_table_entry_list);
//number_of_neighbours = 0; //This variable is initialized in the init function.

/* This version of IoTorii assigns one HLMAC address, so there is not used in
 * thiis case. We just use a variable to hold an assigned HLMAC address, and it
 * is initialized in the init function.
 */
//LIST(iotorii_hlmac_table);
//EXTRA END
#endif

static void
init_sec(void)
{
#if LLSEC802154_USES_AUX_HEADER
  if(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) ==
     PACKETBUF_ATTR_SECURITY_LEVEL_DEFAULT) {
    packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL,
                       CSMA_LLSEC_SECURITY_LEVEL);
  }
#endif
}
/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{

  init_sec();

  csma_output_packet(sent, ptr);
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return NETSTACK_RADIO.on();
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return NETSTACK_RADIO.off();
}
/*---------------------------------------------------------------------------*/
static int
max_payload(void)
{
  int framer_hdrlen;

  init_sec();

  framer_hdrlen = NETSTACK_FRAMER.length();

  if(framer_hdrlen < 0) {
    /* Framing failed, we assume the maximum header length */
    framer_hdrlen = CSMA_MAC_MAX_HEADER;
  }

  return CSMA_MAC_LEN - framer_hdrlen;
}
/*---------------------------------------------------------------------------*/
//EXTRA BEGIN
#ifdef IOTORII_NODE_TYPE
#if IOTORII_NODE_TYPE > 0 //The root or common nodes
static void
iotorii_handle_hello_timer()
{
  int mac_max_payload = max_payload();
  if(mac_max_payload <= 0) {
   /* Framing failed, Hello can not be created. */
     LOG_WARN("output: failed to calculate payload size - Hello can not be created\n");
  }else{
    packetbuf_clear(); //Hello has no payload.
    /*
    * The destination address, the broadcast address, is tagged to the outbound
    * packet.
    */
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
    LOG_DBG("Hello prepared to send\n");
    send_packet(NULL,NULL);
    //ctimer_reset(&hello_timer); //Restart the timer from the previous expire time.
    //ctimer_restart(&hello_timer); //Restart the timer from current time.
    //ctimer_stop(&hello_timer); //Stop the timer.
  }
}
/*---------------------------------------------------------------------------*/
void
iotorii_send_sethlmac(void)
{
  int mac_max_payload = max_payload();
  if(mac_max_payload <= 0) {
   /* Framing failed, SetHLMAC can not be created */
     LOG_WARN("output: failed to calculate payload size - Hello can not be created\n");
  }else{
    //Accomodate SetHLMAC addresses in the payload.
    //uint8_t cleared_packetbuf = 0; //Packet buffer needs to be cleared.
    neighbour_table_entry_t *neighbour_entry = list_head(neighbour_table_entry_list);
    //uint8_t neighbour_id = neighbour_entry->id;
    /* A pointer to walk on the packet buffer array/pointer */
    uint8_t *packetbuf_ptr = NULL;
    int dalalen_counter = 0;
    uint8_t i = 1;

    /* The ayload structure:
     * +------------+--------+-----+------+-----+-----+------+
     * | Prefix len | Prefix | ID1 | MAC1 | ... | IDn | MACn |
     * +------------+--------+-----+------+-----+-----+------+
     */

    /* Add first neighbour
     * 2=1 is for adding the length of the HLMAC address prefix+1 for adding id.
     */

     ////
     LOG_DBG("Info before sending SetHLMAC: ");
     LOG_DBG("Number of neighbours: %d, mac_max_payload: %d, LINKADDR_SIZE: %d.\n", number_of_neighbours, mac_max_payload, LINKADDR_SIZE);
     ////

     //if (neighbour_entry && (packetbuf_datalen() >=  (node_hlmac_address.len + 2 + LINKADDR_SIZE))){ //both statements are true
     if (number_of_neighbours > 0 && i<=number_of_neighbours && neighbour_entry && (mac_max_payload >=  (node_hlmac_address.len + 2 + LINKADDR_SIZE))){
      //if(!cleared_packetbuf){
        //cleared_packetbuf = 1;
        //Preparing the packet buffer
        /* reset packetbuf buffer */
        packetbuf_clear();
        packetbuf_ptr = packetbuf_dataptr();
        /* copy "payload" */
        /* Prefix lenght */
        memcpy(packetbuf_ptr, &(node_hlmac_address.len), 1);
        packetbuf_ptr ++;
        dalalen_counter ++;
        /* Neighbour prefix */
        memcpy(packetbuf_ptr, node_hlmac_address.address, node_hlmac_address.len);
        packetbuf_ptr += node_hlmac_address.len;
        dalalen_counter += node_hlmac_address.len;
      //} //END IF cleared_packetbuf

      do{  //for (; packetbuf_datalen() >=  (dalalen_counter + LINKADDR_SIZE + 1) && i<=number_of_neighbours && neighbour_entry; i++){
        /* Neighbour ID */
        memcpy(packetbuf_ptr, &(neighbour_entry->id), 1);
        packetbuf_ptr ++;
        dalalen_counter ++;
        /* Neighbour MAC address */
        memcpy(packetbuf_ptr, &(neighbour_entry->addr), LINKADDR_SIZE);
        packetbuf_ptr += LINKADDR_SIZE;
        dalalen_counter += LINKADDR_SIZE;

        neighbour_entry = list_item_next(neighbour_entry);
        i++;
      }while (mac_max_payload >=  (dalalen_counter + LINKADDR_SIZE + 1) && i<=number_of_neighbours && neighbour_entry); //End for

      packetbuf_set_datalen(dalalen_counter);
      /*
      * Control info: the destination address, the broadcast address, is tagged to the outbound
      * packet.
      */
      packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
      char *neighbour_hlmac_addr_str = hlmac_addr_to_str(node_hlmac_address);
      LOG_DBG("SetHLMAC prefix (addr:%s) sent to advertise to %d nodes.\n", neighbour_hlmac_addr_str, i-1);
      free(neighbour_hlmac_addr_str);
      LOG_DBG("SetHLMAC prepared to send\n");
      send_packet(NULL, NULL);

    }else{ //End if datalen
      LOG_DBG("Node hos not any neighbour/payload is low to send SetHLMAC.\n");
    }

  }// END else
}
#endif
/*---------------------------------------------------------------------------*/
#if IOTORII_NODE_TYPE == 1 //For root
static void
iotorii_handle_sethlmac_timer()
{
  //uint8_t id = 1;
  hlmac_assign_root_addr(1);
  iotorii_send_sethlmac();
  //ctimer_reset(&sethlmac_timer); //Restart the timer from the previous expire time.
  //ctimer_restart(&sethlmac_timer); //Restart the timer from current time.
  //ctimer_stop(&sethlmac_timer); //Stop the timer.
}
#endif
#endif  // end IOTORII_NODE_TYPE
//EXTRA END
/*---------------------------------------------------------------------------*/
static void
init(void)
{
#if LLSEC802154_USES_AUX_HEADER
#ifdef CSMA_LLSEC_DEFAULT_KEY0
  uint8_t key[16] = CSMA_LLSEC_DEFAULT_KEY0;
  csma_security_set_key(0, key);
#endif
#endif /* LLSEC802154_USES_AUX_HEADER */
  csma_output_init();
  on();
  //EXTRA BEGIN
#ifdef IOTORII_NODE_TYPE
#if IOTORII_NODE_TYPE == 1 //Root node
  LOG_INFO("This node operates as a root.\n");
#endif
#if IOTORII_NODE_TYPE > 1 //Common node
  LOG_INFO("This node operates as a common node.\n ");
#endif
#if IOTORII_NODE_TYPE > 0 //Root or Common node, we set a timer to send a Hello message.
  clock_time_t hello_start_time = 2 * CLOCK_SECOND;
  hello_start_time = hello_start_time / 2 + (random_rand() % (hello_start_time / 2)); //Hello is propagated at 1sec ~ 2sec after node initialization
  LOG_DBG("Scheduling a Hello message after %u ticks in the future\n", (unsigned)hello_start_time);
  ctimer_set(&hello_timer, hello_start_time, iotorii_handle_hello_timer, NULL);

  //Create Neighbour table
  number_of_neighbours = 0;
  node_hlmac_address = UNSPECIFIED_HLMAC_ADDRESS;
#endif
#if IOTORII_NODE_TYPE == 1 //Root node, we set a timer to send a SetHLMAC address.
  clock_time_t sethlmac_start_time;
  sethlmac_start_time = 4 * CLOCK_SECOND; //after 4 seconds
  LOG_DBG("Scheduling a SetHLMAC message after %u ticks in the future\n", (unsigned)sethlmac_start_time);
  ctimer_set(&sethlmac_timer, sethlmac_start_time, iotorii_handle_sethlmac_timer, NULL);
#endif
#endif
//EXTRA END
}
/*---------------------------------------------------------------------------*/
//EXTRA BEGIN
void
iotorii_handle_incoming_hello() //To process an IoTorii Hello control broadcast packet received from other nodes
{
  const linkaddr_t *sender_addr = packetbuf_addr(PACKETBUF_ADDR_SENDER);

  LOG_DBG("A Hello message received from ");
  LOG_DBG_LLADDR(sender_addr);
  LOG_DBG("\n");

  if (number_of_neighbours < 256){
    uint8_t address_is_in_table = 0;
    neighbour_table_entry_t *new_nb;
    //Check if the address is in the list.
    for(new_nb=list_head(neighbour_table_entry_list); new_nb!=NULL; new_nb=new_nb->next){
      if (linkaddr_cmp(&(new_nb->addr), sender_addr)){ //This condithion can be merged with the "for" condition.
        address_is_in_table = 1;
      }
    }
    if (!address_is_in_table){
      new_nb = (neighbour_table_entry_t *)malloc(sizeof(neighbour_table_entry_t));
      new_nb->id = ++number_of_neighbours;
      new_nb->addr = *sender_addr;
      list_add(neighbour_table_entry_list, new_nb);
      LOG_DBG("A new neighbour added to IoTorii neighbour table, address: ");
      LOG_DBG_LLADDR(sender_addr);
      LOG_DBG(", ID: %d\n", new_nb->id);
    }else{
      LOG_DBG("Address of hello (");
      LOG_DBG_LLADDR(sender_addr);
      LOG_DBG(") message received already!\n");
    }
  }else{
    LOG_WARN("The IoTorii neighbour table is full! \n");
  }
////
  //Write neighbour table
  neighbour_table_entry_t *nb;
  LOG_DBG("Naighbour Table:");
  for(nb=list_head(neighbour_table_entry_list); nb!=NULL; nb=list_item_next(nb)){
    LOG_DBG("Id: %d, MAC addr:",nb->id);
    LOG_DBG_LLADDR(&nb->addr);
    LOG_DBG(" - ");
  }
  LOG_DBG("\n");
////
}
/*---------------------------------------------------------------------------*/
/*
 *To process an IoTorii SetHLMAC control broadcast packet received from other
 *nodes
 */
void
iotorii_handle_incoming_sethlamc()
{
  LOG_DBG("A SetHLMAC message received from ");
  LOG_DBG_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
  LOG_DBG("\n");

  uint8_t *packetbuf_ptr;
  packetbuf_ptr = packetbuf_dataptr();
  int dalalen_counter = 0;
  //uint8_t pref_len;
  uint8_t id = 0;
  hlmacaddr_t *prefix = NULL;
  linkaddr_t link_address = linkaddr_null;
  uint8_t is_first_record = 1; // The first record of each payload includes a prefix lenght and prefix.

  /* Read "payload" */
  while(hlmac_is_unspecified_addr(node_hlmac_address) && dalalen_counter < packetbuf_datalen() && !linkaddr_cmp(&link_address, &linkaddr_node_addr)){
    if (is_first_record){
      is_first_record = 0;
      /* Prefix lenght */
      prefix = (hlmacaddr_t *) malloc(sizeof(hlmacaddr_t));
      memcpy(&prefix->len, packetbuf_ptr, 1);
      packetbuf_ptr ++;
      dalalen_counter ++;
      /* Neighbour prefix */
      prefix->address = (uint8_t *) malloc(sizeof(uint8_t) * prefix->len);
      memcpy(prefix->address, packetbuf_ptr, prefix->len);
      packetbuf_ptr += prefix->len;
      dalalen_counter += prefix->len;
    }
    /* Neighbour ID */
    memcpy(&id, packetbuf_ptr, 1);
    packetbuf_ptr ++;
    dalalen_counter ++;
    /* MAC address */
    memcpy(&link_address, packetbuf_ptr, LINKADDR_SIZE);
    packetbuf_ptr += LINKADDR_SIZE;
    dalalen_counter += LINKADDR_SIZE;
  } // End while
  if(linkaddr_cmp(&link_address, &linkaddr_node_addr)){
    hlmac_add_new_id(prefix, id);
    hlmac_assign_address(*prefix);
    iotorii_send_sethlmac(); //To advertise the prefix
  }
  if(is_first_record == 0){
    free(prefix->address);
    free(prefix);
    prefix = NULL;
  }
}
/*---------------------------------------------------------------------------*/
void
iotorii_operation(void)
{
  if (packetbuf_holds_broadcast()){
    //if (hello identification) //
    if (packetbuf_datalen() == 0)
      iotorii_handle_incoming_hello();
    //else if (SetHLMAC identification)
    else
      iotorii_handle_incoming_sethlamc();
    //else
      //handle_data_broadcast(); //To send a data broadcast packet, received from another node, to the upper layer
  }//else
    //handle_handle_unicast();  //For handling an unicast packet received from another node
}
//EXTRA END
/*---------------------------------------------------------------------------*/
static void
input_packet(void)
{
#if CSMA_SEND_SOFT_ACK
  uint8_t ackdata[CSMA_ACK_LEN];
#endif

  if(packetbuf_datalen() == CSMA_ACK_LEN) {
    /* Ignore ack packets */
    LOG_DBG("ignored ack\n");
  } else if(csma_security_parse_frame() < 0) {
    LOG_ERR("failed to parse %u\n", packetbuf_datalen());
  } else if(!linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
                                         &linkaddr_node_addr) &&
            !packetbuf_holds_broadcast()) {
    LOG_WARN("not for us\n");
  } else if(linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_SENDER), &linkaddr_node_addr)) {
    LOG_WARN("frame from ourselves\n");
  } else {
    int duplicate = 0;

    /* Check for duplicate packet. */
    duplicate = mac_sequence_is_duplicate();
    if(duplicate) {
      /* Drop the packet. */
      LOG_WARN("drop duplicate link layer packet from ");
      LOG_WARN_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
      LOG_WARN_(", seqno %u\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO));
    } else {
      mac_sequence_register_seqno();
    }

#if CSMA_SEND_SOFT_ACK
    if(packetbuf_attr(PACKETBUF_ATTR_MAC_ACK)) {
      ackdata[0] = FRAME802154_ACKFRAME;
      ackdata[1] = 0;
      ackdata[2] = ((uint8_t *)packetbuf_hdrptr())[2];
      NETSTACK_RADIO.send(ackdata, CSMA_ACK_LEN);
    }
#endif /* CSMA_SEND_SOFT_ACK */
    if(!duplicate) {
      LOG_INFO("received packet from ");
      LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
      LOG_INFO_(", seqno %u, len %u\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO), packetbuf_datalen());
      //EXTRA BEGIN
      //NETSTACK_NETWORK.input();
      iotorii_operation();
      //EXTRA END
    }
  }
}
/*---------------------------------------------------------------------------*/
const struct mac_driver iotorii_csma_driver = {  //const struct mac_driver csma_driver = { //EXTRA
  "IoTorii CSMA",
  init,
  send_packet,
  input_packet,
  on,
  off,
  max_payload,
};
/*---------------------------------------------------------------------------*/
