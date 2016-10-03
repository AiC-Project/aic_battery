#include <cutils/properties.h>
#include <sys/socket.h>

#include "aic.h"
#include "read_battery_proto.h"

// Protobuf
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "sensors_packet.pb.h"


using namespace google::protobuf::io;

// Read the framing
uint32_t read_header(const char* buf)
{
  google::protobuf::uint32 size;
  google::protobuf::io::ArrayInputStream ais(buf, 4);
  CodedInputStream coded_input(&ais);
  coded_input.ReadVarint32(&size);
  ALOGD("read_header: expected size of payload is %d", size);
  return size;
}

// Read and parse the message
void read_body(int csock, uint32_t size, char* level, char* full, char* status, char* online)
{
    int bytecount;
    sensors_packet payload;
    uint32_t full_size = size + 4;
    char* buffer = new char[full_size];
    if((bytecount = recv(csock, (void*)buffer, full_size, MSG_WAITALL)) == -1)
    {
        ALOGE("Error receiving data (%d)", errno);
        delete[] buffer;
        return;
    }
    else if (bytecount != full_size)
    {
        ALOGE("Error: expected %d bytes, received %d", full_size, bytecount);
        delete[] buffer;
        return;
    }
    google::protobuf::io::ArrayInputStream ais(buffer, full_size);
    CodedInputStream coded_input(&ais);
    coded_input.ReadVarint32(&size);
    google::protobuf::io::CodedInputStream::Limit msgLimit = coded_input.PushLimit(size);
    payload.ParseFromCodedStream(&coded_input);
    coded_input.PopLimit(msgLimit);

    if (payload.has_battery() ){
        snprintf(full, NB_ELEM, "%d", payload.battery().battery_full());
        snprintf(level, NB_ELEM, "%d", payload.battery().battery_level());
        snprintf(online, NB_ELEM, "%d", payload.battery().ac_online());

        switch(payload.battery().battery_status())
        {
            case sensors_packet_BatteryPayload_BatteryStatusType_CHARGING :
                snprintf(status, NB_ELEM, "Charging");
                break;
            case sensors_packet_BatteryPayload_BatteryStatusType_DISCHARGING :
                snprintf(status, NB_ELEM, "Discharging");
                break;
            case sensors_packet_BatteryPayload_BatteryStatusType_NOTCHARGING :
                snprintf(status, NB_ELEM, "Not charging");
                break;
            case sensors_packet_BatteryPayload_BatteryStatusType_FULL :
                snprintf(status, NB_ELEM, "Full");
                break;
            case sensors_packet_BatteryPayload_BatteryStatusType_UNKNOWN :
                snprintf(status, NB_ELEM, "Unknown");
                break;
            default:
                snprintf(status, NB_ELEM, "Unknown");
                break;
        }
        ALOGI("Sensor data unpacked: %s %s %s %s",  level, full, status, online);
    }
    else
    {
        ALOGE("Incorrect message");
    }
    delete[] buffer;
}
