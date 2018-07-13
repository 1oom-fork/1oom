#ifndef INC_1OOM_UIOPT_H
#define INC_1OOM_UIOPT_H

#include "types.h"

struct uiopt_s {
    char const * const str;
    enum {
        UIOPT_TYPE_NONE,
        UIOPT_TYPE_FUNC,
        UIOPT_TYPE_BOOL,
        UIOPT_TYPE_CYCLE,
        UIOPT_TYPE_SLIDER_CALL,
        UIOPT_TYPE_SLIDER_INT
    } type;
    union {
        struct {
            bool (*cb)(void);
        } tf;
        struct {
            bool (*toggle)(void);
            bool const * const value_ro_ptr;
        } tb;
        struct {
            bool (*next)(void);
            const char *(*get)(void);
        } tc;
        struct {
            bool (*set)(int value);
            int * const value_ptr;
            int16_t vmin, vmax;
        } ts;
    };
};

#define UIOPT_ITEM_FUNC(_n_, _f_)   { _n_, UIOPT_TYPE_FUNC, .tf = { _f_ } }
#define UIOPT_ITEM_BOOL(_n_, _v_, _s_)  { _n_, UIOPT_TYPE_BOOL, .tb = { _s_, &(_v_) } }
#define UIOPT_ITEM_CYCLE(_n_, _g_, _s_)  { _n_, UIOPT_TYPE_CYCLE, .tc = { _s_, _g_ } }
#define UIOPT_ITEM_SLIDER_CALL(_v_, _s_, _v0_, _v1_) { 0, UIOPT_TYPE_SLIDER_CALL, .ts = { _s_, &(_v_), _v0_, _v1_ } }
#define UIOPT_ITEM_SLIDER_INT(_v_, _v0_, _v1_) { 0, UIOPT_TYPE_SLIDER_INT, .ts = { 0, &(_v_), _v0_, _v1_ } }
#define UIOPT_ITEM_END  { 0, UIOPT_TYPE_NONE }

extern const struct uiopt_s uiopts_audio[];

#endif
