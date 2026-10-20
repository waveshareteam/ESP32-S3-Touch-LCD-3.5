/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>

/*To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 *You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 Note that the `lv_examples` library is for LVGL v7 and you shouldn't install it for this version (since LVGL v8)
 as the examples and demos are now part of the main LVGL library. */

// #include <examples/lv_examples.h>
// #include <demos/lv_demos.h>

// #define DIRECT_MODE // Uncomment to enable full frame buffer

#include <Arduino_GFX_Library.h>
#include "TCA9554.h"
#include "TouchDrvFT6X36.hpp"
#include "SensorQMI8658.hpp"

#define GFX_BL 6  // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

#define SPI_MISO 2
#define SPI_MOSI 1
#define SPI_SCLK 5

#define LCD_CS -1
#define LCD_DC 3
#define LCD_RST -1
#define LCD_HOR_RES 320
#define LCD_VER_RES 480

#define I2C_SDA 8
#define I2C_SCL 7

TCA9554 TCA(0x20);
SensorQMI8658 qmi;

IMUdata acc;
IMUdata gyr;


Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC /* DC */, LCD_CS /* CS */, SPI_SCLK /* SCK */, SPI_MOSI /* MOSI */, SPI_MISO /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7796(
  bus, LCD_RST /* RST */, 0 /* rotation */, true, LCD_HOR_RES, LCD_VER_RES);

TouchDrvFT6X36 touch;

uint32_t screenWidth;
uint32_t screenHeight;
uint32_t bufSize;
lv_disp_draw_buf_t draw_buf;
lv_color_t *disp_draw_buf1;
lv_color_t *disp_draw_buf2;
lv_disp_drv_t disp_drv;

void lvgl_qmi8658_ui_init(lv_obj_t *parent);

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
#ifndef DIRECT_MODE
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
#endif  // #ifndef DIRECT_MODE

  lv_disp_flush_ready(disp_drv);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  int16_t x[1], y[1];
  uint8_t touched = touch.getPoint(x, y, 1);

  if (touched) {
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates*/
    data->point.x = x[0];
    data->point.y = y[0];
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void lcd_reset(void) {
  TCA.write1(1, 1);
  delay(10);
  TCA.write1(1, 0);
  delay(10);
  TCA.write1(1, 1);
  delay(200);
}

void qmi8658_init(void) {
  if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS)) {
    while (1) {
      Serial.println("Failed to find QMI8658 - check your wiring!");
      delay(1000);
    }
  }
  /* Get chip id*/
  Serial.print("Device ID:");
  Serial.println(qmi.getChipID(), HEX);


  if (qmi.selfTestAccel()) {
    Serial.println("Accelerometer self-test successful");
  } else {
    Serial.println("Accelerometer self-test failed!");
  }

  if (qmi.selfTestGyro()) {
    Serial.println("Gyroscope self-test successful");
  } else {
    Serial.println("Gyroscope self-test failed!");
  }


  qmi.configAccelerometer(
    /*
         * ACC_RANGE_2G
         * ACC_RANGE_4G
         * ACC_RANGE_8G
         * ACC_RANGE_16G
         * */
    SensorQMI8658::ACC_RANGE_4G,
    /*
         * ACC_ODR_1000H
         * ACC_ODR_500Hz
         * ACC_ODR_250Hz
         * ACC_ODR_125Hz
         * ACC_ODR_62_5Hz
         * ACC_ODR_31_25Hz
         * ACC_ODR_LOWPOWER_128Hz
         * ACC_ODR_LOWPOWER_21Hz
         * ACC_ODR_LOWPOWER_11Hz
         * ACC_ODR_LOWPOWER_3H
        * */
    SensorQMI8658::ACC_ODR_1000Hz,
    /*
        *  LPF_MODE_0     //2.66% of ODR
        *  LPF_MODE_1     //3.63% of ODR
        *  LPF_MODE_2     //5.39% of ODR
        *  LPF_MODE_3     //13.37% of ODR
        *  LPF_OFF        // OFF Low-Pass Fitter
        * */
    SensorQMI8658::LPF_MODE_0);




  qmi.configGyroscope(
    /*
        * GYR_RANGE_16DPS
        * GYR_RANGE_32DPS
        * GYR_RANGE_64DPS
        * GYR_RANGE_128DPS
        * GYR_RANGE_256DPS
        * GYR_RANGE_512DPS
        * GYR_RANGE_1024DPS
        * */
    SensorQMI8658::GYR_RANGE_64DPS,
    /*
         * GYR_ODR_7174_4Hz
         * GYR_ODR_3587_2Hz
         * GYR_ODR_1793_6Hz
         * GYR_ODR_896_8Hz
         * GYR_ODR_448_4Hz
         * GYR_ODR_224_2Hz
         * GYR_ODR_112_1Hz
         * GYR_ODR_56_05Hz
         * GYR_ODR_28_025H
         * */
    SensorQMI8658::GYR_ODR_896_8Hz,
    /*
        *  LPF_MODE_0     //2.66% of ODR
        *  LPF_MODE_1     //3.63% of ODR
        *  LPF_MODE_2     //5.39% of ODR
        *  LPF_MODE_3     //13.37% of ODR
        *  LPF_OFF        // OFF Low-Pass Fitter
        * */
    SensorQMI8658::LPF_MODE_3);




  /*
    * If both the accelerometer and gyroscope sensors are turned on at the same time,
    * the output frequency will be based on the gyroscope output frequency.
    * The example configuration is 896.8HZ output frequency,
    * so the acceleration output frequency is also limited to 896.8HZ
    * */
  qmi.enableGyroscope();
  qmi.enableAccelerometer();

  // Print register configuration information
  qmi.dumpCtrlRegister();

  Serial.println("Read data now...");
}


