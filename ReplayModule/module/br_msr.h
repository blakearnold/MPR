
//freeze on smm
#define IA32_DEBUGCTL 0x1D9
#define IA32_PERF_CAPABILITIES 0x345

//GENERAL PERPOSE performance counters
#define IA32_PMC0 0xc1
#define IA32_PMC1 0xc2
#define IA32_PMC2 0xc3
#define IA32_PMC3 0xc4

//fixed_function performance counters
#define IA32_FIXED_CTR0 0x309
#define IA32_FIXED_CTR1 0x30A
#define IA32_FIXED_CTR2 0x30B

//selectors for gen purpose performance counters
#define IA32_PERFEVTSEL0 0x186
#define IA32_PERFEVTSEL1 0x187
#define IA32_PERFEVTSEL2 0x188
#define IA32_PERFEVTSEL3 0x189

//PEBS
//#define IA32_MISC_ENABLES
//#define IA32_PEBS_ENABLE
//#define IA32_DS_AREA
