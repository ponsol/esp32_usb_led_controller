#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led_strip.h"



uint8_t  colr=250, colg=10, colb=0;

static char *TAG = "mydev_usb";


#define USBD_STACK_SIZE     4096
#define RW_STACK_SIZE      2*configMINIMAL_STACK_SIZE

enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED     = 1000,
  BLINK_SUSPENDED   = 2500,

  BLINK_ALWAYS_ON   = UINT32_MAX,
  BLINK_ALWAYS_OFF  = 0
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;


void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}


bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{

  printf("control mesg %d\n", request->bRequest);

  if (stage != CONTROL_STAGE_SETUP) return true;


  switch (request->bmRequestType_bit.type)
  {
    case TUSB_REQ_TYPE_VENDOR:

      switch (request->bRequest)
      {
        case VENDOR_REQUEST_WEBUSB:
          int val ;
          return tud_control_xfer(rhport, request, (void*)(uintptr_t) &val, sizeof(int));
        break;


        default: break;
      }
    break;

    case TUSB_REQ_TYPE_CLASS:
        return tud_control_status(rhport, request);
    break;

    default: break;
  }

  return false;
}




void usb_device_task(void *params) 
{
  (void) params ;

  tud_init(BOARD_TUD_RHPORT);
  while (1)
  {
    tud_task();
    //write flush here 
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

}




void read_task(void *params) 
{

  (void) params;
   uint8_t buf[64];

  while ( 1 )
  {
      // There are data available
      {
	int n = 4 ;
        memset(buf, 0, 64);
        uint32_t count = tud_vendor_read(buf, n);
        // read and echo back

         if ( count > 0 ) {
              colr = buf[0]; colg = buf[1]; colb = buf[2];
   	      printf("read: \n");
	      for ( int i =0 ; i < count ; i++ ) printf("%02X \n", buf[i] );
         }
      }

    vTaskDelay(1);
  }

}



void write_task(void *params) 
{

  (void) params;
   uint8_t buf[64];

  while ( 1 )
  {
      // There are data available
      {
	memset(buf, 0x0, 64);
        uint32_t count = 4 ;
	memset(buf,0, 64);
        buf[0] = colr; buf[1] = colg; buf[2] = colb ;
	tud_vendor_write(buf, count);
        tud_vendor_write_flush();

      }

    vTaskDelay(1);
  }

}

void tud_vendor_rx_cb(uint8_t itf)
{
  (void) itf;
}


#define BLINK_GPIO 38
static led_strip_handle_t led_strip;

static void configure_led(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, 
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
}



void led_task(void *params)  
{

    configure_led();

    while (1) {

        led_strip_set_pixel(led_strip, 0, colr, colg, colb);
        led_strip_refresh(led_strip);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


int app_main(void)
{
    board_init();
    xTaskCreate( usb_device_task, "usbd_task", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate( read_task, "read_task", RW_STACK_SIZE, NULL, configMAX_PRIORITIES-2, NULL);
    xTaskCreate( write_task, "write_task", RW_STACK_SIZE, NULL, configMAX_PRIORITIES-2, NULL);
    xTaskCreate( led_task, "led_task", RW_STACK_SIZE, NULL, configMAX_PRIORITIES-2, NULL);
    ESP_LOGI(TAG,"\n");
    ESP_LOGI(TAG, "Device Initialized\n");


    return 0;
}


