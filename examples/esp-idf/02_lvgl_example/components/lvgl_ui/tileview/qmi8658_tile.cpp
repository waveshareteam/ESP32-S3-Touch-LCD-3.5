#include "qmi8658_tile.h"
#include "esp_qmi8658_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static lv_obj_t *list;

lv_obj_t *label_accel_x;
lv_obj_t *label_accel_y;
lv_obj_t *label_accel_z;

lv_obj_t *label_gyro_x;
lv_obj_t *label_gyro_y;
lv_obj_t *label_gyro_z;
lv_obj_t *label_imu_temp;



static void qmi8658_time_cb(lv_timer_t *timer)
{
    IMUdata acc;
    IMUdata gyr;
    char str[20];
    if (qmi.getDataReady()) {

        // printf("Timestamp:");
        // printf(qmi.getTimestamp());

        if (qmi.getAccelerometer(acc.x, acc.y, acc.z)) {

            // Print to serial plotter
            // printf("ACCEL.x:"); printf(acc.x); printf(",");
            // printf("ACCEL.y:"); printf(acc.y); printf(",");
            // printf("ACCEL.z:\r\n"); printf(acc.z); printfln();

            sprintf(str, "%.2f mg", acc.x * 1000);
            lv_label_set_text(label_accel_x, str);

            sprintf(str, "%.2f mg", acc.y * 1000);
            lv_label_set_text(label_accel_y, str);

            sprintf(str, "%.2f mg", acc.z * 1000);
            lv_label_set_text(label_accel_z, str);

            /*
            m2/s to mg
            printf(" ACCEL.x:"); printf(acc.x * 1000); printfln(" mg");
            printf(",ACCEL.y:"); printf(acc.y * 1000); printfln(" mg");
            printf(",ACCEL.z:"); printf(acc.z * 1000); printfln(" mg");
            */

        }

        if (qmi.getGyroscope(gyr.x, gyr.y, gyr.z)) {


            // Print to serial plotter
            // printf("GYRO.x:"); printf(gyr.x); printf(",");
            // printf("GYRO.y:"); printf(gyr.y); printf(",");
            // printf("GYRO.z:\r\n"); printf(gyr.z); printfln();

            sprintf(str, "%.2f degrees/sec", gyr.x);
            lv_label_set_text(label_gyro_x, str);

            sprintf(str, "%.2f degrees/sec", gyr.y);
            lv_label_set_text(label_gyro_y, str);

            sprintf(str, "%.2f degrees/sec", gyr.z);
            lv_label_set_text(label_gyro_z, str);

            // printf(" GYRO.x:"); printf(gyr.x); printfln(" degrees/sec");
            // printf(",GYRO.y:"); printf(gyr.y); printfln(" degrees/sec");
            // printf(",GYRO.z:"); printf(gyr.z); printfln(" degrees/sec");

        }
        sprintf(str, "%.2f degrees C", qmi.getTemperature_C());
        lv_label_set_text(label_imu_temp, str);
        // printf("Temperature:");
        // printf(qmi.getTemperature_C());
        // printfln(" degrees C");

    }
}


void qmi8658_tile_init(lv_obj_t *parent)
{
    /*Create a list*/
    list = lv_list_create(parent);
    lv_obj_t *lable = lv_label_create(parent);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_label_set_text(lable, "QMI8658");
    lv_obj_align(lable, LV_ALIGN_TOP_MID, 0, 3);

    lv_obj_set_size(list, lv_pct(95), lv_pct(90));
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *list_item;

    list_item = lv_list_add_btn(list, NULL, "Accel_x");
    label_accel_x = lv_label_create(list_item);
    lv_label_set_text(label_accel_x, "----");

    list_item = lv_list_add_btn(list, NULL, "Accel_y");
    label_accel_y = lv_label_create(list_item);
    lv_label_set_text(label_accel_y, "----");

    list_item = lv_list_add_btn(list, NULL, "Accel_z");
    label_accel_z = lv_label_create(list_item);
    lv_label_set_text(label_accel_z, "----");

    list_item = lv_list_add_btn(list, NULL, "Gyro_x");
    label_gyro_x = lv_label_create(list_item);
    lv_label_set_text(label_gyro_x, "----");

    list_item = lv_list_add_btn(list, NULL, "Gyro_y");
    label_gyro_y = lv_label_create(list_item);
    lv_label_set_text(label_gyro_y, "----");

    list_item = lv_list_add_btn(list, NULL, "Gyro_z");
    label_gyro_z = lv_label_create(list_item);
    lv_label_set_text(label_gyro_z, "----");

    list_item = lv_list_add_btn(list, NULL, "IMU_Temp");
    label_imu_temp = lv_label_create(list_item);
    lv_label_set_text(label_imu_temp, "--- C");
    lv_timer_create(qmi8658_time_cb, 100, NULL);
}