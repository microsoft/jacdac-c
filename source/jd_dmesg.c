#include "jd_protocol.h"
#include "jd_dmesg.h"

#if JD_DMESG_BUFFER_SIZE > 0

#if JD_DMESG_BUFFER_SIZE < 256
#error "Too small DMESG buffer"
#endif

struct CodalLogStore codalLogStore;

JD_FAST
static void logwriten(const char *msg, int l) {
    target_disable_irq();
    if (codalLogStore.ptr + l >= sizeof(codalLogStore.buffer)) {
        codalLogStore.buffer[0] = '.';
        codalLogStore.buffer[1] = '.';
        codalLogStore.buffer[2] = '.';
        codalLogStore.ptr = 3;
    }
    if (l + codalLogStore.ptr >= sizeof(codalLogStore.buffer))
        return; // shouldn't happen
    memcpy(codalLogStore.buffer + codalLogStore.ptr, msg, l);
    codalLogStore.ptr += l;
    codalLogStore.buffer[codalLogStore.ptr] = 0;
    target_enable_irq();
}

void jd_vdmesg(const char *format, va_list ap) {
    char tmp[160];
    jd_vsprintf(tmp, sizeof(tmp) - 1, format, ap);
    int len = strlen(tmp);
#if JD_LORA || JD_ADVANCED_STRING
    while (len && (tmp[len - 1] == '\r' || tmp[len - 1] == '\n'))
        len--;
#endif
    tmp[len] = '\n';
    tmp[len + 1] = 0;
    logwriten(tmp, len + 1);
}

void jd_dmesg(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    jd_vdmesg(format, arg);
    va_end(arg);
}

JD_FAST
unsigned jd_dmesg_read(void *dst, unsigned space, uint32_t *state) {
    target_disable_irq();
    uint32_t curr_ptr = codalLogStore.ptr;
    if (curr_ptr < *state) {
        // the dmesg buffer wrapped-around
        curr_ptr = *state;
        while (curr_ptr < sizeof(codalLogStore.buffer) && codalLogStore.buffer[curr_ptr] != 0)
            curr_ptr++;
        if (curr_ptr == *state) {
            // nothing more to write
            *state = 0;
            curr_ptr = codalLogStore.ptr;
        }
    }

    int towrite = curr_ptr - *state;
    if (towrite > 0) {
        if (towrite > space)
            towrite = space;
        memcpy(dst, codalLogStore.buffer + *state, towrite);
        *state += towrite;
    } else {
        towrite = 0;
    }
    target_enable_irq();

    return towrite;
}

#endif
