#include "contiki.h"
#include "dev/leds.h"
#include "rime/timesynch.h"

#define SYNCH_PERIOD	60 * CLOCK_SECOND
#define LEDS_ON_PERIOD	RTIMER_SECOND/10

static struct rtimer rt;
static struct pt pt;

static char
rtimer_task(struct rtimer *t, void *ptr)
{
  PT_BEGIN(&pt);

  while (1) {
    leds_on(LEDS_ALL);
    rtimer_set(t, RTIMER_NOW() + LEDS_ON_PERIOD, 0, (rtimer_callback_t)rtimer_task, NULL);
    PT_YIELD(&pt);

    leds_off(LEDS_ALL);
    rtimer_set(t, timesynch_time_to_rtimer(0), 0, (rtimer_callback_t)rtimer_task, NULL);
    PT_YIELD(&pt);
  }

  PT_END(&pt);
}

/*---------------------------------------------------------------------------*/
PROCESS(timesync_test_process, "timesync-test");
AUTOSTART_PROCESSES(&timesync_test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(timesync_test_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  PT_INIT(&pt);

  // wait until network is synchronized
  etimer_set(&et, SYNCH_PERIOD);
  PROCESS_WAIT_UNTIL(etimer_expired(&et));

  rtimer_set(&rt, timesynch_time_to_rtimer(0), 0, (rtimer_callback_t)rtimer_task, NULL);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
