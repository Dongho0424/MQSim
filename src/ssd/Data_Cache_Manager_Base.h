#ifndef DATA_CACHE_MANAGER_BASE_H
#define DATA_CACHE_MANAGER_BASE_H

#include <vector>

#include "../sim/Sim_Object.h"
#include "../utils/Workload_Statistics.h"
#include "Host_Interface_Base.h"
#include "NVM_Firmware.h"
#include "NVM_PHY_ONFI.h"
#include "User_Request.h"

namespace SSD_Components {
class NVM_Firmware;
class Host_Interface_Base;
enum class Caching_Mode { WRITE_CACHE, READ_CACHE, WRITE_READ_CACHE, TURNED_OFF };
enum class Caching_Mechanism { SIMPLE, ADVANCED };
// How the cache space is shared among the concurrently running I/O
// flows/streams
enum class Cache_Sharing_Mode {
  SHARED,  // each application has access to the entire cache space
  EQUAL_PARTITIONING
};
class Data_Cache_Manager_Base : public MQSimEngine::Sim_Object {
  friend class Data_Cache_Manager_Flash_Advanced;
  friend class Data_Cache_Manager_Flash_Simple;

 public:
  Data_Cache_Manager_Base(const sim_object_id_type& id, Host_Interface_Base* host_interface, NVM_Firmware* nvm_firmware,
                          unsigned int dram_row_size, unsigned int dram_data_rate, unsigned int dram_busrt_size,
                          sim_time_type dram_tRCD, sim_time_type dram_tCL, sim_time_type dram_tRP,
                          Caching_Mode* caching_mode_per_input_stream, Cache_Sharing_Mode sharing_mode,
                          unsigned int stream_count);
  virtual ~Data_Cache_Manager_Base();
  void Setup_triggers();
  void Start_simulation();
  void Validate_simulation_config();

  // type def UserRequestServicedSignalHanderType as func pointer: (User_Request*) -> Void
  typedef void (*UserRequestServicedSignalHanderType)(User_Request*);
  void Connect_to_user_request_serviced_signal(UserRequestServicedSignalHanderType);
  typedef void (*MemoryTransactionServicedSignalHanderType)(NVM_Transaction*);
  void Connect_to_user_memory_transaction_serviced_signal(MemoryTransactionServicedSignalHanderType);

  void Set_host_interface(Host_Interface_Base* host_interface);
  virtual void Do_warmup(std::vector<Utils::Workload_Statistics*> workload_stats) = 0;

 protected:
  static Data_Cache_Manager_Base* _my_instance;
  Host_Interface_Base* host_interface;
  NVM_Firmware* nvm_firmware;
  unsigned int dram_row_size;   // The size of the DRAM rows in bytes
  unsigned int dram_data_rate;  // in MT/s
  unsigned int dram_busrt_size;
  double dram_burst_transfer_time_ddr;  // The transfer time of two bursts,
                                        // changed from sim_time_type to double
                                        // to increase precision
  // DRAM access parameters in nano-seconds
  sim_time_type dram_tRCD, dram_tCL, dram_tRP;
  Cache_Sharing_Mode sharing_mode;
  static Caching_Mode* caching_mode_per_input_stream;
  unsigned int stream_count;  // io_flows.size()

  std::vector<UserRequestServicedSignalHanderType> connected_user_request_serviced_signal_handlers;
  void broadcast_user_request_serviced_signal(User_Request* user_request);

  std::vector<MemoryTransactionServicedSignalHanderType> connected_user_memory_transaction_serviced_signal_handlers;
  void broadcast_user_memory_transaction_serviced_signal(NVM_Transaction* transaction);

  static void handle_user_request_arrived_signal(User_Request* user_request);
  virtual void process_new_user_request(User_Request* user_request) = 0;

  bool is_user_request_finished(const User_Request* user_request) {
    return (user_request->Transaction_list.size() == 0 && user_request->Sectors_serviced_from_cache == 0);
  }
};

// inline sim_time_type estimate_dram_access_time(const unsigned int memory_access_size_in_byte,
//                                                const unsigned int dram_row_size,
//                                                const unsigned int dram_burst_size_in_bytes,
//                                                const double dram_burst_transfer_time_ddr, const sim_time_type tRCD,
//                                                const sim_time_type tCL, const sim_time_type tRP) {
//   if (memory_access_size_in_byte <= dram_row_size) {
//     return (sim_time_type)(tRCD + tCL +
//                            sim_time_type((double)(memory_access_size_in_byte / dram_burst_size_in_bytes / 2) *
//                                          dram_burst_transfer_time_ddr));
//   } else {
//     return (sim_time_type)(
//       (tRCD + tCL + (sim_time_type)((double)(dram_row_size / dram_burst_size_in_bytes / 2 *
//       dram_burst_transfer_time_ddr) + tRP) * (double)(memory_access_size_in_byte / dram_row_size / 2))
//       +
//       tRCD + tCL + (sim_time_type)((double)(memory_access_size_in_byte % dram_row_size) /
//       ((double)dram_burst_size_in_bytes * dram_burst_transfer_time_ddr)));
//   }
// }

inline sim_time_type estimate_dram_access_time(const unsigned int memory_access_size_in_byte,
                                               const unsigned int dram_row_size,
                                               const unsigned int dram_burst_size_in_bytes,
                                               const double dram_burst_transfer_time_ddr, const sim_time_type tRCD,
                                               const sim_time_type tCL, const sim_time_type tRP) {
  // 1. 공통적으로 사용되는 단위 시간 계산 (DDR을 고려한 버스트 전송 시간)
  // 기존 코드의 'size / burst_size / 2 * transfer_time' 로직을 반영
  auto calculate_burst_delay = [&](unsigned int size_in_bytes) -> sim_time_type {
    double num_bursts = static_cast<double>(size_in_bytes) / dram_burst_size_in_bytes;
    return static_cast<sim_time_type>(num_bursts / 2.0 * dram_burst_transfer_time_ddr);
  };

  const sim_time_type activation_time = tRCD + tCL;

  // Case 1: 단일 Row 내에서 액세스가 끝나는 경우
  if (memory_access_size_in_byte <= dram_row_size) {
    return activation_time + calculate_burst_delay(memory_access_size_in_byte);
  }

  // Case 2: 여러 Row에 걸쳐 액세스가 발생하는 경우
  const unsigned int full_rows = memory_access_size_in_byte / dram_row_size;
  const unsigned int remaining_bytes = memory_access_size_in_byte % dram_row_size;

  // 한 Row를 완전히 액세스하고 닫는(tRP) 데 걸리는 총 시간
  const sim_time_type full_row_access_cycle = activation_time + calculate_burst_delay(dram_row_size) + tRP;

  // 전체 시간 = (전체 Row 반복 시간) + (마지막 Row 활성화 및 남은 데이터 전송 시간)
  sim_time_type total_delay = static_cast<sim_time_type>(full_row_access_cycle * (full_rows / 2.0));
  total_delay += activation_time + calculate_burst_delay(remaining_bytes);

  return total_delay;
}
}  // namespace SSD_Components

#endif  // !DATA_CACHE_MANAGER_BASE_H
