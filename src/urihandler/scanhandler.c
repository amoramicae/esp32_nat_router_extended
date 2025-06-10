#include "handler.h"
#include "scan.h"
#include <string.h>

#include "router_globals.h"

extern bool eth_link_up; // From esp32_nat_router.c
static const char *TAG = "ScanHandler";

void fillInfoData(char **db, char **textColor)
{
    char* uplink_mode = NULL;
    get_config_param_str("uplink_mode", &uplink_mode);

    if (uplink_mode != NULL && strcmp(uplink_mode, "eth") == 0) {
        if (eth_link_up) {
            *db = strdup("Connected");
            *textColor = "success";
        } else {
            *db = strdup("Disconnected");
            *textColor = "danger";
        }
    } else {
        *db = realloc(*db, 5);
        wifi_ap_record_t apinfo;
        memset(&apinfo, 0, sizeof(apinfo));
        if (esp_wifi_sta_get_ap_info(&apinfo) == ESP_OK)
        {
            sprintf(*db, "%d", apinfo.rssi);
            *textColor = findTextColorForSSID(apinfo.rssi);
            ESP_LOGD(TAG, "RSSI: %d", apinfo.rssi);
            ESP_LOGD(TAG, "SSID: %s", apinfo.ssid);
        }
        else
        {
            sprintf(*db, "%d", 0);
            *textColor = "danger";
        }
    }
    if (uplink_mode) free(uplink_mode);
}

esp_err_t scan_download_get_handler(httpd_req_t *req)
{

    if (isLocked())
    {
        return redirectToLock(req);
    }

    httpd_req_to_sockfd(req);

    char *defaultIP = getDefaultIPByNetmask();

    extern const char scan_start[] asm("_binary_scan_html_start");
    extern const char scan_end[] asm("_binary_scan_html_end");
    const size_t scan_html_size = (scan_end - scan_start);

    char *scan_page = malloc(scan_html_size + strlen(defaultIP));

    sprintf(scan_page, scan_start, defaultIP);

    closeHeader(req);

    ESP_LOGI(TAG, "Requesting scan page");

    esp_err_t ret = httpd_resp_send(req, scan_page, HTTPD_RESP_USE_STRLEN);
    
    char* uplink_mode = NULL;
    get_config_param_str("uplink_mode", &uplink_mode);
    if(uplink_mode == NULL || strcmp(uplink_mode, "wifi") == 0) {
        fillNodes();
    }
    if (uplink_mode) free(uplink_mode);

    free(scan_page);
    free(defaultIP);
    return ret;
}
