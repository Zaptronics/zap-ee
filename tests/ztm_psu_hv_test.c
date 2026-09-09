#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "zaptronics/modules/ztm_psu_hv.h"

typedef struct {
    uint8_t rdac[2];
    bool prepared;
    bool enabled;
    bool pgood;
    uint16_t adc_code;
    uint32_t drive_uV;
    uint32_t delayed_ms;
    ztm_psu_hv_event_type_t last_event;
    unsigned event_count;
} fake_t;

static int i2c_write(void *context, uint8_t address, const uint8_t *data,
                     size_t len, uint32_t timeout_us)
{
    (void)address; (void)timeout_us;
    fake_t *fake = context;
    assert(data != NULL && len == 2u);
    fake->rdac[(data[0] & 0x80u) != 0u ? 1u : 0u] = data[1];
    return 0;
}
static int i2c_write_read(void *context, uint8_t address,
                          const uint8_t *write_data, size_t write_len,
                          uint8_t *read_data, size_t read_len,
                          uint32_t timeout_us)
{
    (void)address; (void)timeout_us;
    fake_t *fake = context;
    assert(write_data != NULL && write_len == 1u);
    assert(read_data != NULL && read_len == 1u);
    read_data[0] = fake->rdac[(write_data[0] & 0x80u) != 0u ? 1u : 0u];
    return 0;
}
static int i2c_probe(void *context, uint8_t address, uint32_t timeout_us)
{ (void)context; (void)address; (void)timeout_us; return 0; }
static int prepare(void *context)
{ fake_t *fake=context; fake->prepared=true; fake->enabled=false; return 0; }
static int enable_set(void *context, bool enabled)
{ ((fake_t *)context)->enabled=enabled; return 0; }
static int enable_get(void *context, bool *enabled)
{ *enabled=((fake_t *)context)->enabled; return 0; }
static int pgood_read(void *context, bool *pgood)
{ *pgood=((fake_t *)context)->pgood; return 0; }
static int vsense_read(void *context, uint16_t *code, uint32_t *uV)
{ fake_t *fake=context; if (code) *code=fake->adc_code; *uV=fake->drive_uV; return 0; }
static void delay_ms(void *context, uint32_t ms)
{ ((fake_t *)context)->delayed_ms += ms; }
static void event(void *context, const ztm_psu_hv_event_t *ev)
{ fake_t *fake=context; fake->last_event=ev->type; fake->event_count++; }

int main(void)
{
    fake_t fake = {
        .pgood = true,
        .adc_code = 1234u,
        .drive_uV = 8000000u,
    };
    const zap_i2c_bus_ops_t i2c_ops = {
        .write=i2c_write, .write_read=i2c_write_read, .probe=i2c_probe,
    };
    const zap_i2c_bus_t i2c = { .context=&fake, .ops=&i2c_ops, .timeout_us=5000u };
    const zap_time_t time = { .context=&fake, .delay_ms=delay_ms };
    const ztm_psu_hv_config_t config = {
        .i2c=&i2c, .time=&time, .lock=NULL, .address_straps=0u,
        .control_context=&fake, .prepare=prepare,
        .enable_set=enable_set, .enable_get=enable_get,
        .pgood_read=pgood_read, .vsense_read=vsense_read,
        .event_context=&fake, .event=event,
    };

    ztm_psu_hv_t dev = {0};
    assert(ztm_psu_hv_init(&dev, &config) == 0);
    assert(fake.prepared && !fake.enabled);
    assert(fake.rdac[0] == 255u && fake.rdac[1] == 255u);
    assert(ztm_psu_hv_ready(&dev));
    assert(fake.event_count == 1u && fake.last_event == ZTM_PSU_HV_EVENT_READY);

    assert(ztm_psu_hv_stage_code(&dev, 100u));
    assert(!fake.enabled && fake.rdac[0] == 0u && fake.rdac[1] == 100u);

    assert(!ztm_psu_hv_start(&dev, 54u, 20u));
    assert(!fake.enabled);
    assert(fake.rdac[0] == 255u && fake.rdac[1] == 255u);
    assert(dev.last_operation_stage == ZTM_PSU_HV_OP_VALIDATE_CODE);

    assert(ztm_psu_hv_start(&dev, 100u, 20u));
    assert(fake.enabled);
    assert(dev.drive_uV == 8000000u);

    fake.pgood = false;
    assert(ztm_psu_hv_poll(&dev) == 0);
    assert(!fake.enabled);
    assert(fake.last_event == ZTM_PSU_HV_EVENT_PGOOD_FAULT);
    assert(fake.rdac[0] == 255u && fake.rdac[1] == 255u);

    fake.pgood = true;
    fake.drive_uV = 8000000u;
    assert(ztm_psu_hv_start(&dev, 100u, 20u));
    fake.drive_uV = 16000000u;
    assert(ztm_psu_hv_poll(&dev) == 0);
    assert(!fake.enabled);
    assert(fake.last_event == ZTM_PSU_HV_EVENT_E60_OVERVOLTAGE);

    uint32_t drive = 0u;
    assert(ztm_psu_hv_calibrated_drive_for_code(55u, &drive));
    assert(drive == 13695600u);
    assert(!ztm_psu_hv_e60_input_within_rating(15000001u));
    return 0;
}
