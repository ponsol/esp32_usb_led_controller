#include "tusb.h"
#include "usb_descriptors.h"

tusb_desc_device_t const desc_device =   {
        .bLength = sizeof(desc_device),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200, // USB version. 0x0200 means version 2.0
        .bDeviceClass = 0xFF, //TUSB_CLASS_UNSPECIFIED,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
        .idVendor = 0x303A,
        .idProduct = 0x4008,
        .bcdDevice = 0x10,     // Device FW version
        .iManufacturer = 0x01, // see string_descriptor[1] bellow
        .iProduct = 0x02,      // see string_descriptor[2] bellow
        .iSerialNumber = 0x03, // see string_descriptor[3] bellow
        .bNumConfigurations = 0x01
};


uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}


uint8_t const desc_configuration[] = {
	0x09,	         //.bLength 
        0x02,            //.bDescriptorType
	0x20, 0x00,      //.wTotalLength
        0x01,            //.bNumInterfaces 
        0x01,	         //.bConfigurationValue
	0x00,	         //.iConfiguration  //string_desc_index
	0xC0,            //.bmAttributes
	0x32,            //.bMaxPower
        // Interface 0 //
        0x09,          // bLength //
        0x04,          // bDescriptorType // must be 0x04 -> config
        0x00,          // bInterfaceNumber //
        0x00,          // bAlternateSetting //
        0x02,          // bNumEndpoints //
        0xFF,          // bInterfaceClass //
        0x00,          // bInterfaceSubClass //
        0x00,          // bInterfaceProtocol //
        0x04,          // iInterface //  string_desccrip_index
        // EP 0 //
	0x07,          //bLength
        0x05,          // bDescriptorType
        0x81,          // bEndpointAddress: in/number 1000 0001
        0x06,          // bmAttributes Data/Async/Bulk: 00!00!01!10 
	8,0x00,     // wMaxPacketSize
        0x04,          // bInterval		       		      
        // EP 1 //
	0x07,          //bLength
        0x05,          // bDescriptorType
        0x02,          // bEndpointAddress  //out
        0x06,          // bmAttributes
	8,0x00,     // wMaxPacketSize
        0x04,          // bInterval		       		      
};

void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes) {
   printf("in cb sent bytes %ld for itf %02X\n", sent_bytes, itf );
}


uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index; // for multiple configurations
  return desc_configuration;
}


//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+


// array of pointer to string descriptors
char const* string_desc_arr [] = {
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "TinyUSB",             // 1: Manufacturer
    "TinyUSB Device",      // 2: Product
    "FABCDE",              // 3: Serials, should use chip ID
    "MYDEV USB device",    // 4: Mydev
    "TinyUSB Newdev",      // 5: Vendor Interface
};


static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;

  uint8_t chr_count;

  if ( index == 0)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }else
  {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

    const char* str = string_desc_arr[index];

    // Cap at max char
    chr_count = (uint8_t) strlen(str);
    if ( chr_count > 31 ) chr_count = 31;

    // Convert ASCII string into UTF-16
    for(uint8_t i=0; i<chr_count; i++)
    {
      _desc_str[1+i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8 ) | (2*chr_count + 2));

  return _desc_str;
}


