//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#if defined(HAVE_HIDAPI) 

#include "log.hpp"
#include "device_io_hid.hpp"

namespace hw {
  namespace io {
 
    #undef MONERO_DEFAULT_LOG_CATEGORY
    #define MONERO_DEFAULT_LOG_CATEGORY "device.io"
 
    #define ASSERT_X(exp,msg)    CHECK_AND_ASSERT_THROW_MES(exp, msg); 

    #define MAX_BLOCK  64
    
    static std::string safe_hid_error(hid_device *hwdev) {
      if (hwdev) {
        return  std::string((char*)hid_error(hwdev));
      }
      return std::string("NULL device");
    }

    static std::string safe_hid_path(const hid_device_info *hwdev_info) {
      if (hwdev_info && hwdev_info->path) {
        return  std::string(hwdev_info->path);
      }
      return std::string("NULL path");
    }

    device_io_hid::device_io_hid(unsigned short c, unsigned char t, unsigned int ps, unsigned int to) : 
      channel(c), 
      tag(t), 
      packet_size(ps), 
      timeout(to),
      usb_vid(0),
      usb_pid(0),
      usb_device(NULL) {
    }

    device_io_hid::device_io_hid() : device_io_hid(DEFAULT_CHANNEL, DEFAULT_TAG, DEFAULT_PACKET_SIZE, DEFAULT_TIMEOUT) {
    }

    void device_io_hid::io_hid_log(int read, unsigned char* buffer, int block_len) {
      if (hid_verbose) {
        char  strbuffer[1024];
        hw::buffer_to_str(strbuffer, sizeof(strbuffer), (char*)buffer, block_len);
        MDEBUG( "HID " << (read?"<":">") <<" : "<<strbuffer);
      }
    }
 
    void device_io_hid::init() {
      int r;
      r = hid_init();
      ASSERT_X(r>=0, "Unable to init hidapi library. Error "+std::to_string(r)+": "+safe_hid_error(this->usb_device));
    }

    void device_io_hid::connect(void *params) {
      hid_conn_params *p = (struct hid_conn_params*)params;
      this->connect(p->vid, p->pid, p->interface_number, p->usage_page, p->interface_OR_page);
    }

    void device_io_hid::connect(unsigned int vid, unsigned  int pid, int interface_number, unsigned short usage_page, bool interface_OR_page ) {
      hid_device_info *hwdev_info, *hwdev_info_list;
      hid_device      *hwdev;

      this->disconnect();

      hwdev_info_list = hid_enumerate(vid, pid);
      ASSERT_X(hwdev_info_list, "Unable to enumerate device "+std::to_string(vid)+":"+std::to_string(vid)+  ": "+ safe_hid_error(this->usb_device));
      hwdev = NULL;
      hwdev_info = hwdev_info_list;
      while (hwdev_info) {
        if ((interface_OR_page && ((hwdev_info->usage_page == usage_page) || (hwdev_info->interface_number == interface_number))) ||
                                  ((hwdev_info->usage_page == usage_page) && (hwdev_info->interface_number == interface_number))) {
          MDEBUG("HID Device found: " << safe_hid_path(hwdev_info));
          hwdev = hid_open_path(hwdev_info->path);
          break;
        } else {
          MDEBUG("HID Device discard: " << safe_hid_path(hwdev_info) << "("+std::to_string(hwdev_info->usage_page) << "," << std::to_string(hwdev_info->interface_number) << ")");
        }
        hwdev_info = hwdev_info->next;
      }
      hid_free_enumeration(hwdev_info_list);
      ASSERT_X(hwdev, "Unable to open device "+std::to_string(pid)+":"+std::to_string(vid));
      this->usb_vid = vid;
      this->usb_pid = pid;
      this->usb_device = hwdev;
    }


    bool device_io_hid::connected() const {
      return this->usb_device != NULL;
    }

    int device_io_hid::exchange(unsigned char *command, unsigned int cmd_len, unsigned char *response, unsigned int max_resp_len)  {
      unsigned char buffer[400];
      unsigned char padding_buffer[MAX_BLOCK+1];
      unsigned int  result;
               int  hid_ret;
      unsigned int  sw_offset;
      unsigned int  remaining;
      unsigned int  offset = 0;

      ASSERT_X(this->usb_device,"No device opened");

      //Split command in several HID packet
      memset(buffer, 0, sizeof(buffer));
      result = this->wrapCommand(command, cmd_len, buffer, sizeof(buffer));
      remaining = result;

      while (remaining > 0) {
        int block_size = (remaining > MAX_BLOCK ? MAX_BLOCK : remaining);
        memset(padding_buffer, 0, sizeof(padding_buffer));
        memcpy(padding_buffer+1, buffer + offset, block_size);
        io_hid_log(0, padding_buffer, block_size+1);
        hid_ret = hid_write(this->usb_device, padding_buffer, block_size+1);
        ASSERT_X(hid_ret>=0, "Unable to send hidapi command. Error "+std::to_string(result)+": "+ safe_hid_error(this->usb_device));
        offset += block_size;
        remaining -= block_size;
      }

      //get first response
      memset(buffer, 0, sizeof(buffer));
      hid_ret = hid_read_timeout(this->usb_device, buffer, MAX_BLOCK, this->timeout);
      ASSERT_X(hid_ret>=0, "Unable to read hidapi response. Error "+std::to_string(result)+": "+ safe_hid_error(this->usb_device));
      result = (unsigned int)hid_ret;
      io_hid_log(1, buffer, result); 
      offset = MAX_BLOCK;
      //parse first response and get others if any
      for (;;) {
        result = this->unwrapReponse(buffer, offset, response, max_resp_len);
        if (result != 0) {
          break;
        }
        hid_ret = hid_read_timeout(this->usb_device, buffer + offset, MAX_BLOCK, this->timeout);
        ASSERT_X(hid_ret>=0, "Unable to receive hidapi response. Error "+std::to_string(result)+": "+ safe_hid_error(this->usb_device));
        result = (unsigned int)hid_ret;
        io_hid_log(1, buffer + offset, result);
        offset += MAX_BLOCK;
      }      
      return result;
    }

