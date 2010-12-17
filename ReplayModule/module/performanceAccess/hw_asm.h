#define num_to_mask(x) ((1U << (x)) - 1)

static inline unsigned get_counters(void)
{
        unsigned a, b;
        asm("cpuid" : "=a" (a), "=b" (b) : "0" (0xa) : "ecx","edx");
        return num_to_mask((v >> 8) & 0xff);
}

