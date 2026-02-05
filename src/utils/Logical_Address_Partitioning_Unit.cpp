#include "Logical_Address_Partitioning_Unit.h"

namespace Utils {
int**** Logical_Address_Partitioning_Unit::resource_list;
std::vector<std::vector<flash_channel_ID_type>> Logical_Address_Partitioning_Unit::stream_channel_ids;
std::vector<std::vector<flash_chip_ID_type>> Logical_Address_Partitioning_Unit::stream_chip_ids;
std::vector<std::vector<flash_die_ID_type>> Logical_Address_Partitioning_Unit::stream_die_ids;
std::vector<std::vector<flash_plane_ID_type>> Logical_Address_Partitioning_Unit::stream_plane_ids;
HostInterface_Types Logical_Address_Partitioning_Unit::hostinterface_type;
bool Logical_Address_Partitioning_Unit::initialized = false;
std::vector<LHA_type> Logical_Address_Partitioning_Unit::pdas_per_flow;
std::vector<LHA_type> Logical_Address_Partitioning_Unit::start_lhas_per_flow;
std::vector<LHA_type> Logical_Address_Partitioning_Unit::end_lhas_per_flow;
unsigned int Logical_Address_Partitioning_Unit::channel_count;
unsigned int Logical_Address_Partitioning_Unit::chip_no_per_channel;
unsigned int Logical_Address_Partitioning_Unit::die_no_per_chip;
unsigned int Logical_Address_Partitioning_Unit::plane_no_per_die;
LHA_type Logical_Address_Partitioning_Unit::total_pda_no = 0; // in sectors
LHA_type Logical_Address_Partitioning_Unit::total_lha_no = 0; // in sectors

void Logical_Address_Partitioning_Unit::Reset() {
  initialized = false;
  pdas_per_flow.clear();
  start_lhas_per_flow.clear();
  end_lhas_per_flow.clear();
  stream_channel_ids.clear();
  stream_chip_ids.clear();
  stream_die_ids.clear();
  stream_plane_ids.clear();

  for (flash_channel_ID_type channel_id = 0; channel_id < channel_count; channel_id++) {
    for (flash_chip_ID_type chip_id = 0; chip_id < chip_no_per_channel; chip_id++) {
      for (flash_die_ID_type die_id = 0; die_id < die_no_per_chip; die_id++) {
        delete[] resource_list[channel_id][chip_id][die_id];
      }
      delete[] resource_list[channel_id][chip_id];
    }
    delete[] resource_list[channel_id];
  }

  delete[] resource_list;
}

void Logical_Address_Partitioning_Unit::Allocate_logical_address_for_flows(
    HostInterface_Types hostinterface_type, unsigned int io_flows_cnt, unsigned int channel_count,
    unsigned int chip_no_per_channel, unsigned int die_no_per_chip, unsigned int plane_no_per_die,
    std::vector<std::vector<flash_channel_ID_type>> stream_channel_ids,
    std::vector<std::vector<flash_chip_ID_type>> stream_chip_ids,
    std::vector<std::vector<flash_die_ID_type>> stream_die_ids,
    std::vector<std::vector<flash_plane_ID_type>> stream_plane_ids, unsigned int block_no_per_plane,
    unsigned int page_no_per_block, unsigned int sector_no_per_page, double overprovisioning_ratio) {
  if (initialized) {
    return;
  }

  Logical_Address_Partitioning_Unit::hostinterface_type = hostinterface_type;
  Logical_Address_Partitioning_Unit::channel_count = channel_count;
  Logical_Address_Partitioning_Unit::chip_no_per_channel = chip_no_per_channel;
  Logical_Address_Partitioning_Unit::die_no_per_chip = die_no_per_chip;
  Logical_Address_Partitioning_Unit::plane_no_per_die = plane_no_per_die;

  resource_list = new int***[channel_count];
  bool resource_sharing = false;
  for (unsigned int C = 0; C < channel_count; C++) {
    resource_list[C] = new int**[chip_no_per_channel];
    for (unsigned int W = 0; W < chip_no_per_channel; W++) {
      resource_list[C][W] = new int*[die_no_per_chip];
      for (unsigned int D = 0; D < die_no_per_chip; D++) {
        resource_list[C][W][D] = new int[plane_no_per_die];
        for (unsigned int P = 0; P < plane_no_per_die; P++) {
          resource_list[C][W][D][P] = 0;
        }
      }
    }
  }
  
  // 여러 io_flow가 공유하는 resource list
  for (unsigned int s = 0; s < io_flows_cnt; s++) { // s: stream_id, or io_flow_id
    for (const auto& C : stream_channel_ids[s]) {
      for (const auto& W : stream_chip_ids[s]) {
        for (const auto& D : stream_die_ids[s]) {
          for (const auto& P : stream_plane_ids[s]) {
            if (C >= channel_count) PRINT_ERROR("Invalid channel ID specified for I/O flow " << s);
            if (W >= chip_no_per_channel) PRINT_ERROR("Invalid chip ID specified for I/O flow " << s);
            if (D >= die_no_per_chip) PRINT_ERROR("Invalid die ID specified for I/O flow " << s);
            if (P >= plane_no_per_die) PRINT_ERROR("Invalid plane ID specified for I/O flow " << s);
            
            resource_list[C][W][D][P]++;
          }
        }
      }
    }
  }

  // class member var에 복사
  Logical_Address_Partitioning_Unit::stream_channel_ids = stream_channel_ids;
  Logical_Address_Partitioning_Unit::stream_chip_ids = stream_chip_ids;
  Logical_Address_Partitioning_Unit::stream_die_ids = stream_die_ids;
  Logical_Address_Partitioning_Unit::stream_plane_ids = stream_plane_ids;

  // LSA (logical, host's view), PDA (device's view) counting
  const double usable_ratio = 1.0 - overprovisioning_ratio;
  const double sectors_per_plane = static_cast<double>(block_no_per_plane * page_no_per_block * sector_no_per_page);
  const double logical_sectors_per_plane = sectors_per_plane * usable_ratio;
  
  std::vector<LHA_type> lsa_count_per_stream;
  lsa_count_per_stream.reserve(io_flows_cnt);
  for (unsigned int s = 0; s < io_flows_cnt; s++) {

    double lsa_count = 0.0;
    LHA_type pda_count = 0;
    for (const auto& C : stream_channel_ids[s]) {
      for (const auto& W : stream_chip_ids[s]) {
        for (const auto& D : stream_die_ids[s]) {
          for (const auto& P : stream_plane_ids[s]) {
            double sharing_factor = static_cast<double>(resource_list[C][W][D][P]);
            // LHA count per io_flow. overprovisioning_ratio 곱한 만큼, host에게는 적게 보여야 함.
            // 만약 2개 이상의 io_flow가 같은 resource를 사용한다면 그만큼 lsa count가 낮아진다는 것을 반영
            lsa_count += logical_sectors_per_plane / sharing_factor;
            
            // PDA는 device's view이므로 overprovisioning_ratio를 고려하지 않아야한다.
            pda_count += sectors_per_plane / sharing_factor;
          }
        }
      }
    }
    lsa_count_per_stream.push_back(static_cast<LHA_type>(lsa_count));
    pdas_per_flow.push_back(pda_count);
    total_pda_no += pda_count;
  }

  // 각 Flow별 LHA 시작/종료 주소 할당 (LBA Partitioning)
  // io_flow 마다 LBA를 아예 겹치지 않게 나눠 가진다.
  // io_flow 0: 0 ~ max0
  // io_flow 1: max0 ~ max1
  // ...
  total_lha_no = 0;
  for (unsigned int s = 0; s < io_flows_cnt; s++) {
    start_lhas_per_flow.push_back(total_lha_no);
    end_lhas_per_flow.push_back(total_lha_no + lsa_count_per_stream[s] - 1);
    total_lha_no += lsa_count_per_stream[s];
  }

  initialized = true;
}

double Logical_Address_Partitioning_Unit::Get_share_of_physcial_pages_in_plane(flash_channel_ID_type channel_id,
                                                                               flash_chip_ID_type chip_id,
                                                                               flash_die_ID_type die_id,
                                                                               flash_plane_ID_type plane_id) {
  switch (hostinterface_type) {
    case HostInterface_Types::NVME:
      return 1.0 / double(resource_list[channel_id][chip_id][die_id][plane_id]);
    case HostInterface_Types::SATA:
    default:
      break;
  }

  return 1.0;
}

LHA_type Logical_Address_Partitioning_Unit::Start_lha_available_to_flow(stream_id_type stream_id) {
  if (initialized) {
    switch (hostinterface_type) {
      case HostInterface_Types::SATA:
        return start_lhas_per_flow[stream_id];
        break;
      case HostInterface_Types::NVME:
        return start_lhas_per_flow[stream_id];
        break;
      default:
        break;
    }
  }

  PRINT_ERROR("The address partitioning unit is not initialized!")
}

LHA_type Logical_Address_Partitioning_Unit::End_lha_available_to_flow(stream_id_type stream_id) {
  if (initialized) {
    switch (hostinterface_type) {
      case HostInterface_Types::SATA:
        return end_lhas_per_flow[stream_id];
        break;
      case HostInterface_Types::NVME:
        return end_lhas_per_flow[stream_id];
        break;
      default:
        break;
    }
  }
  PRINT_ERROR("The address partitioning unit is not initialized!")
}

LHA_type Logical_Address_Partitioning_Unit::LHA_count_allocate_to_flow_from_host_view(stream_id_type stream_id) {
  if (initialized) {
    switch (hostinterface_type) {
      case HostInterface_Types::SATA:
        return (end_lhas_per_flow[stream_id] - start_lhas_per_flow[stream_id] + 1);
        break;
      case HostInterface_Types::NVME:
        return (end_lhas_per_flow[stream_id] - start_lhas_per_flow[stream_id] + 1);
        break;
      default:
        break;
    }
  }
  PRINT_ERROR("The address partitioning unit is not initialized!")
}

LHA_type Logical_Address_Partitioning_Unit::LHA_count_allocate_to_flow_from_device_view(stream_id_type stream_id) {
  if (initialized) {
    switch (hostinterface_type) {
      // It is not possible to differentiate between streams inside the device when a SATA host interface is used, so
      // all lha space is available to all streams
      case HostInterface_Types::SATA:
        return total_lha_no;
        break;
      case HostInterface_Types::NVME:
        return (end_lhas_per_flow[stream_id] - start_lhas_per_flow[stream_id] + 1);
        break;
      default:
        break;
    }
  }
  PRINT_ERROR("The address partitioning unit is not initialized!")
}

PDA_type Logical_Address_Partitioning_Unit::PDA_count_allocate_to_flow(stream_id_type stream_id) {
  if (initialized) {
    switch (hostinterface_type) {
      // It is not possible to differentiate between streams inside the device when a SATA host interface is used, so
      // all lha space is available to all streams
      case HostInterface_Types::SATA:
        return total_pda_no;
      case HostInterface_Types::NVME:
        return pdas_per_flow[stream_id];
        break;
      default:
        break;
    }
  }
  PRINT_ERROR("The address partitioning unit is not initialized!")
}

LHA_type Logical_Address_Partitioning_Unit::Get_total_device_lha_count() { return total_lha_no; }
}  // namespace Utils
