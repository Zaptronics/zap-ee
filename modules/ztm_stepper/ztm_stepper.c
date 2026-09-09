// SPDX-License-Identifier: Apache-2.0
#include "zaptronics/modules/ztm_stepper.h"

#include <string.h>

static bool valid_id(uint8_t id) { return id >= 1u && id <= ZTM_STEPPER_DRIVER_COUNT; }
static bool active_id(uint8_t id) { return id >= 1u && id <= ZTM_STEPPER_ACTIVE_DRIVER_COUNT; }
static bool valid_microstep(ztm_stepper_microstep_t mode)
{
    return mode >= ZTM_STEPPER_MICROSTEP_FULL &&
           mode <= ZTM_STEPPER_MICROSTEP_ONE_TWENTY_EIGHTH;
}

static ztm_stepper_kval_config_t default_kval(void)
{
    ztm_stepper_kval_config_t v = {
        ZTM_STEPPER_DEFAULT_KVAL_HOLD, ZTM_STEPPER_DEFAULT_KVAL_RUN,
        ZTM_STEPPER_DEFAULT_KVAL_ACC, ZTM_STEPPER_DEFAULT_KVAL_DEC
    };
    return v;
}

static void emit(ztm_stepper_t *m, ztm_stepper_event_type_t type,
                 uint8_t id, uint16_t status)
{
    if (m != NULL && m->config.event != NULL) {
        ztm_stepper_event_t e = { type, id, status };
        m->config.event(m->config.event_context, &e);
    }
}

bool ztm_stepper_compose_frame(uint8_t device_id, uint8_t value,
                               uint8_t wire_frame[ZTM_STEPPER_DRIVER_COUNT])
{
    if (!valid_id(device_id) || wire_frame == NULL) return false;
    memset(wire_frame, L6470_CMD_NOP, ZTM_STEPPER_DRIVER_COUNT);
    wire_frame[ZTM_STEPPER_DRIVER_COUNT - device_id] = value;
    return true;
}

static bool ready(const ztm_stepper_t *m)
{
    return m != NULL && m->ready && zap_spi_framed_device_valid(m->config.spi);
}

bool ztm_stepper_ready(const ztm_stepper_t *m) { return ready(m); }

static bool seq_begin(ztm_stepper_t *m)
{
    return zap_spi_framed_sequence_begin(m->config.spi) == ZAP_SPI_OK;
}
static bool seq_end(ztm_stepper_t *m)
{
    return zap_spi_framed_sequence_end(m->config.spi) == ZAP_SPI_OK;
}
static bool raw_frame(ztm_stepper_t *m, const uint8_t tx[4], uint8_t rx[4])
{
    return zap_spi_framed_frame(m->config.spi, tx, rx, ZTM_STEPPER_DRIVER_COUNT) == ZAP_SPI_OK;
}

static bool target_sequence_locked(ztm_stepper_t *m, uint8_t id,
                                   const uint8_t *bytes, size_t count)
{
    if (!ready(m) || !valid_id(id) || bytes == NULL || count == 0u || !seq_begin(m)) return false;
    bool ok = true;
    for (size_t i = 0; i < count && ok; ++i) {
        uint8_t frame[4];
        (void)ztm_stepper_compose_frame(id, bytes[i], frame);
        ok = raw_frame(m, frame, NULL);
    }
    bool ended = seq_end(m);
    return ok && ended;
}

static bool target_read_locked(ztm_stepper_t *m, uint8_t id, uint8_t command,
                               uint8_t *data, size_t count)
{
    if (!ready(m) || !valid_id(id) || data == NULL || count == 0u || !seq_begin(m)) return false;
    uint8_t frame[4];
    (void)ztm_stepper_compose_frame(id, command, frame);
    bool ok = raw_frame(m, frame, NULL);
    const size_t wire_index = ZTM_STEPPER_DRIVER_COUNT - id;
    for (size_t i = 0; i < count && ok; ++i) {
        uint8_t tx[4] = {0};
        uint8_t rx[4] = {0};
        ok = raw_frame(m, tx, rx);
        if (ok) data[i] = rx[wire_index];
    }
    bool ended = seq_end(m);
    return ok && ended;
}

