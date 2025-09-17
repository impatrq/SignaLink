#include "lvgl.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

static const char *TAG = "lvgl_lcd_ui";
static lv_obj_t *g_label = NULL;

void lvgl_lcd_ui(lv_disp_t *disp)
{
    if (!disp) return;

    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_clean(scr); /* elimina objetos previos / evita congelados */

    g_label = lv_label_create(scr);
    lv_label_set_long_mode(g_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(g_label, "Signalink a la ONIET"); /* texto inicial que quieres ver */
    /* Forzar ancho menor que el texto para activar el scroll */
    lv_obj_set_width(g_label, disp->driver->hor_res / 2);
    lv_obj_align(g_label, LV_ALIGN_TOP_MID, 0, 0);

    ESP_LOGI(TAG, "lvgl_lcd_ui: UI creada (label con texto)");
}

/* helper público si querés actualizar el label desde MQTT/otro task */
void lcd_set_text(const char *txt)
{
    if (!txt) return;
    if (!g_label) {
        ESP_LOGW(TAG, "lcd_set_text: label no inicializado");
        return;
    }
    if (lvgl_port_lock(0)) {
        lv_label_set_text(g_label, txt);
        lv_label_set_long_mode(g_label, LV_LABEL_LONG_SCROLL_CIRCULAR); /* reiniciar scroll */
        lvgl_port_unlock();
    } else {
        ESP_LOGW(TAG, "lcd_set_text: no se pudo tomar lock lvgl");
    }
}