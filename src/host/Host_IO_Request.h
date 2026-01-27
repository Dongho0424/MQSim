#ifndef HOST_IO_REQUEST_H
#define HOST_IO_REQUEST_H

#include "../ssd/SSD_Defs.h"

namespace Host_Components {
enum class Host_IO_Request_Type { READ, WRITE };
class Host_IO_Request {
 public:
  sim_time_type Arrival_time;  // The time that the request has been generated, from trace file
  sim_time_type Enqueue_time;  // The time that the request enqueued into the I/O queue
  LHA_type Start_LBA;
  unsigned int LBA_count;  // IO req가 접근하는 LBA의 개수, req의 size
  Host_IO_Request_Type Type;
  uint16_t IO_queue_info;   // for NVMe model, req에 할당된 command_id를 저장
  uint16_t Source_flow_id;  // Only used in SATA host interface
};
}  // namespace Host_Components

#endif  // !HOST_IO_REQUEST_H
