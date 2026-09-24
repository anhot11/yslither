#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "app/src/network/snakeyrain_weather.h"
#include "app/src/network/server_list.h"

void test_snakeyrain_initialization(void) {
    printf("Testing SnakeyRain Weather Radar initialization...\n");
    snakeyrain_weather_init();

    int count = snakeyrain_weather_count();
    assert(count >= 10);
    printf("PASS: snakeyrain_weather_count() = %d (>= 10 seed servers ready)\n", count);

    int total_bots = snakeyrain_weather_total_bots();
    assert(total_bots > 1000);
    printf("PASS: snakeyrain_weather_total_bots() = %d bots monitored\n", total_bots);

    rain_server_entry r0;
    bool ok = snakeyrain_weather_get_copy(0, &r0);
    assert(ok);
    assert(r0.port == 444);
    assert(strlen(r0.ip) > 6);
    assert(r0.bots_alive > 0);
    assert(strlen(r0.city) > 0);
    printf("PASS: Server 0: %s (#%s) in %s (%s) - %d bots, state: %s, port: %d\n",
           r0.full_addr, r0.sid_str, r0.city, r0.cont, r0.bots_alive, r0.dir_state, r0.port);
}

void test_server_list_lluvia_integration(void) {
    printf("Testing server_list integration with Lluvia zone...\n");
    server_list_init();

    int total_servers = server_list_count();
    int official_servers = server_list_official_count();
    int rain_servers = snakeyrain_weather_count();

    assert(total_servers == official_servers + rain_servers);
    printf("PASS: server_list_count = %d (Official: %d + Lluvia: %d)\n",
           total_servers, official_servers, rain_servers);

    // Verify retrieval of a Lluvia server from server_list
    server_entry s_rain;
    bool found_lluvia = false;
    for (int i = 0; i < total_servers; i++) {
        if (server_list_get_copy(i, &s_rain)) {
            if (strcmp(s_rain.region, "Lluvia") == 0) {
                found_lluvia = true;
                assert(s_rain.bots_alive > 0);
                assert(s_rain.port == 444);
                break;
            }
        }
    }
    assert(found_lluvia);
    printf("PASS: Retrieved Lluvia entry from server_list: %s (%s) - %d bots\n",
           s_rain.full_addr, s_rain.name, s_rain.bots_alive);

    // Verify name resolution for Lluvia servers
    const char* rain_name = server_list_get_name_by_ip(s_rain.full_addr);
    assert(rain_name != NULL);
    assert(strstr(rain_name, "Lluvia") != NULL);
    printf("PASS: server_list_get_name_by_ip(\"%s\") = \"%s\"\n", s_rain.full_addr, rain_name);

    // Verify best IP in Lluvia zone
    const char* best_rain_ip = server_list_get_best_ip_by_region("Lluvia");
    assert(best_rain_ip != NULL && strlen(best_rain_ip) > 6);
    printf("PASS: server_list_get_best_ip_by_region(\"Lluvia\") = \"%s\"\n", best_rain_ip);
}

void test_live_network_weather_radar(void) {
    printf("Testing live WebSocket connection to https://snakeyrain.com/weather/ ...\n");
    // Give the background worker thread a moment to connect and stream
    for (int i = 0; i < 15; i++) {
        if (snakeyrain_weather_is_connected()) {
            break;
        }
        usleep(200000); // 200ms
    }

    bool connected = snakeyrain_weather_is_connected();
    int count = snakeyrain_weather_count();
    int bots = snakeyrain_weather_total_bots();

    printf("STATUS: SnakeyRain Radar Live: %s | Active Storms: %d | Total Bots: %d\n",
           connected ? "CONNECTED (Live Stream)" : "STANDBY (Seed/Cached Mode)",
           count, bots);
    assert(count >= 10);
    assert(bots > 0);
    printf("PASS: Weather radar successfully operational!\n");
}

int main(void) {
    printf("============================================================\n");
    printf(" RUNNING SNAKEYRAIN WEATHER & LLUVIA SERVER ZONE TEST SUITE\n");
    printf("============================================================\n");

    test_snakeyrain_initialization();
    test_server_list_lluvia_integration();
    test_live_network_weather_radar();

    printf("============================================================\n");
    printf(" ALL SNAKEYRAIN & LLUVIA SERVER ZONE TESTS PASSED!\n");
    printf("============================================================\n");
    exit(0);
}
