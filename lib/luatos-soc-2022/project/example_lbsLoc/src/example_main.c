#include "common_api.h"
#include "luat_rtos.h"
#include "luat_mobile.h"
#include "lbsLoc.h"
#include "luat_network_adapter.h"
#include "luat_debug.h"
#include "luat_mobile.h"
#include "net_lwip.h"

#define LBSLOC_SERVER_UDP_IP "bs.openluat.com"          // Base station positioning URL
#define LBSLOC_SERVER_UDP_PORT 12411                    // port
#define PRODUCT_KEY "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" // product key


#define MOBILE_MNC (0)
#define UNICOM_MNC (1)
#define TELE_MNC (11)
#define IMSI_LEN (18)
#define MOBILE_NUM (6)
#define UNICOM_NUM (4)
#define TELE_NUM (4)

const char mobile[MOBILE_NUM][3] = {"00", "02", "04", "07", "08", "13"}; //China Mobile
const char unicom[UNICOM_NUM][3] = {"01", "06", "09", "10"};             // China Unicom
const char tele[TELE_NUM][3] = {"03", "05", "11", "12"};                 // China Telecom

luat_rtos_task_handle lbsLoc_request_task_handle;
uint8_t g_link_status = 0;

static void mobile_event_callback_t(LUAT_MOBILE_EVENT_E event, uint8_t index, uint8_t status)
{
    switch (event)
    {
    case LUAT_MOBILE_EVENT_NETIF:
        switch (status)
        {
        case LUAT_MOBILE_NETIF_LINK_ON:
            g_link_status = 1;
            LUAT_DEBUG_PRINT("Network registration successful\r\n");
			luat_socket_check_ready(index, NULL);
            break;
        default:
            LUAT_DEBUG_PRINT("The network was not registered successfully\r\n");
            g_link_status = 0;
            break;
        }
    case LUAT_MOBILE_EVENT_SIM:
        switch (status)
        {
        case LUAT_MOBILE_SIM_READY:
            LUAT_DEBUG_PRINT("SIM card has been inserted\r\n");
            break;
        case LUAT_MOBILE_NO_SIM:
        default:
            break;
        }
    case LUAT_MOBILE_EVENT_CELL_INFO:
        switch (status)
        {
        case LUAT_MOBILE_CELL_INFO_UPDATE:
            LUAT_DEBUG_PRINT("The periodic search for cell information is completed once\r\n");
            break;
        default:
            break;
        }
    }
}

static int8_t search_mnc(char *mnc)
{
    for (uint8_t i = 0; i < MOBILE_NUM; i++)
    {
        if (strncmp(mnc, mobile[i], 2) == 0)
        {
            return MOBILE_MNC;
        }
    }
    for (uint8_t i = 0; i < UNICOM_NUM; i++)
    {
        if (strncmp(mnc, unicom[i], 2) == 0)
        {
            return UNICOM_MNC;
        }
    }
    for (uint8_t i = 0; i < TELE_NUM; i++)
    {
        if (strncmp(mnc, tele[i], 2) == 0)
        {
            return TELE_MNC;
        }
    }
    return -1;
}


static uint8_t imeiToBcd(uint8_t *arr, uint8_t len, uint8_t *outPut)
{
    if (len % 2 != 0)
    {
        arr[len] = 0x0f;
    }

    uint8_t tmp = 0;

    for (uint8_t j = 0; j < len; j = j + 2)
    {
        outPut[tmp] = (arr[j] & 0x0f) << 4 | (arr[j + 1] & 0x0f);
        tmp++;
    }
    for (uint8_t i = 0; i < 8; i++)
    {
        outPut[i] = (outPut[i] % 0x10) * 0x10 + (outPut[i] - (outPut[i] % 0x10)) / 0x10;
    }
    return 0;
}

