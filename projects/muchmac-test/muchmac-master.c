#include "contiki.h"
#include "net/rime/timesynch.h"

/*---------------------------------------------------------------------------*/
PROCESS(muchmac_master_process, "muchmac-master");
AUTOSTART_PROCESSES(&muchmac_master_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(muchmac_master_process, ev, data)
{
  PROCESS_BEGIN();

  timesynch_set_authority_level(0);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
