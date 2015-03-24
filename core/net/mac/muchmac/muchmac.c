#include "muchmac.h"
#include "net/netstack.h"
#include "net/packetbuf.h"

static void
send(mac_callback_t sent_callback, void *ptr)
{

}

static void
send_list(mac_callback_t sent_callback, void *ptr, struct rdc_buf_list *list)
{

}

static void
input(void)
{

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
