#include <stdio.h>

#define LOG_MODULE_REGISTER(module, ...) static const char *const __ocre_log_module = #module

#define LOG_ERR(fmt, ...)		 fprintf(stderr, "<err> %s: " fmt "\n", __ocre_log_module, ##__VA_ARGS__);
#define LOG_WRN(fmt, ...)		 fprintf(stderr, "<wrn> %s: " fmt "\n", __ocre_log_module, ##__VA_ARGS__);
#define LOG_INF(fmt, ...)		 fprintf(stderr, "<inf> %s: " fmt "\n", __ocre_log_module, ##__VA_ARGS__);
#define LOG_DBG(fmt, ...)		 fprintf(stderr, "<dbg> %s: " fmt "\n", __ocre_log_module, ##__VA_ARGS__);
