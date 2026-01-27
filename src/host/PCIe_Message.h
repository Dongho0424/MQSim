#ifndef PCIE_MESSAGE_H
#define PCIE_MESSAGE_H

#include <cstdint>

namespace Host_Components {
enum class PCIe_Destination_Type { HOST, DEVICE };
enum class PCIe_Message_Type {
  READ_REQ,   // read_request; Host <- SSD
  WRITE_REQ,  // write_request; both Host <-> SSD
  READ_COMP   // read completion; Host -> SSD
};

class PCIe_Message {
 public:
  PCIe_Destination_Type Destination;
  PCIe_Message_Type Type;
  void* Payload;              // data object, either Submission_Queue_Entry or Completion_Queue_Entry
  unsigned int Payload_size;  // size in bytes, sizeof(Submission_Queue_Entry).
  uint64_t Address;           // host memory address or device register address
};
}  // namespace Host_Components

#endif  //! PCIE_MESSAGE_H
