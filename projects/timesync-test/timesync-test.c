#include "contiki.h"

/*---------------------------------------------------------------------------*/
PROCESS(timesync_test_process, "timesync-test");
AUTOSTART_PROCESSES(&timesync_test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(timesync_test_process, ev, data)
{
  PROCESS_BEGIN();

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
