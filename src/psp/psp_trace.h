/* Optional startup tracing on the PSP (-DKFX_PSP_TRACE=ON), see PlatformPSP.cpp. */
#ifndef KFX_PSP_TRACE_H
#define KFX_PSP_TRACE_H
#if defined(KFX_PSP_TRACE)
#ifdef __cplusplus
extern "C" {
#endif
void psp_trace(const char* fmt, ...);
#ifdef __cplusplus
}
#endif
#define PSP_TRACE(...) psp_trace(__VA_ARGS__)
#else
#define PSP_TRACE(...) ((void)0)
#endif
#endif
