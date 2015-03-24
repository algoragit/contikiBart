#include "muchmac.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"

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
    queuebuf_to_packetbuf(list->buf);
    send(sent_callback, ptr);
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
  return NETSTACK_RADIO.on();
}

static int
off(int keep_radio_on)
{
  if (keep_radio_on) {
    return NETSTACK_RADIO.on();
  } else {
    return NETSTACK_RADIO.off();
  }
}

static unsigned short
channel_check_interval(void)
{
  return 0;
}

static void
init(void)
{
  on();
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