static bool broadcast_byte_locked(ztm_stepper_t *m, uint8_t value, bool active_only)
{
    if (!ready(m) || !seq_begin(m)) return false;
    uint8_t frame[4];
    for (size_t i = 0; i < 4u; ++i) frame[i] = value;
    if (active_only) frame[0] = L6470_CMD_NOP; /* driver 4 is farthest / first on wire */
    bool ok = raw_frame(m, frame, NULL);
    bool ended = seq_end(m);
    return ok && ended;
}

static bool command_locked(ztm_stepper_t *m, uint8_t id, uint8_t cmd)
{
    return target_sequence_locked(m, id, &cmd, 1u);
}

static bool set_param_locked(ztm_stepper_t *m, uint8_t id, uint8_t reg,
                             uint8_t nbytes, uint32_t value)
{
    uint8_t bytes[L6470_MAX_SEQUENCE_BYTES];
    size_t count = l6470_encode_set_param(reg, value, nbytes, bytes);
    return count != 0u && target_sequence_locked(m, id, bytes, count);
}

static bool get_param_locked(ztm_stepper_t *m, uint8_t id, uint8_t reg,
                             uint8_t nbytes, uint32_t *value)
{
    if (value == NULL || nbytes == 0u || nbytes > 4u) return false;
    uint8_t bytes[4] = {0};
    if (!target_read_locked(m, id, (uint8_t)(L6470_CMD_GET_PARAM | reg), bytes, nbytes)) return false;
    uint32_t v = 0u;
    for (uint8_t i = 0u; i < nbytes; ++i) v = (v << 8) | bytes[i];
    *value = v;
    return true;
}