/// @brief BCD ->> str
/// @param pOutBuffer
/// @param pInBuffer
/// @param nInLen length
/// @return
static uint32_t location_service_bcd_to_str(uint8_t *pOutBuffer, uint8_t *pInBuffer, uint32_t nInLen)
{
    uint32_t len = 0;
    uint8_t ch;
    uint8_t *p = pOutBuffer;
    uint32_t i = 0;

    if (pOutBuffer == NULL || pInBuffer == NULL || nInLen == 0)
    {
        return 0;
    }

    for (i = 0; i < nInLen; i++)
    {
        ch = pInBuffer[i] & 0x0F;
        if (ch == 0x0F)
        {
            break;
        }
        *pOutBuffer++ = ch + '0';

        ch = (pInBuffer[i] >> 4) & 0x0F;
        if (ch == 0x0F)
        {
            break;
        }
        *pOutBuffer++ = ch + '0';
    }

    len = pOutBuffer - p;

    return len;
}

static uint8_t location_service_parse_response(struct am_location_service_rsp_data_t *response, uint8_t *latitude, uint8_t *longitude,
                                               uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    uint8_t loc[20] = {0};
    uint32_t len = 0;

    if (response == NULL || latitude == NULL || longitude == NULL || year == NULL || month == NULL || day == NULL || hour == NULL || minute == NULL || second == NULL)
    {
        LUAT_DEBUG_PRINT("location_service_parse_response: invalid parameter\r\n");
        return 0;
    }

    if (!(response->result == 0 || response->result == 0xFF))
    {
        LUAT_DEBUG_PRINT("location_service_parse_response: result fail %d\r\n", response->result);
        return 0;
    }

    // latitude
    len = location_service_bcd_to_str(loc, response->latitude, AM_LOCATION_SERVICE_LOCATION_BCD_LEN);
    if (len <= 0)
    {
        LUAT_DEBUG_PRINT("location_service_parse_response: latitude fail\r\n");
        return 0;
    }
    strncat((char *)latitude, (char *)loc, 3);
    strncat((char *)latitude, ".", 2);
    strncat((char *)latitude, (char *)(loc + 3), len - 3);
    len = location_service_bcd_to_str(loc, response->longitude, AM_LOCATION_SERVICE_LOCATION_BCD_LEN);
    if (len <= 0)
    {
        LUAT_DEBUG_PRINT("location_service_parse_response: longitude fail\r\n");
        return 0;
    }
    strncat((char *)longitude, (char *)loc, 3);
    strncat((char *)longitude, (char *)".", 2);
    strncat((char *)longitude, (char *)(loc + 3), len - 3);
    *year = response->year + 2000;
    *month = response->month;
    *day = response->day;
    *hour = response->hour;
    *minute = response->minute;
    *second = response->second;
    return 1;
}
static int32_t luat_test_socket_callback(void *pdata, void *param)
{
    OS_EVENT *event = (OS_EVENT *)pdata;
    LUAT_DEBUG_PRINT("%x", event->ID);
    return 0;
}
static void lbsloc_request_task(void *param)
{
    while (1)
    {
        while (!g_link_status)
        {
            luat_rtos_task_sleep(1000);
            LUAT_DEBUG_PRINT("wait link up");
        }

        network_ctrl_t *network_ctrl = NULL;
        network_ctrl = network_alloc_ctrl(NW_ADAPTER_INDEX_LWIP_GPRS);
        network_init_ctrl(network_ctrl, luat_rtos_get_current_handle(), luat_test_socket_callback, NULL);
        network_set_base_mode(network_ctrl, 0, 15000, 0, 0, 0, 0);
        int result;
        while (1)
        {
            result = network_wait_link_up(network_ctrl, 5000);
            if (result)
            {
                LUAT_DEBUG_PRINT("network_wait_link_up fail %d\r\n", result);
                continue;
            }
            else
            {
                LUAT_DEBUG_PRINT("network_wait_link_up success %d\r\n", result);
                break;
            }
        }
        struct am_location_service_rsp_data_t locationServiceResponse;
        uint8_t latitude[20] = {0};  // longitude
        uint8_t longitude[20] = {0}; // Dimensions
        uint16_t year = 0;           // Year
        uint8_t month = 0;           // moon
        uint8_t day = 0;             // day
        uint8_t hour = 0;            // Hour
        uint8_t minute = 0;          // minute
        uint8_t second = 0;          // Second
        uint8_t lbsLocReqBuf[176] = {0};
        int8_t mnc = -1;
        char imsi[24] = {0};
        memset(lbsLocReqBuf, 0, 176);
        uint8_t sendLen = 0;
        lbsLocReqBuf[sendLen++] = strlen(PRODUCT_KEY);
        memcpy(&lbsLocReqBuf[sendLen], (UINT8 *)PRODUCT_KEY, strlen(PRODUCT_KEY));
        sendLen = sendLen + strlen(PRODUCT_KEY);
        lbsLocReqBuf[sendLen++] = 0x28;
        CHAR imeiBuf[16];
        memset(imeiBuf, 0, sizeof(imeiBuf));
        luat_mobile_get_imei(0, imeiBuf, 16);
        uint8_t imeiBcdBuf[8] = {0};
        imeiToBcd((uint8_t *)imeiBuf, 15, imeiBcdBuf);
        memcpy(&lbsLocReqBuf[sendLen], imeiBcdBuf, 8);
        sendLen = sendLen + 8;
        CHAR muidBuf[64];
        memset(muidBuf, 0, sizeof(muidBuf));
        luat_mobile_get_muid(muidBuf, sizeof(muidBuf));
        lbsLocReqBuf[sendLen++] = strlen(muidBuf);
        memcpy(&lbsLocReqBuf[sendLen], (UINT8 *)muidBuf, strlen(muidBuf));
        sendLen = sendLen + strlen(muidBuf);
        luat_mobile_cell_info_t cell_info;
        memset(&cell_info, 0, sizeof(cell_info));
        luat_mobile_get_cell_info(&cell_info);
        lbsLocReqBuf[sendLen++] = 0x01;
        lbsLocReqBuf[sendLen++] = (cell_info.lte_service_info.tac >> 8) & 0xFF;
        lbsLocReqBuf[sendLen++] = cell_info.lte_service_info.tac & 0xFF;
        lbsLocReqBuf[sendLen++] = (cell_info.lte_service_info.mcc >> 8) & 0xFF;
        lbsLocReqBuf[sendLen++] = cell_info.lte_service_info.mcc & 0XFF;
        if (luat_mobile_get_imsi(0, imsi, sizeof(imsi)) > 0)
        {
            char tmp[3] = {0};
            memcpy(tmp, imsi + 3, 2);
            mnc = search_mnc(tmp);
            if (mnc != -1)
            {
                lbsLocReqBuf[sendLen++] = mnc;
            }
        }
        else
        {
            mnc = cell_info.lte_service_info.mnc;
            if (mnc > 10)
            {
                char tmp[3] = {0};
                snprintf(tmp, 3, "%02x", mnc);
                int ret1 = atoi(tmp);
                lbsLocReqBuf[sendLen++] = ret1;
            }
            else
            {
                lbsLocReqBuf[sendLen++] = mnc;
            }
        }

        int16_t sRssi;
        uint8_t retRssi;
        sRssi = cell_info.lte_service_info.rssi;
        if (sRssi <= -113)
        {
            retRssi = 0;
        }
        else if (sRssi < -52)
        {
            retRssi = (sRssi + 113) >> 1;
        }
        else
        {
            retRssi = 31;
        }
        lbsLocReqBuf[sendLen++] = retRssi;
        lbsLocReqBuf[sendLen++] = (cell_info.lte_service_info.cid >> 24) & 0xFF;
        lbsLocReqBuf[sendLen++] = (cell_info.lte_service_info.cid >> 16) & 0xFF;
        lbsLocReqBuf[sendLen++] = (cell_info.lte_service_info.cid >> 8) & 0xFF;
        lbsLocReqBuf[sendLen++] = cell_info.lte_service_info.cid & 0xFF;

        uint32_t tx_len, rx_len;
        uint8_t is_break, is_timeout;

        result = network_connect(network_ctrl, LBSLOC_SERVER_UDP_IP, sizeof(LBSLOC_SERVER_UDP_IP), NULL, LBSLOC_SERVER_UDP_PORT, 5000);
        LUAT_DEBUG_PRINT("%d", result);
        if (!result)
        {
            result = network_tx(network_ctrl, lbsLocReqBuf, sendLen, 0, NULL, 0, &tx_len, 5000);
            if (!result)
            {
                result = network_wait_rx(network_ctrl, 15000, &is_break, &is_timeout);
                LUAT_DEBUG_PRINT("%d", result);
                if (!result)
                {
                    if (!is_timeout && !is_break)
                    {
                        result = network_rx(network_ctrl, &locationServiceResponse, sizeof(struct am_location_service_rsp_data_t), 0, NULL, NULL, &rx_len);
                        if (!result && (sizeof(struct am_location_service_rsp_data_t) == rx_len))
                        {
                            LUAT_DEBUG_PRINT("response result:%d", locationServiceResponse.result);
                            switch (locationServiceResponse.result)
                            {
                            case LBSLOC_SUCCESS:
                            case WIFILOC_SUCCESS:
                                if (location_service_parse_response(&locationServiceResponse, latitude, longitude, &year, &month, &day, &hour, &minute, &second))
                                {
                                    LUAT_DEBUG_PRINT("latitude: %s, longitude: %s, year: %d, month: %d, day: %d, hour: %d, minute: %d, second: %d", latitude, longitude, year, month, day, hour, minute, second);
                                }
                                else
                                {
                                    LUAT_DEBUG_PRINT("rcv response, but process fail\r\n");
                                }
                                break;
                            case UNKNOWN_LOCATION:
                                LUAT_DEBUG_PRINT("The base station database cannot query the location information of all cells\r\n");
                                LUAT_DEBUG_PRINT("Open http://bs.openluat.com/ in your computer browser and manually find the location of the community based on the printed community information, mcc: %x, mnc: %x, lac: %d, ci: %d \r\n", cell_info.lte_service_info.mcc, cell_info.lte_service_info.mnc, cell_info.lte_service_info.tac, cell_info.lte_service_info.cid);
                                LUAT_DEBUG_PRINT("If the location can be found manually, there is a BUG in the server. Report the problem directly to the technical staff\r\n");
                                LUAT_DEBUG_PRINT("If the location cannot be found manually, the base station database has not yet included the cell location information of the current device. Feedback to the technical staff and we will include it as soon as possible\r\n");
                                break;
                            case PERMISSION_ERROR:
                                LUAT_DEBUG_PRINT("Permission error, please contact official personnel to try to locate the problem %d\r\n", locationServiceResponse.result);
                                break;
                            case UNKNOWN_ERROR:
                                LUAT_DEBUG_PRINT("Unknown error, please contact official personnel to try to locate the problem");
                                break;
                            default:
                                break;
                            }
                        }
                    }
                }
                else
                {
                    LUAT_DEBUG_PRINT("tx fil");
                }
            }
        }
        else
        {
            LUAT_DEBUG_PRINT("connect fil");
        }
        network_close(network_ctrl, 5000);
        network_release_ctrl(network_ctrl);
        memset(latitude, 0, 20);
        memset(longitude, 0, 20);
        year = 0;
        month = 0;
        day = 0;
        hour = 0;
        minute = 0;
        second = 0;
        luat_rtos_task_sleep(30000);
    }
}

void lbsloc_demo_init(void)
{
    luat_rtos_task_handle lbsloc_demo_task_handle;
    luat_rtos_task_create(&lbsLoc_request_task_handle, 4 * 2048, 50, "lbs task", lbsloc_request_task, NULL, NULL);
}
void mobile_event_register(void)
{
    net_lwip_init();
	net_lwip_register_adapter(NW_ADAPTER_INDEX_LWIP_GPRS);
	network_register_set_default(NW_ADAPTER_INDEX_LWIP_GPRS);
    luat_mobile_event_register_handler(mobile_event_callback_t);
}
INIT_HW_EXPORT(mobile_event_register, "0");
INIT_TASK_EXPORT(lbsloc_demo_init,"1");
