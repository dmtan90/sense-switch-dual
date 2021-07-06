/*
  * socket_ota_update
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "socket_ota_update.h"
#include "ota_update.h"
#include "config.h"


typedef enum {
    NWR_READ_COMPLETE = 0,
    NWR_READ_TIMEOUT = 1,
    NWR_DISCONNECTED = 2,
    NWR_ERROR = 3,
} TNetworkResult;

const char TAG[] =  "SocketOTAUpadate";

#define NORM_C(c) (((c) >= 32 && (c) < 127) ? (c) : '.')

#define AP_CONNECTION_ESTABLISHED (1 << 0)
static EventGroupHandle_t sEventGroup;

// Indicates that we should trigger a re-boot after sending the response.
static int sRebootAfterReply;

// Listen to TCP requests on port 80
static void socketTask(void *pvParameters);

static void processMessage(const char *message, int messageLen, char *responseBuf, int responseBufLen);
static int networkReceive(int s, char *buf, int maxLen, int *actualLen);
static void networkSetConnected(uint8_t c);

Socketota_update* instance = NULL;

Socketota_update::Socketota_update()
{
	ESP_LOGD(TAG, "Socketota_update");
};

Socketota_update::~Socketota_update()
{
	ESP_LOGD(TAG, "~Socketota_update");
};

Socketota_update* Socketota_update::getInstance()
{
	if(instance == NULL)
	{
		instance = new Socketota_update();
	}
	return instance;
};

void Socketota_update::init()
{
    ESP_LOGI(TAG, "init");

    sEventGroup = xEventGroupCreate();
    xTaskCreate(&socketTask, "socketTask", 10240, NULL, tskIDLE_PRIORITY, NULL);

    ota_update *ota = ota_update::getInstance();
    if(!ota->inProgress())
    {
        ota->init();
    }
};

int Socketota_update::isConnected()
{
    return xEventGroupGetBits(sEventGroup) & AP_CONNECTION_ESTABLISHED;
}

static void socketTask(void *pvParameters)
{
    const int maxRequestLen = 10000;
    const int maxResponseLen = 1000;
    const int tcpPort = 8080;

    ESP_LOGD(TAG, "socketTask");

    while (1) {

        // Barrier for the connection (we need to be connected to an AP).
        xEventGroupWaitBits(sEventGroup, AP_CONNECTION_ESTABLISHED, false, true, portMAX_DELAY);
        ESP_LOGD(TAG, "socketTask: connected to access point");

        // Create TCP socket.

        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) {
            ESP_LOGD(TAG, "socketTask: failed to create socket: %d (%s)", errno, strerror(errno));
            delay_ms(1000);
            continue;
        }


        // Bind socket to port.

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof (struct sockaddr_in));
        serverAddr.sin_len = sizeof(struct sockaddr_in);
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(tcpPort);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        int b = bind(s, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in));
        if (b < 0) {
            ESP_LOGD(TAG, "socketTask: failed to bind socket %d: %d (%s)", s, errno, strerror(errno));
            delay_ms(1000);
            continue;
        }


        // Listen to incoming connections.

        ESP_LOGD(TAG, "socketTask: 'listen' on socket %d", s);
        listen(s, 1); // backlog max. 1 connection
        while (1) {


            // Accept the connection on a separate socket.

            ESP_LOGD(TAG, "--------------------");
            ESP_LOGD(TAG, "socketTask: 'accept' on socket %d", s);
            struct sockaddr_in clientAddr;
            socklen_t clen = sizeof(clientAddr);
            int s2 = accept(s, (struct sockaddr *)&clientAddr, &clen);
            if (s2 < 0) {
                ESP_LOGE(TAG, "socketTask: 'accept' failed: %d (%s)", errno, strerror(errno));
                delay_ms(1000);
                break;
            }

            // Would normally fork here.
            // For the moment, we support only a single open connection at any time.

            do {
            	// Allocate and clear memory for the request data.

                char *requestBuf = (char*)malloc(maxRequestLen * sizeof(char));
                if (!requestBuf) {
                    ESP_LOGE(TAG, "socketTask: malloc for requestBuf failed: %d (%s)", errno, strerror(errno));
                    break;
                }
                bzero(requestBuf, maxRequestLen);


                // Read the request and store it in the allocated buffer.

                int totalRequestLen = 0;
                TNetworkResult result = (TNetworkResult)networkReceive(s2, requestBuf, maxRequestLen, &totalRequestLen);

                if (result != NWR_READ_COMPLETE) {
                    ESP_LOGI(TAG, "nothing more to, closing socket %d", s2);
                    free(requestBuf);
                    close(s2);
                    break;
                }

                // Read completed successfully.
                // Process the request and create the response.

                ESP_LOGI(TAG, "socketTask: received %d bytes: %02x %02x %02x %02x ... | %c%c%c%c...",
                         totalRequestLen,
                         requestBuf[0], requestBuf[1], requestBuf[2], requestBuf[3],
                         NORM_C(requestBuf[0]), NORM_C(requestBuf[1]), NORM_C(requestBuf[2]), NORM_C(requestBuf[3]));


                char *responseBuf = (char*)malloc(maxResponseLen * sizeof(char));
                processMessage(requestBuf, totalRequestLen, responseBuf, maxResponseLen);

                free(requestBuf);


                // Send the response back to the client.

                int totalLen = strlen(responseBuf);
                int nofWritten = 0;
                ESP_LOGD(TAG, "socketTask: write %d bytes to socket %d: %02x %02x %02x %02x ... | %c%c%c%c...", totalLen, s2,
                         responseBuf[0], responseBuf[1], responseBuf[2], responseBuf[3],
                         NORM_C(responseBuf[0]), NORM_C(responseBuf[1]), NORM_C(responseBuf[2]), NORM_C(responseBuf[3]));

                do {
                    int n = write(s2, &responseBuf[nofWritten], totalLen - nofWritten);
                    int e = errno;
                    //ESP_LOGD(TAG, "socketTask: write: socket %d, n = %d, errno = %d", s2, n, e);

                    if (n > 0) {
                        nofWritten += n;

                        // More to write?
                        if (totalLen - nofWritten == 0) {
                            break;
                        }

                    } else if (n == 0) {
                        // Disconnected?
                        break;

                    } else {
                        // n < 0
                        if (e == EAGAIN) {
                            //ESP_LOGD(TAG, "socketTask: write: EAGAIN");
                            continue;
                        }
                        ESP_LOGE(TAG, "socketTask: write failed: %d (%s)", errno, strerror(errno));
                        break;
                    }

                } while (1);

                free(responseBuf);


                if (sRebootAfterReply) {
                    ESP_LOGI(TAG, "socketTask: Reboot in 2 seconds...");
                    delay_ms(2000);
                    esp_restart();
                }

            } while (1);
        }

        // Should never arrive here
        close(s);
        delay_ms(2000);
    }
}

static int networkReceive(int s, char *buf, int maxLen, int *actualLen)
{
    //ESP_LOGI(TAG, "networkReceive: start maxlen = %d", maxLen);

    int totalLen = 0;

    for (int timeoutCtr = 0; timeoutCtr < 3000; timeoutCtr++) {

        int readAgain = 0;
        do {
            buf[totalLen] = 0x00;
            int n = recv(s, &buf[totalLen], maxLen - totalLen, MSG_DONTWAIT);
            int e = errno;

            // Error?
            if (n > 0) {
                // Message complete?
                totalLen += n;
                if (totalLen > 0) {
                    // We currently support two record types:
                    // Records that start with !xxxx where x is a hexadecimal length indicator
                    // Records that start with something else and are terminated with a newline
                    int recordLen = 0;
                    int recordWithLengthIndicator = (1 == sscanf(buf, "!%04x", &recordLen));
                    ESP_LOGD(TAG, "networkReceive: recordWithLengthIndicator = %d, expected length = %d, current length = %d", recordWithLengthIndicator, recordLen, totalLen);
                    if ((recordWithLengthIndicator && totalLen == recordLen)
                        || (!recordWithLengthIndicator && buf[totalLen - 1] == '\n'))
                    {
                        ESP_LOGD(TAG, "networkReceive: received %d byte packet on socket %d", totalLen, s);
                        *actualLen = totalLen;
                        return NWR_READ_COMPLETE;
                    }
                }
                // Not yet complete. Read again immediately.
                readAgain = 1;

            } else if (n < 0 && e == EAGAIN) {
                // No data available right now.
                // Wait for a short moment before trying again.
                readAgain = 0;

            } else {
                // Error (n=0, n<0).
                ESP_LOGE(TAG, "recv n = %d, errno = %d (%s)", n, e, strerror(e));
                return NWR_ERROR;
            }

        } while (readAgain);

        // n == 0, wait a bit
        //ESP_LOGI(TAG, "networkReceive: wait for more data");
        delay_ms(10);
    }

    return NWR_READ_TIMEOUT;
}

static void processMessage(const char *message, int messageLen, char *responseBuf, int responseBufLen)
{
    // Response to send back to the TCP client.
    char response[256];
    sprintf(response, "OK\r\n");
    ota_update *ota = ota_update::getInstance();

    if (message[0] == '!') {
        ota_update_result result = OTA_OK;

        if (message[1] == '[') {
            ESP_LOGI(TAG, "processMessage: OTA start");
            result = ota->begin();

        } else if (message[1] == ']') {
            ESP_LOGI(TAG, "processMessage: OTA end");
            result = ota->end();

        } else if (message[1] == '*') {
            ESP_LOGI(TAG, "processMessage: Reboot");
            sRebootAfterReply = 1;

        } else {
            result = ota->writeHexData(&message[5], messageLen - 5);
        }

        if (result != OTA_OK) {
            ESP_LOGE(TAG, "processMessage: OTA_ERROR %d", result);
            sprintf(response, "OTA_ERROR %d\r\n", result);
        }

    } else if (message[0] == '?') {
    	ota->dumpInformation();
    }

    strncpy(responseBuf, response, responseBufLen);
}

static void networkSetConnected(uint8_t c)
{
    if (c) {
        xEventGroupSetBits(sEventGroup, AP_CONNECTION_ESTABLISHED);
    } else {
        xEventGroupClearBits(sEventGroup, AP_CONNECTION_ESTABLISHED);
    }
}
