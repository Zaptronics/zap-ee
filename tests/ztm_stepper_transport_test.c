#include <assert.h>
#include <string.h>
#include "zaptronics/modules/ztm_stepper.h"

typedef struct {
    bool active;
    uint8_t frames[160][4];
    size_t frame_count;
} fake_spi_t;

static int f_begin(void *ctx, uint32_t timeout_us)
{
    (void)timeout_us;
    fake_spi_t *f = ctx;
    if (!f || f->active) return ZAP_SPI_E_STATE;
    f->active = true;
    return ZAP_SPI_OK;
}
static int f_frame(void *ctx, const uint8_t *tx, uint8_t *rx, size_t len,
                   uint8_t fill, uint32_t timeout_us)
{
    (void)timeout_us;
    fake_spi_t *f = ctx;
    if (!f || !f->active || len != 4u || f->frame_count >= 160u) return ZAP_SPI_E_STATE;
    for (size_t i=0;i<4u;i++) f->frames[f->frame_count][i] = tx ? tx[i] : fill;
    if (rx) {
        /* Physical response order farthest->nearest: driver 4,3,2,1. */
        rx[0]=4u; rx[1]=3u; rx[2]=2u; rx[3]=1u;
    }
    f->frame_count++;
    return ZAP_SPI_OK;
}
static int f_end(void *ctx)
{
    fake_spi_t *f = ctx;
    if (!f || !f->active) return ZAP_SPI_E_STATE;
    f->active = false;
    return ZAP_SPI_OK;
}
static void delay_ms(void *ctx, uint32_t ms) { (void)ctx; (void)ms; }
static int power(void *ctx, bool enabled) { (void)ctx; (void)enabled; return 0; }

int main(void)
{
    fake_spi_t f = {0};
    const zap_spi_framed_device_ops_t ops = { f_begin, f_frame, f_end };
    const zap_spi_framed_device_t spi = { &f, &ops, 5000u, 0u };
    const zap_time_t time = { NULL, delay_ms, NULL, NULL };
    const ztm_stepper_config_t cfg = { .spi=&spi, .time=&time, .lock=NULL,
                                       .control_context=NULL, .power_set=power,
                                       .event_context=NULL, .event=NULL };
    ztm_stepper_t m;
    assert(ztm_stepper_init(&m, &cfg) == 0);
    assert(ztm_stepper_ready(&m));

    f.frame_count = 0u;
    assert(ztm_stepper_run_all(&m, true, 200.0f));
    assert(f.frame_count == 4u);
    for (size_t i=0;i<4u;i++) {
        assert(f.frames[i][0] == L6470_CMD_NOP); /* physical driver 4 */
        assert(f.frames[i][1] == f.frames[i][2]);
        assert(f.frames[i][2] == f.frames[i][3]);
    }

    f.frame_count = 0u;
    assert(ztm_stepper_run(&m, 1u, false, 100.0f));
    assert(f.frame_count == 4u);
    for (size_t i=0;i<4u;i++) {
        assert(f.frames[i][0] == 0u && f.frames[i][1] == 0u && f.frames[i][2] == 0u);
    }
    assert(f.frames[0][3] == L6470_CMD_RUN);

    f.frame_count = 0u;
    assert(!ztm_stepper_move(&m, ZTM_STEPPER_UNUSED_DRIVER_ID, true, 100u));
    assert(f.frame_count == 0u);

    uint8_t responses[4] = {0};
    assert(ztm_stepper_nop(&m, responses));
    assert(responses[0] == 1u && responses[1] == 2u && responses[2] == 3u && responses[3] == 4u);
    return 0;
}