static bool get_status_locked(ztm_stepper_t *m, uint8_t id, uint16_t *status)
{
    uint8_t bytes[2];
    if (status == NULL || !target_read_locked(m, id, L6470_CMD_GET_STATUS, bytes, 2u)) return false;
    *status = (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
    m->last_status[id - 1u] = *status;
    return true;
}

static bool apply_kval_locked(ztm_stepper_t *m, uint8_t id,
                              const ztm_stepper_kval_config_t *c)
{
    return c != NULL &&
           set_param_locked(m,id,L6470_REG_KVAL_HOLD,1u,c->hold) &&
           set_param_locked(m,id,L6470_REG_KVAL_RUN,1u,c->run) &&
           set_param_locked(m,id,L6470_REG_KVAL_ACC,1u,c->acc) &&
           set_param_locked(m,id,L6470_REG_KVAL_DEC,1u,c->dec);
}

static bool apply_config_locked(ztm_stepper_t *m, uint8_t id)
{
    const size_t i = id - 1u;
    return set_param_locked(m,id,L6470_REG_STEP_MODE,1u,(uint8_t)m->microstep_mode[i]) &&
           set_param_locked(m,id,L6470_REG_ACC,2u,l6470_acc_from_sps2(ZTM_STEPPER_DEFAULT_ACCEL_SPS2)) &&
           set_param_locked(m,id,L6470_REG_DEC,2u,l6470_acc_from_sps2(ZTM_STEPPER_DEFAULT_DECEL_SPS2)) &&
           set_param_locked(m,id,L6470_REG_MAX_SPEED,2u,l6470_max_speed_from_steps_per_s(ZTM_STEPPER_DEFAULT_MAX_SPEED_SPS)) &&
           apply_kval_locked(m,id,&m->kval_config[i]);
}

static bool apply_all_locked(ztm_stepper_t *m)
{
    for (uint8_t id = 1u; id <= 4u; ++id) if (!apply_config_locked(m,id)) return false;
    return command_locked(m, ZTM_STEPPER_UNUSED_DRIVER_ID, L6470_CMD_HARD_HIZ);
}

static bool lock_op(ztm_stepper_t *m) { return m != NULL && zap_lock_acquire(m->config.lock) == 0; }
static void unlock_op(ztm_stepper_t *m) { zap_lock_release(m->config.lock); }

int ztm_stepper_init(ztm_stepper_t *m, const ztm_stepper_config_t *c)
{
    if (m == NULL || c == NULL || !zap_spi_framed_device_valid(c->spi) ||
        !zap_time_valid(c->time) || !zap_lock_valid(c->lock)) return -1;
    memset(m, 0, sizeof(*m));
    m->config = *c;
    for (size_t i=0;i<4u;++i) { m->kval_config[i]=default_kval(); m->microstep_mode[i]=ZTM_STEPPER_DEFAULT_MICROSTEP; }
    if (m->config.power_set != NULL && m->config.power_set(m->config.control_context, false) != 0) return -2;
    zap_delay_ms(m->config.time, 1u);
    if (m->config.power_set != NULL && m->config.power_set(m->config.control_context, true) != 0) return -3;
    zap_delay_ms(m->config.time, 1u);
    m->ready = true;
    if (!lock_op(m)) { m->ready=false; return -4; }
    bool ok = broadcast_byte_locked(m, L6470_CMD_RESET_DEVICE, false);
    if (ok) { zap_delay_ms(m->config.time, 1u); ok = apply_all_locked(m); }
    unlock_op(m);
    if (!ok) { m->ready=false; if (m->config.power_set) (void)m->config.power_set(m->config.control_context,false); return -5; }
    return 0;
}

int ztm_stepper_deinit(ztm_stepper_t *m)
{
    if (m == NULL) return -1;
    if (m->ready && lock_op(m)) { (void)broadcast_byte_locked(m,L6470_CMD_HARD_HIZ,false); unlock_op(m); }
    int rc = 0;
    if (m->config.power_set != NULL && m->config.power_set(m->config.control_context,false) != 0) rc=-2;
    m->ready=false;
    return rc;
}

static void update_fault(ztm_stepper_t *m, size_t i, uint16_t s)
{
    bool stall=l6470_status_has_stall(s), warn=l6470_status_has_thermal_warning(s), shut=l6470_status_has_thermal_shutdown(s);
    if (stall != m->stall[i]) emit(m, stall?ZTM_STEPPER_EVENT_STALL_ASSERTED:ZTM_STEPPER_EVENT_STALL_CLEARED,(uint8_t)(i+1u),s);
    if (warn != m->thermal_warning[i]) emit(m,warn?ZTM_STEPPER_EVENT_THERMAL_WARNING_ASSERTED:ZTM_STEPPER_EVENT_THERMAL_WARNING_CLEARED,(uint8_t)(i+1u),s);
    if (shut != m->thermal_shutdown[i]) emit(m,shut?ZTM_STEPPER_EVENT_THERMAL_SHUTDOWN_ASSERTED:ZTM_STEPPER_EVENT_THERMAL_SHUTDOWN_CLEARED,(uint8_t)(i+1u),s);
    m->stall[i]=stall; m->thermal_warning[i]=warn; m->thermal_shutdown[i]=shut;
}

int ztm_stepper_poll(ztm_stepper_t *m)
{
    if (!ready(m)) return -1;
    if (zap_lock_try_acquire(m->config.lock) != 0) return 0;
    bool ok=true;
    for (uint8_t id=1u; id<=4u; ++id) { uint16_t s=0u; if (!get_status_locked(m,id,&s)) { ok=false; break; } update_fault(m,id-1u,s); }
    unlock_op(m);
    if (!ok) { emit(m,ZTM_STEPPER_EVENT_COMMUNICATION_ERROR,0u,0u); return -2; }
    return 0;
}

void ztm_stepper_get_snapshot(const ztm_stepper_t *m, ztm_stepper_status_t *s)
{
    if (m==NULL || s==NULL) return;
    s->ready=m->ready;
    memcpy(s->kval_config,m->kval_config,sizeof(s->kval_config));
    memcpy(s->microstep_mode,m->microstep_mode,sizeof(s->microstep_mode));
    memcpy(s->last_status,m->last_status,sizeof(s->last_status));
    memcpy(s->stall,m->stall,sizeof(s->stall));
    memcpy(s->thermal_warning,m->thermal_warning,sizeof(s->thermal_warning));
    memcpy(s->thermal_shutdown,m->thermal_shutdown,sizeof(s->thermal_shutdown));
}

bool ztm_stepper_nop(ztm_stepper_t *m, uint8_t responses[4])
{
    if (!ready(m)||responses==NULL||!lock_op(m)) return false;
    bool ok=seq_begin(m); uint8_t tx[4]={0},rx[4]={0}; if(ok) ok=raw_frame(m,tx,rx); if(ok) { for (uint8_t id=1u; id<=4u; ++id) responses[id-1u]=rx[4u-id]; } bool ended=ok?seq_end(m):seq_end(m); unlock_op(m); return ok&&ended;
}

bool ztm_stepper_get_status(ztm_stepper_t *m,uint8_t id,uint16_t *s){ if(!ready(m)||!valid_id(id)||s==NULL||!lock_op(m))return false; bool ok=get_status_locked(m,id,s); unlock_op(m); return ok; }
bool ztm_stepper_get_param(ztm_stepper_t *m,uint8_t id,uint8_t r,uint8_t n,uint32_t *v){ if(!ready(m)||!valid_id(id)||!lock_op(m))return false; bool ok=get_param_locked(m,id,r,n,v); unlock_op(m); return ok; }
bool ztm_stepper_set_param(ztm_stepper_t *m,uint8_t id,uint8_t r,uint8_t n,uint32_t v){ if(!ready(m)||!valid_id(id)||!lock_op(m))return false; bool ok=set_param_locked(m,id,r,n,v); if(ok&&n==1u&&v<=255u){size_t i=id-1u; switch(r){case L6470_REG_KVAL_HOLD:m->kval_config[i].hold=(uint8_t)v;break;case L6470_REG_KVAL_RUN:m->kval_config[i].run=(uint8_t)v;break;case L6470_REG_KVAL_ACC:m->kval_config[i].acc=(uint8_t)v;break;case L6470_REG_KVAL_DEC:m->kval_config[i].dec=(uint8_t)v;break;case L6470_REG_STEP_MODE:m->microstep_mode[i]=(ztm_stepper_microstep_t)(v&7u);break;default:break;}} if(ok&&id==4u)ok=command_locked(m,4u,L6470_CMD_HARD_HIZ); unlock_op(m); return ok; }

bool ztm_stepper_get_kval_config(ztm_stepper_t *m,uint8_t id,ztm_stepper_kval_config_t *c){ if(!ready(m)||!valid_id(id)||c==NULL||!lock_op(m))return false; uint32_t a,b,d,e; bool ok=get_param_locked(m,id,L6470_REG_KVAL_HOLD,1u,&a)&&get_param_locked(m,id,L6470_REG_KVAL_RUN,1u,&b)&&get_param_locked(m,id,L6470_REG_KVAL_ACC,1u,&d)&&get_param_locked(m,id,L6470_REG_KVAL_DEC,1u,&e); if(ok){c->hold=(uint8_t)a;c->run=(uint8_t)b;c->acc=(uint8_t)d;c->dec=(uint8_t)e;} unlock_op(m); return ok; }
bool ztm_stepper_set_kval_config(ztm_stepper_t *m,uint8_t id,const ztm_stepper_kval_config_t *c){ if(!ready(m)||!valid_id(id)||c==NULL||!lock_op(m))return false; bool ok=apply_kval_locked(m,id,c); if(ok)m->kval_config[id-1u]=*c; if(ok&&id==4u)ok=command_locked(m,4u,L6470_CMD_HARD_HIZ); unlock_op(m); return ok; }
bool ztm_stepper_set_kval_config_all(ztm_stepper_t *m,const ztm_stepper_kval_config_t *c){ if(!ready(m)||c==NULL||!lock_op(m))return false; bool ok=true; for(uint8_t id=1u;id<=4u&&ok;++id){ok=apply_kval_locked(m,id,c);if(ok)m->kval_config[id-1u]=*c;} if(ok)ok=command_locked(m,4u,L6470_CMD_HARD_HIZ); unlock_op(m); return ok; }

bool ztm_stepper_get_microstep(ztm_stepper_t *m,uint8_t id,ztm_stepper_microstep_t *mode){ if(!ready(m)||!valid_id(id)||mode==NULL||!lock_op(m))return false; uint32_t v=0; bool ok=get_param_locked(m,id,L6470_REG_STEP_MODE,1u,&v); if(ok){*mode=(ztm_stepper_microstep_t)(v&7u);ok=valid_microstep(*mode);} unlock_op(m);return ok; }
bool ztm_stepper_set_microstep(ztm_stepper_t *m,uint8_t id,ztm_stepper_microstep_t mode){ if(!ready(m)||!valid_id(id)||!valid_microstep(mode)||!lock_op(m))return false; uint32_t cur=0; bool ok=command_locked(m,id,L6470_CMD_HARD_HIZ)&&get_param_locked(m,id,L6470_REG_STEP_MODE,1u,&cur); if(ok){uint8_t u=(uint8_t)((cur&~7u)|(uint8_t)mode);ok=set_param_locked(m,id,L6470_REG_STEP_MODE,1u,u)&&command_locked(m,id,L6470_CMD_RESET_POS);} if(ok)m->microstep_mode[id-1u]=mode; if(ok&&id==4u)ok=command_locked(m,4u,L6470_CMD_HARD_HIZ); unlock_op(m);return ok; }
bool ztm_stepper_set_microstep_all(ztm_stepper_t *m,ztm_stepper_microstep_t mode){ if(!ready(m)||!valid_microstep(mode)||!lock_op(m))return false; bool ok=true; for(uint8_t id=1u;id<=4u&&ok;++id){uint32_t cur=0;ok=command_locked(m,id,L6470_CMD_HARD_HIZ)&&get_param_locked(m,id,L6470_REG_STEP_MODE,1u,&cur);if(ok){uint8_t u=(uint8_t)((cur&~7u)|(uint8_t)mode);ok=set_param_locked(m,id,L6470_REG_STEP_MODE,1u,u)&&command_locked(m,id,L6470_CMD_RESET_POS);}if(ok)m->microstep_mode[id-1u]=mode;} if(ok)ok=command_locked(m,4u,L6470_CMD_HARD_HIZ); unlock_op(m);return ok; }

bool ztm_stepper_reset(ztm_stepper_t *m,uint8_t id){ if(!ready(m)||!valid_id(id)||!lock_op(m))return false; bool ok=command_locked(m,id,L6470_CMD_RESET_DEVICE); if(ok){zap_delay_ms(m->config.time,1u);ok=apply_config_locked(m,id);} if(ok&&id==4u)ok=command_locked(m,4u,L6470_CMD_HARD_HIZ); if(ok){m->stall[id-1u]=false;m->thermal_warning[id-1u]=false;m->thermal_shutdown[id-1u]=false;} unlock_op(m);return ok; }
bool ztm_stepper_reset_all(ztm_stepper_t *m){ if(!ready(m)||!lock_op(m))return false; bool ok=broadcast_byte_locked(m,L6470_CMD_RESET_DEVICE,false); if(ok){zap_delay_ms(m->config.time,1u);ok=apply_all_locked(m);} if(ok){memset(m->stall,0,sizeof(m->stall));memset(m->thermal_warning,0,sizeof(m->thermal_warning));memset(m->thermal_shutdown,0,sizeof(m->thermal_shutdown));} unlock_op(m);return ok; }
bool ztm_stepper_hard_hiz(ztm_stepper_t *m,uint8_t id){ if(!ready(m)||!valid_id(id)||!lock_op(m))return false;bool ok=command_locked(m,id,L6470_CMD_HARD_HIZ);unlock_op(m);return ok; }
bool ztm_stepper_hard_hiz_all(ztm_stepper_t *m){ if(!ready(m)||!lock_op(m))return false;bool ok=broadcast_byte_locked(m,L6470_CMD_HARD_HIZ,false);unlock_op(m);return ok; }
bool ztm_stepper_soft_stop(ztm_stepper_t *m,uint8_t id){ if(!ready(m)||!valid_id(id)||!lock_op(m))return false;bool ok=command_locked(m,id,L6470_CMD_SOFT_STOP);if(ok&&id==4u)ok=command_locked(m,4u,L6470_CMD_HARD_HIZ);unlock_op(m);return ok; }
bool ztm_stepper_soft_stop_all(ztm_stepper_t *m){ if(!ready(m)||!lock_op(m))return false;bool ok=broadcast_byte_locked(m,L6470_CMD_SOFT_STOP,true);if(ok)ok=command_locked(m,4u,L6470_CMD_HARD_HIZ);unlock_op(m);return ok; }

bool ztm_stepper_run(ztm_stepper_t *m,uint8_t id,bool f,float s){ if(!ready(m)||!active_id(id)||s<0.0f||!lock_op(m))return false;uint8_t b[4];l6470_encode_run(f,l6470_speed_from_steps_per_s(s),b);bool ok=target_sequence_locked(m,id,b,4u);unlock_op(m);return ok; }
bool ztm_stepper_run_all(ztm_stepper_t *m,bool f,float s){ if(!ready(m)||s<0.0f||!lock_op(m))return false;uint8_t b[4];l6470_encode_run(f,l6470_speed_from_steps_per_s(s),b);bool ok=seq_begin(m);for(size_t k=0;k<4u&&ok;++k){uint8_t frame[4]={L6470_CMD_NOP,b[k],b[k],b[k]};ok=raw_frame(m,frame,NULL);}bool ended=seq_end(m);unlock_op(m);return ok&&ended; }
bool ztm_stepper_move(ztm_stepper_t *m,uint8_t id,bool f,uint32_t steps){ if(!ready(m)||!active_id(id)||!lock_op(m))return false;uint8_t b[4];l6470_encode_move(f,steps,b);bool ok=target_sequence_locked(m,id,b,4u);unlock_op(m);return ok; }

bool ztm_stepper_apply_test_config(ztm_stepper_t *m,uint8_t id){ if(!ready(m)||!valid_id(id)||!lock_op(m))return false;bool ok=command_locked(m,id,L6470_CMD_HARD_HIZ);if(ok){m->kval_config[id-1u]=default_kval();m->microstep_mode[id-1u]=ZTM_STEPPER_DEFAULT_MICROSTEP;ok=apply_config_locked(m,id);}if(ok&&id==4u)ok=command_locked(m,4u,L6470_CMD_HARD_HIZ);unlock_op(m);return ok; }
bool ztm_stepper_apply_test_config_all(ztm_stepper_t *m){ if(!ready(m)||!lock_op(m))return false;bool ok=broadcast_byte_locked(m,L6470_CMD_HARD_HIZ,false);if(ok){for(size_t i=0;i<4u;++i){m->kval_config[i]=default_kval();m->microstep_mode[i]=ZTM_STEPPER_DEFAULT_MICROSTEP;}ok=apply_all_locked(m);}unlock_op(m);return ok; }
