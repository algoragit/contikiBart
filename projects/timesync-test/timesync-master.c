#include "contiki.h"
#include "net/rime/timesynch.h"

/*---------------------------------------------------------------------------*/
PROCESS(timesync_master_process, "timesync-master");
AUTOSTART_PROCESSES(&timesync_master_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(timesync_master_process, ev, data)
{
  PROCESS_BEGIN();

  timesynch_set_authority_level(0);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