void setup() {

  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  qmi8658_init();

  TCA.begin();
  TCA.pinMode1(1,OUTPUT);


  lcd_reset();
  Serial.println("Arduino_GFX Hello World example");

  if (!touch.begin(Wire, FT6X36_SLAVE_ADDRESS)) {
    Serial.println("Failed to find FT6X36 - check your wiring!");
    while (1) {
      delay(1000);
    }
  }
  // Init Display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }

  gfx->fillScreen(RGB565_BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  lv_init();

  screenWidth = gfx->width();
  screenHeight = gfx->height();

  bufSize = screenWidth * 120;

  disp_draw_buf1 = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_8BIT);

  disp_draw_buf2 = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_8BIT);

  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf1, disp_draw_buf2, bufSize);

  /* Initialize the display */
  lv_disp_drv_init(&disp_drv);
  /* Change the following line to your display resolution */
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;

  lv_disp_drv_register(&disp_drv);

  /* Initialize the (dummy) input device driver */
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  lvgl_qmi8658_ui_init(lv_scr_act());

  /* Option 3: Or try out a demo. Don't forget to enable the demos in lv_conf.h. E.g. LV_USE_DEMOS_WIDGETS*/
  // lv_demo_widgets();
  // lv_demo_benchmark();
  // lv_demo_keypad_encoder();
  // lv_demo_music();
  // lv_demo_stress();

  Serial.println("Setup done");
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */
  delay(1);
}


lv_obj_t *label_accel_x;
lv_obj_t *label_accel_y;
lv_obj_t *label_accel_z;
lv_obj_t *label_gyro_x;
lv_obj_t *label_gyro_y;
lv_obj_t *label_gyro_z;
lv_obj_t *label_temp;

lv_timer_t *qmi8658_timer = NULL;


static void qmi8658_callback(lv_timer_t *timer) {
  char str_buffer[20];
  if (qmi.getDataReady()) {
    if (qmi.getAccelerometer(acc.x, acc.y, acc.z)) {
      snprintf(str_buffer, sizeof(str_buffer), "%.2f", acc.x);
      lv_label_set_text(label_accel_x, str_buffer);
      snprintf(str_buffer, sizeof(str_buffer), "%.2f", acc.y);
      lv_label_set_text(label_accel_y, str_buffer);
      snprintf(str_buffer, sizeof(str_buffer), "%.2f", acc.z);
      lv_label_set_text(label_accel_z, str_buffer);
    }

    if (qmi.getGyroscope(gyr.x, gyr.y, gyr.z)) {
      snprintf(str_buffer, sizeof(str_buffer), "%.2f", gyr.x);
      lv_label_set_text(label_gyro_x, str_buffer);
      snprintf(str_buffer, sizeof(str_buffer), "%.2f", gyr.y);
      lv_label_set_text(label_gyro_y, str_buffer);
      snprintf(str_buffer, sizeof(str_buffer), "%.2f", gyr.z);
      lv_label_set_text(label_gyro_z, str_buffer);
    }
    snprintf(str_buffer, sizeof(str_buffer), "%.2fC", qmi.getTemperature_C());
    lv_label_set_text(label_temp, str_buffer);
  }
}


void lvgl_qmi8658_ui_init(lv_obj_t *parent) {
  lv_obj_t *list = lv_list_create(parent);
  lv_obj_set_size(list, lv_pct(100), lv_pct(100));

  lv_obj_t *list_item = lv_list_add_btn(list, NULL, "accel_x");
  label_accel_x = lv_label_create(list_item);
  lv_label_set_text(label_accel_x, "0.00");

  list_item = lv_list_add_btn(list, NULL, "accel_y");
  label_accel_y = lv_label_create(list_item);
  lv_label_set_text(label_accel_y, "0.00");

  list_item = lv_list_add_btn(list, NULL, "accel_z");
  label_accel_z = lv_label_create(list_item);
  lv_label_set_text(label_accel_z, "0.00");

  list_item = lv_list_add_btn(list, NULL, "gyro_x");
  label_gyro_x = lv_label_create(list_item);
  lv_label_set_text(label_gyro_x, "0.00");

  list_item = lv_list_add_btn(list, NULL, "gyro_y");
  label_gyro_y = lv_label_create(list_item);
  lv_label_set_text(label_gyro_y, "0.00");

  list_item = lv_list_add_btn(list, NULL, "gyro_z");
  label_gyro_z = lv_label_create(list_item);
  lv_label_set_text(label_gyro_z, "0.00");

  list_item = lv_list_add_btn(list, NULL, "temp");
  label_temp = lv_label_create(list_item);
  lv_label_set_text(label_temp, "0.00C");

  qmi8658_timer = lv_timer_create(qmi8658_callback, 100, NULL);
}
