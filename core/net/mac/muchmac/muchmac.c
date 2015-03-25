#include "muchmac.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include <stdio.h>
#include "net/rime/timesynch.h"
#include "dev/leds.h"
#include "lib/memb.h"
#include "lib/list.h"

#define ON_PERIOD	RTIMER_SECOND/2

static struct ctimer ct;
static struct pt pt;
static struct rtimer rt;

// queue
struct muchmac_queue_item {
  struct muchmac_queue_item *next;
  struct queuebuf *buf;
  mac_callback_t sent_callback;
  void *sent_ptr;
};

#define MAX_QUEUE_SIZE	4
MEMB(muchmac_queue_memb, struct muchmac_queue_item, MAX_QUEUE_SIZE);
LIST(muchmac_queue);

PROCESS(send_packet, "muchmac send process");

static volatile unsigned char radio_is_on = 0;

static int
turn_on()
{
  leds_on(LEDS_BLUE);
  radio_is_on = 1;
  return NETSTACK_RADIO.on();
}

static int
turn_off()
{
  leds_off(LEDS_BLUE);
  radio_is_on = 0;
  return NETSTACK_RADIO.off();
}

static char
powercycle(struct rtimer *t, void *ptr)
{
  PT_BEGIN(&pt);

  while (1) {
    // slot start
    turn_on();
    if (list_tail(muchmac_queue) != NULL) {
      process_poll(&send_packet);
    }
    rtimer_set(t, RTIMER_NOW() + ON_PERIOD, 0, (rtimer_callback_t)powercycle, NULL);
    PT_YIELD(&pt);

    // slot end
    turn_off();
    rtimer_set(t, timesynch_time_to_rtimer(RTIMER_SECOND), 0, (rtimer_callback_t)powercycle, NULL);
    PT_YIELD(&pt);

    // slot start
    turn_on();
    rtimer_set(t, RTIMER_NOW() + ON_PERIOD, 0, (rtimer_callback_t)powercycle, NULL);
    PT_YIELD(&pt);

    // slot end
    turn_off();
    rtimer_set(t, timesynch_time_to_rtimer(0), 0, (rtimer_callback_t)powercycle, NULL);
    PT_YIELD(&pt);
  }

  PT_END(&pt);
}


#define WAIT_FOR_SEND	RTIMER_SECOND/10

PROCESS_THREAD(send_packet, ev, data)
{
  PROCESS_BEGIN();

  while (1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    if (list_tail(muchmac_queue) != NULL) {
      // busy wait to account for clock drift
      rtimer_clock_t t0 = RTIMER_NOW();
      while (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + WAIT_FOR_SEND)) ;

      struct muchmac_queue_item *item = list_tail(muchmac_queue);
      queuebuf_to_packetbuf(item->buf);

      printf("muchmac: send packet %p from queue\n", item->buf);

      int ret;
      packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);

      if (NETSTACK_FRAMER.create_and_secure() < 0) {
        ret = MAC_TX_ERR_FATAL;
      } else {
        switch (NETSTACK_RADIO.send(packetbuf_hdrptr(), packetbuf_totlen())) {
        case RADIO_TX_OK:
          ret = MAC_TX_OK;
          break;
        case RADIO_TX_COLLISION:
          ret = MAC_TX_COLLISION;
          break;
        case RADIO_TX_NOACK:
          ret = MAC_TX_NOACK;
          break;
        default:
          ret = MAC_TX_ERR;
          break;
        }
      }

      mac_call_sent_callback(item->sent_callback, item->sent_ptr, ret, 1);

      list_remove(muchmac_queue, item);
      memb_free(&muchmac_queue_memb, item);
    }
  }

  PROCESS_END();
}

static void
send(mac_callback_t sent_callback, void *ptr)
{
  int ret;

  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);

  if (NETSTACK_FRAMER.create_and_secure() < 0) {
    ret = MAC_TX_ERR_FATAL;
  } else {
    switch (NETSTACK_RADIO.send(packetbuf_hdrptr(), packetbuf_totlen())) {
    case RADIO_TX_OK:
      ret = MAC_TX_OK;
      break;
    case RADIO_TX_COLLISION:
      ret = MAC_TX_COLLISION;
      break;
    case RADIO_TX_NOACK:
      ret = MAC_TX_NOACK;
      break;
    default:
      ret = MAC_TX_ERR;
      break;
    }
  }

  mac_call_sent_callback(sent_callback, ptr, ret, 1);
}

static void
send_list(mac_callback_t sent_callback, void *ptr, struct rdc_buf_list *list)
{
  if (list != NULL) {
    if (radio_is_on) {
      queuebuf_to_packetbuf(list->buf);
      send(sent_callback, ptr);
    } else {
      struct muchmac_queue_item* item;
      item = memb_alloc(&muchmac_queue_memb);
      if (item == NULL) {
        mac_call_sent_callback(sent_callback, ptr, MAC_TX_ERR_FATAL, 1);
      } else {
        item->buf = list->buf;
        item->sent_callback = sent_callback;
        item->sent_ptr = ptr;
        list_push(muchmac_queue, item);
        printf("packet %p queued\n", item->buf);
      }
    }
  }
}

static void
input(void)
{
  if (NETSTACK_FRAMER.parse() >= 0) {
    if (linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &linkaddr_node_addr) ||
        packetbuf_holds_broadcast()) {
      NETSTACK_MAC.input();
    }
  }
}

static int
on(void)
{
  return turn_on();
}

static int
off(int keep_radio_on)
{
  if (keep_radio_on) {
    return turn_on();
  } else {
    return turn_off();
  }
}

static unsigned short
channel_check_interval(void)
{
  return CLOCK_SECOND;
}

static void
start_tdma(void *ptr)
{
  printf("muchmac: starting TDMA mode\n");
  rtimer_set(&rt, timesynch_time_to_rtimer(0), 0, (rtimer_callback_t)powercycle, NULL);
}

static void
init(void)
{
  on();

  PT_INIT(&pt);

  memb_init(&muchmac_queue_memb);
  list_init(muchmac_queue);

  process_start(&send_packet, NULL);

  // schedule start of TDMA mode
  ctimer_set(&ct, 120*CLOCK_SECOND, start_tdma, NULL);
}

const struct rdc_driver muchmac_driver = {
  .name = "muchmac",
  .init = init,
  .send = send,
  .send_list = send_list,
  .input = input,
  .on = on,
  .off = off,
  .channel_check_interval = channel_check_interval
};