    void device_io_hid::disconnect(void)  {
      if (this->usb_device) {
        hid_close(this->usb_device);
      }
      this->usb_vid = 0;
      this->usb_pid = 0;
      this->usb_device = NULL;
    }

    void device_io_hid::release()  {
      /* Do not exit, as the lib context is global*/
      //hid_exit();
    }

    unsigned int device_io_hid::wrapCommand(const unsigned char *command, size_t command_len, unsigned char *out, size_t out_len) {
      unsigned int sequence_idx = 0;
      unsigned int offset = 0;
      unsigned int offset_out = 0;
      unsigned int block_size;

      ASSERT_X(this->packet_size >= 3, "Invalid Packet size: "+std::to_string(this->packet_size)) ;
      ASSERT_X(out_len >= 7,  "out_len too short: "+std::to_string(out_len));

      out_len -= 7;
      out[offset_out++] = ((this->channel >> 8) & 0xff);
      out[offset_out++] = (this->channel & 0xff);
      out[offset_out++] = this->tag;
      out[offset_out++] = ((sequence_idx >> 8) & 0xff);
      out[offset_out++] = (sequence_idx & 0xff);
      sequence_idx++;
      out[offset_out++] = ((command_len >> 8) & 0xff);
      out[offset_out++] = (command_len & 0xff);
      block_size = (command_len > this->packet_size - 7 ? this->packet_size - 7 : command_len);
      ASSERT_X(out_len >= block_size,  "out_len too short: "+std::to_string(out_len));
      out_len -= block_size;
      memcpy(out + offset_out, command + offset, block_size);
      offset_out += block_size;
      offset += block_size;
      while (offset != command_len) {
        ASSERT_X(out_len >= 5,  "out_len too short: "+std::to_string(out_len));
        out_len -= 5;
        out[offset_out++] = ((this->channel >> 8) & 0xff);
        out[offset_out++] = (this->channel & 0xff);
        out[offset_out++] = this->tag;
        out[offset_out++] = ((sequence_idx >> 8) & 0xff);
        out[offset_out++] = (sequence_idx & 0xff);
        sequence_idx++;
        block_size = ((command_len - offset) > this->packet_size - 5 ? this->packet_size - 5 : command_len - offset);
        ASSERT_X(out_len >= block_size,  "out_len too short: "+std::to_string(out_len));
        out_len -= block_size;
        memcpy(out + offset_out, command + offset, block_size);
        offset_out += block_size;
        offset += block_size;
      }
      while ((offset_out % this->packet_size) != 0) {
        ASSERT_X(out_len >= 1,  "out_len too short: "+std::to_string(out_len));
        out_len--;
        out[offset_out++] = 0;
      }
      return offset_out;
    }

    /*
     * return  0 if more data are needed
    *         >0 if response is fully available
     */
    unsigned int device_io_hid::unwrapReponse(const unsigned char *data, size_t data_len, unsigned char *out, size_t out_len) {
      unsigned int sequence_idx = 0;
      unsigned int offset = 0;
      unsigned int offset_out = 0;
      unsigned int response_len;
      unsigned int block_size;
      unsigned int val;

      //end?
      if ((data == NULL) || (data_len < 7 + 5)) { 
        return 0;
      }

      //check hid header
      val = (data[offset]<<8) + data[offset+1];
      offset += 2;
      ASSERT_X(val == this->channel, "Wrong Channel");
      val =  data[offset];
      offset++;
      ASSERT_X(val == this->tag, "Wrong TAG");      
      val = (data[offset]<<8) + data[offset+1];
      offset += 2;
      ASSERT_X(val == sequence_idx, "Wrong sequence_idx");

      //fetch
      response_len = (data[offset++] << 8);
      response_len |= data[offset++];
      ASSERT_X(out_len >= response_len, "Out Buffer too short");
      if (data_len < (7 + response_len)) {
        return 0;
      }
      block_size = (response_len > (this->packet_size - 7) ? this->packet_size - 7 : response_len);
      memcpy(out + offset_out, data + offset, block_size);
      offset += block_size;
      offset_out += block_size;
      while (offset_out != response_len) {
        sequence_idx++;
        if (offset == data_len) {
          return 0;
        }
        val = (data[offset]<<8) + data[offset+1];
        offset += 2;
        ASSERT_X(val == this->channel, "Wrong Channel");
        val =  data[offset];
        offset++;
        ASSERT_X(val == this->tag, "Wrong TAG");      
        val = (data[offset]<<8) + data[offset+1];
        offset += 2;
        ASSERT_X(val == sequence_idx, "Wrong sequence_idx");

        block_size = ((response_len - offset_out) > this->packet_size - 5 ? this->packet_size - 5 : response_len - offset_out);
        if (block_size > (data_len - offset)) {
          return 0;
        }
        memcpy(out + offset_out, data + offset, block_size);
        offset += block_size;
        offset_out += block_size;
      }
      return offset_out;
    }


  }
}

#endif //#if defined(HAVE_HIDAPI) 
