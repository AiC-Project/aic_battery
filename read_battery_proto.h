#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint32_t read_header(const char* buffer);
void read_body(int csock, uint32_t size, char* level, char* full, char* status, char* online);

#ifdef __cplusplus
}
#endif
