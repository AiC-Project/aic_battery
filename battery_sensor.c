#include <pthread.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "read_battery_proto.h"
#include "aic.h"
#include "battery_sensor.h"

#define BATTERY_UPDATE_PERIOD 1
#define BATTERY_PORT 22473

static int is_initialized = 0;

typedef struct aic_callback {
    const char* const property;
    const char* const match;
} aic_callback_t;

static aic_callback_t callbacks[] = {
    {
        .property = BATTERY_LEVEL,
        .match    =  AIC_FAKE_POWER_SUPPLY "/energy_now"
    },
    {
        .property = BATTERY_FULL,
        .match    = AIC_FAKE_POWER_SUPPLY "/energy_full"
    },
    {
        .property = BATTERY_STATUS,
        .match    = AIC_FAKE_POWER_SUPPLY "/status"
    },
    {
        .property = AC_ONLINE,
        .match    = AIC_FAKE_POWER_SUPPLY "/online"
    }
};

static int start_server()
{
    int server;
    struct sockaddr_in srv_addr;

    memset(&srv_addr, '\0', sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons(BATTERY_PORT);
    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        SLOGE("Unable to start battery server");
        return -1;
    }
    int yes = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (bind(server, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0)
    {
        SLOGE("Unable to bind server socket");
        return -1;
    }
    return server;
}

static int wait_for_client(int server)
{
    int client = -1;
    if (listen(server, 1) < 0)
    {
        SLOGE("Unable to listen, errno=%d", errno);
        return -1;
    }
    client = accept(server, NULL, 0);
    if (client < 0)
    {
        SLOGE("Unable to accept(), errno=%d", errno);
        return -1;
    }
    return client;
}

static void* conn_loop(void* ignored)
{
    int server = -1;
    int client = -1;
    char battery_level[128]  = "5000000";
    char battery_full[128]   = "5000000";
    char battery_status[128] = "Not charging";
    char ac_online[2] = "1";

    ALOGE("Starting battery listening thread");

    property_set(BATTERY_LEVEL, battery_level);
    property_set(BATTERY_FULL, battery_full);
    property_set(BATTERY_STATUS, battery_status);
    property_set(AC_ONLINE, ac_online);

    if ((server = start_server()) == -1)
    {
        ALOGE("unable to create battery socket");
        is_initialized = 0;
        return NULL;
    }
    while ((client = wait_for_client(server)) != -1)
    {
        char buffer[4];
        int byte_count = 0;
        memset(buffer, '\0', 4);

        if ((byte_count = recv(client, buffer, 4, MSG_PEEK)) == -1)
            SLOGD("Battery: error receiving data");
        else if (byte_count == 0)
            SLOGD("Battery: no data received");
        else
        {
            SLOGD("Battery: header byte count is %d", byte_count);
            uint32_t framing_size = read_header(buffer);
            if (framing_size < 4*1024*1024)
                read_body(client, framing_size, battery_level, battery_full, battery_status, ac_online);

            property_set(BATTERY_LEVEL, battery_level);
            property_set(BATTERY_FULL, battery_full);
            property_set(BATTERY_STATUS, battery_status);
            property_set(AC_ONLINE, ac_online);
        }
        sleep(BATTERY_UPDATE_PERIOD);
        close(client);
    }
    is_initialized = 0;
    ALOGE("Exiting thread");
    return NULL;
}

static void init_battery()
{
    if (is_initialized)
        return;
    pthread_t tid;
    if (!pthread_create(&tid, NULL, conn_loop, NULL))
        is_initialized = 1;
    else
        ALOGE("Unable to create listening thread");
}

int aic_get_value_from_proc(const char* path, char* buf, size_t size)
{
    init_battery();
    size_t i;
    for (i = 0; i < (sizeof(callbacks)/sizeof(aic_callback_t)); i++)
    {
        aic_callback_t cb = callbacks[i];
        if (!strcmp(cb.match, path))
        {
            char property[PROPERTY_VALUE_MAX];
            int len = property_get(cb.property, property, buf);
            if (len >= 0 && (size_t)len < size)
            {
                snprintf(buf, len + 1, "%s", property);
                return len;
            }
            return -1;
        }
    }
    return -1;
}

int aic_use_fake_battery_value(const char* path, char* buf, size_t size)
{
    init_battery();
    if (!strncmp(path, AIC_FAKE_POWER_SUPPLY, strlen(AIC_FAKE_POWER_SUPPLY)))
    {
        if (buf && size > 0) buf[0] = '\0';
        return aic_get_value_from_proc(path, buf, size);
    }

    return 0;
}
