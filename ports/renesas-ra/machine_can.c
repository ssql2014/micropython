/*
 * machine.CAN for RA8P1 (CANFD-capable).
 *
 * Python API:
 *   can = CAN(0, baudrate=1000000)
 *   can.send(data, id, *, ext=False, fd=False, timeout=1000)
 *   data, id, ext, fd = can.recv(timeout=1000)
 *   can.deinit()
 *
 * Wraps ra8p1_canfd.c which uses FSP r_canfd in polling mode.
 */

#if defined(RA8P1)

#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "ra/ra8p1_canfd.h"

/* ---- object --------------------------------------------------------- */

typedef struct _machine_can_obj_t {
    mp_obj_base_t base;
    uint8_t       channel;
    uint32_t      baudrate;
} machine_can_obj_t;

extern const mp_obj_type_t machine_can_type;

static machine_can_obj_t s_can_obj = {
    .base     = { &machine_can_type },
    .channel  = 0,
    .baudrate = 1000000,
};

/* ---- make_new ------------------------------------------------------- */

static mp_obj_t machine_can_make_new(const mp_obj_type_t *type,
                                     size_t n_args, size_t n_kw,
                                     const mp_obj_t *all_args) {
    enum { ARG_channel, ARG_baudrate };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_channel,  MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_baudrate, MP_ARG_KW_ONLY  | MP_ARG_INT, { .u_int = 1000000 } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                              MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int ch = args[ARG_channel].u_int;
    if (ch != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("CAN channel must be 0"));
    }
    uint32_t baud = (uint32_t)args[ARG_baudrate].u_int;

    if (ra8p1_canfd_init(baud) != 0) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("CAN init failed"));
    }
    s_can_obj.channel  = 0;
    s_can_obj.baudrate = baud;
    return MP_OBJ_FROM_PTR(&s_can_obj);
}

/* ---- print ---------------------------------------------------------- */

static void machine_can_print(const mp_print_t *print, mp_obj_t self_in,
                               mp_print_kind_t kind) {
    machine_can_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "CAN(%u, baudrate=%u)", self->channel, self->baudrate);
}

/* ---- send ----------------------------------------------------------- */

static mp_obj_t machine_can_send(size_t n_args, const mp_obj_t *pos_args,
                                  mp_map_t *kw_args) {
    enum { ARG_data, ARG_id, ARG_ext, ARG_fd, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_data,    MP_ARG_REQUIRED | MP_ARG_OBJ,                    { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_id,      MP_ARG_REQUIRED | MP_ARG_INT,                    { .u_int = 0 } },
        { MP_QSTR_ext,     MP_ARG_KW_ONLY  | MP_ARG_BOOL,                   { .u_bool = false } },
        { MP_QSTR_fd,      MP_ARG_KW_ONLY  | MP_ARG_BOOL,                   { .u_bool = false } },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY  | MP_ARG_INT,                    { .u_int = 1000 } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
                     MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_buffer_info_t buf;
    mp_get_buffer_raise(args[ARG_data].u_obj, &buf, MP_BUFFER_READ);

    if (buf.len > 64) {
        mp_raise_ValueError(MP_ERROR_TEXT("data too long (max 64 bytes)"));
    }

    int err = ra8p1_canfd_send(
        (uint32_t)args[ARG_id].u_int,
        args[ARG_ext].u_bool,
        args[ARG_fd].u_bool,
        buf.buf, (uint8_t)buf.len,
        (uint32_t)args[ARG_timeout].u_int);

    if (err != 0) {
        mp_raise_OSError(MP_ETIMEDOUT);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_can_send_obj, 3, machine_can_send);

/* ---- recv ----------------------------------------------------------- */

static mp_obj_t machine_can_recv(size_t n_args, const mp_obj_t *pos_args,
                                  mp_map_t *kw_args) {
    enum { ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 1000 } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
                     MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint32_t id; bool ext, fd; uint8_t dlc; uint8_t data[64];
    int err = ra8p1_canfd_recv(&id, &ext, &fd, data, &dlc,
                               (uint32_t)args[ARG_timeout].u_int);
    if (err != 0) {
        mp_raise_OSError(MP_ETIMEDOUT);
    }

    mp_obj_t tuple[4];
    tuple[0] = mp_obj_new_bytes(data, dlc);
    tuple[1] = mp_obj_new_int(id);
    tuple[2] = mp_obj_new_bool(ext);
    tuple[3] = mp_obj_new_bool(fd);
    return mp_obj_new_tuple(4, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_can_recv_obj, 1, machine_can_recv);

/* ---- deinit --------------------------------------------------------- */

static mp_obj_t machine_can_deinit(mp_obj_t self_in) {
    ra8p1_canfd_deinit();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_can_deinit_obj, machine_can_deinit);

/* ---- type ----------------------------------------------------------- */

static const mp_rom_map_elem_t machine_can_locals[] = {
    { MP_ROM_QSTR(MP_QSTR_send),   MP_ROM_PTR(&machine_can_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv),   MP_ROM_PTR(&machine_can_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_can_deinit_obj) },
};
static MP_DEFINE_CONST_DICT(machine_can_locals_dict, machine_can_locals);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_can_type,
    MP_QSTR_CAN,
    MP_TYPE_FLAG_NONE,
    make_new, machine_can_make_new,
    print,    machine_can_print,
    locals_dict, &machine_can_locals_dict
);

#endif /* RA8P1 */
