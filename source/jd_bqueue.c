#include "jd_protocol.h"

struct jd_bqueue {
    uint16_t front;
    uint16_t back;
    uint16_t size;
    uint16_t filled;
    uint8_t data[0];
};

unsigned jd_bqueue_occupied_bytes(jd_bqueue_t q) {
    return q->filled;
}

unsigned jd_bqueue_free_bytes(jd_bqueue_t q) {
    return q->size - q->filled - 1;
}

static void validate(jd_bqueue_t q) {
    if (q->back >= q->front)
        JD_ASSERT(q->filled == q->back - q->front);
    else
        JD_ASSERT(q->filled == (q->back + q->size) - q->front);
    JD_ASSERT(q->front < q->size);
    JD_ASSERT(q->back < q->size);
    JD_ASSERT(q->filled < q->size);
}

int jd_bqueue_push(jd_bqueue_t q, const void *data, unsigned len) {
    if (len == 0)
        return 0;

    int ret = -1;

    target_disable_irq();
    if (len <= jd_bqueue_free_bytes(q)) {
        ret = 0;
        unsigned n = q->size - q->back;
        if (n > len)
            n = len;
        memcpy(q->data + q->back, data, n);
        q->filled += len;
        if (len > n) {
            len -= n;
            memcpy(q->data, (const uint8_t *)data + n, len);
            q->back = len;
        } else {
            q->back += n;
            if (q->back == q->size)
                q->back = 0;
        }
    }
    validate(q);
    target_enable_irq();

    return ret;
}

unsigned jd_bqueue_available_cont_data(jd_bqueue_t q) {
    unsigned ret;
    if (q->front <= q->back)
        ret = q->back - q->front;
    else
        ret = q->size - q->front;
    JD_ASSERT(ret <= q->filled);
    return ret;
}

uint8_t *jd_bqueue_cont_data_ptr(jd_bqueue_t q) {
    return q->data + q->front;
}

void jd_bqueue_cont_data_advance(jd_bqueue_t q, unsigned sz) {
    q->front += sz;
    q->filled -= sz;
    if (q->front == q->size)
        q->front = 0;
    validate(q);
}

unsigned jd_bqueue_pop_at_most(jd_bqueue_t q, void *dst, unsigned maxsize) {
    unsigned sz;
    target_disable_irq();
    sz = jd_bqueue_available_cont_data(q);
    if (sz > maxsize)
        sz = maxsize;
    memcpy(dst, jd_bqueue_cont_data_ptr(q), sz);
    jd_bqueue_cont_data_advance(q, sz);
    maxsize -= sz;
    if (maxsize > 0) {
        unsigned n = jd_bqueue_available_cont_data(q);
        if (n > maxsize)
            n = maxsize;
        memcpy((uint8_t *)dst + sz, jd_bqueue_cont_data_ptr(q), n);
        jd_bqueue_cont_data_advance(q, n);
        sz += n;
    }
    target_enable_irq();
    return sz;
}

int jd_bqueue_pop_atomic(jd_bqueue_t q, void *dst, unsigned size) {
    int r;
    target_disable_irq();
    if (q->filled >= size) {
        unsigned rr = jd_bqueue_pop_at_most(q, dst, size);
        JD_ASSERT(rr == size);
        r = 0;
    } else {
        r = -1;
    }
    target_enable_irq();
    return r;
}

int jd_bqueue_pop_byte(jd_bqueue_t q) {
    int r;
    target_disable_irq();
    if (q->filled) {
        r = *jd_bqueue_cont_data_ptr(q);
        jd_bqueue_cont_data_advance(q, 1);
    } else {
        r = -1;
    }
    target_enable_irq();
    return r;
}

jd_bqueue_t jd_bqueue_alloc(unsigned size) {
    jd_bqueue_t q = jd_alloc(sizeof(*q) + size);
    q->size = size;
    q->front = q->back = q->filled = 0;
    return q;
}

#define TEST_SIZE 512
void jd_bqueue_test(void) {
    jd_bqueue_t q = jd_bqueue_alloc(TEST_SIZE);
    int len = 0;

    uint8_t push_data = 0;
    uint8_t pop_data = 0;
    static uint8_t buf[1 << 8];
    int numfull = 0;

    for (int i = 0; i < 200000; ++i) {
        if ((jd_random() & 1) == 1) {
            int sz = (jd_random() & (sizeof(buf) - 1)) + 1;
            if (sz + len < TEST_SIZE) {
                for (int j = 0; j < sz; ++j)
                    buf[j] = push_data++;
                // DMESG("push %d", sz);
                int r = jd_bqueue_push(q, buf, sz);
                JD_ASSERT(r == 0);
                len += sz;
            } else {
                int r = jd_bqueue_push(q, buf, sz);
                JD_ASSERT(r != 0);
                numfull++;
            }
        } else {
            int sz = (jd_random() & 0xff) + 1;
            if ((jd_random() & 1) == 1) {
                sz = jd_bqueue_pop_at_most(q, buf, sz);
            } else {
                if (jd_bqueue_pop_atomic(q, buf, sz) != 0) {
                    JD_ASSERT(len < sz);
                    sz = 0;
                }
            }
            len -= sz;
            // DMESG("pop %d", sz);

            for (int j = 0; j < sz; ++j)
                JD_ASSERT(buf[j] == pop_data++);
        }

        JD_ASSERT(len == (int)jd_bqueue_occupied_bytes(q));
    }

    DMESG("q-test OK %d full", numfull);
}
