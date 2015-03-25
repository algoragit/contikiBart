#include "contiki.h"
#include "random.h"
#include <stdio.h>
#include "net/rime/rime.h"

/*---------------------------------------------------------------------------*/
PROCESS(muchmac_test_process, "muchmac-test");
AUTOSTART_PROCESSES(&muchmac_test_process);

static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("unicast message received from %d.%d\n",
	 from->u8[0], from->u8[1]);
}
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(muchmac_test_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);

  static struct etimer et;

  // node 2 sends to node 1
  // node 3 sends to node 2
  if ((linkaddr_node_addr.u8[0] == 2) || (linkaddr_node_addr.u8[0] == 3)) {
    while(1) {
      etimer_set(&et, 5 * CLOCK_SECOND + random_rand() % (15 * CLOCK_SECOND));
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

      linkaddr_t dest;
      dest.u8[1] = 0;
      if (linkaddr_node_addr.u8[0] == 2)
        dest.u8[0] = 1;
      else
        dest.u8[0] = 2;

      printf("send unicast message to %d.%d\n", dest.u8[0], dest.u8[1]);
      packetbuf_copyfrom("Hello", 5);
      unicast_send(&uc, &dest);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
