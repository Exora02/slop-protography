#include "bend.h"

#include <esp_camera.h>

#include "camera_ctl.h"
#include "log.h"

static const char* TAG = "bend";

static uint32_t xs32(uint32_t* s) {
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

uint32_t bend_apply(uint32_t seed) {
    sensor_t* s = esp_camera_sensor_get();
    if (!s) return seed;
    if (seed == 0) seed = (uint32_t)esp_random();
    uint32_t rng = seed;

    // Manual analog chaos: gain and exposure the AE would never pick.
    s->set_gain_ctrl(s, 0);
    s->set_exposure_ctrl(s, 0);
    s->set_agc_gain(s, 30 + xs32(&rng) % 90);        // lots of amplifier noise
    s->set_aec_value(s, 40 + xs32(&rng) % 1100);     // long or starved exposure
    s->set_gainceiling(s, (gainceiling_t)(1 + xs32(&rng) % 6));

    // Wrong white balance for color shifts.
    s->set_whitebal(s, 0);
    s->set_awb_gain(s, 1);
    s->set_wb_mode(s, xs32(&rng) % 5);               // auto/sunny/cloudy/office/home

    // Switch the digital crutches OFF: sparkle noise and vignetting appear.
    s->set_denoise(s, 0);
    s->set_bpc(s, 0);
    s->set_wpc(s, 0);
    s->set_lenc(s, 0);

    // Occasional on-sensor special effect (negative / sepia / solarize-ish).
    if (xs32(&rng) % 100 < 35) {
        s->set_special_effect(s, 1 + xs32(&rng) % 6);
    } else {
        s->set_special_effect(s, 0);
    }

    // Exaggerated color/contrast/sharpness response.
    s->set_saturation(s, (int)(xs32(&rng) % 7) - 3);
    s->set_contrast(s, (int)(xs32(&rng) % 5) - 2);
    s->set_sharpness(s, (int)(xs32(&rng) % 7) - 3);

    LOGI(TAG, "sensor bent, seed %u", seed);
    return seed;
}

void bend_restore() {
    cam_apply_base();
    LOGI(TAG, "sensor restored");
}
