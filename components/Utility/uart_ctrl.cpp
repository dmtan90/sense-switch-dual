#include "uart_ctrl.h"
#include "config.h"
#include "sys_ctrl.h"
#include <driver/uart.h>
#include <soc/uart_struct.h>
#include <soc/uart_reg.h>
#include <time.h>       /* time */

const char uart_ctrl::mTag[] = "uart_ctrl";
uart_ctrl* uart_ctrl::s_pInstance = NULL;
SemaphoreHandle_t uart_ctrl::xSemaphore = NULL;

// Both definition are same and valid
//static uart_isr_handle_t *handle_console;
static intr_handle_t handle_console;

// Receive buffer to collect incoming data

// Register to collect data length


/*
 * Define UART interrupt subroutine to ackowledge interrupt
 */
void IRAM_ATTR uart_ctrl::uart_intr_handle(void *arg)
{
	ESP_LOGD(mTag, "uart_intr_handle");
	uint16_t rx_fifo_len = 0;
	uint16_t i = 0;
	bool status = false;
	uint16_t urxlen;
	//uint8_t data[uart_ctrl::RX_BUF_SIZE];
	uint8_t rxbuf[uart_ctrl::RX_BUF_SIZE];
	memset(&rxbuf[0], 0x00, uart_ctrl::RX_BUF_SIZE);

	//status = UART1.int_st.val; // read UART interrupt Status
	rx_fifo_len = UART2.status.rxfifo_cnt; // read number of bytes in UART buffer
	urxlen = rx_fifo_len;

	while(rx_fifo_len)
	{
		rxbuf[i++] = UART2.fifo.rw_byte; // read all bytes
		rx_fifo_len--;
		status = true;
	}

	if(status)
	{
		ESP_LOGD(mTag, "Read %d bytes: '%s'", urxlen, rxbuf);
		s_pInstance->setData(&rxbuf[0], urxlen);
	}

	// after reading bytes from buffer clear UART interrupt status
	uart_clear_intr_status(UART_NUM_2, UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);

	// a test code or debug code to indicate UART receives successfully,
	// you can redirect received byte as echo also
	//uart_write_bytes(UART_NUM_2, (const char*) "RX Done", 7);
}

void uart_ctrl::rx_task(void* param)
{
    ESP_LOGD(mTag, "rx_task");
    uint8_t* data = new uint8_t[uart_ctrl::RX_BUF_SIZE];
    while(1)
    {
    	memset(data, 0x00, uart_ctrl::RX_BUF_SIZE);
    	if(s_pInstance->isWaitingData == true)
    	{
    		uint8_t coundown = 30;
		    while (coundown > 0) 
		    {
		    	//simulate UART data
		    	//const int rxBytes = simulateUartData(data, uart_ctrl::RX_BUF_SIZE);
		        const int rxBytes = uart_read_bytes(UART_NUM_2, data, uart_ctrl::RX_BUF_SIZE, 200 / portTICK_RATE_MS);
		        if (rxBytes > 0) 
		        {
		            data[rxBytes] = 0;
		            ESP_LOGD(mTag, "Read %d bytes: '%s'", rxBytes, data);
		            s_pInstance->setData(data, rxBytes);
		            break;
		        }
		        delay_ms(100);
		        coundown--;
		    }
        }
    	
	    delay_ms(10);
    }
    delete[] data;
    sys_ctrl::rebootSystem(0);
    vTaskDelete(NULL);
}

uart_ctrl::uart_ctrl()
{
	ESP_LOGD(mTag, "uart_ctrl");
	for(int i = 0; i < RX_BUF_SIZE; i++)
	{
		mData[i] = 0x00;
	}
	mLength = 0;
	isWaitingData = false;
	init();
};

uart_ctrl::~uart_ctrl()
{
	ESP_LOGD(mTag, "~uart_ctrl");	
}

uart_ctrl* uart_ctrl::getInstance()
{
	//ESP_LOGD(mTag, "getInstance");
	if(s_pInstance == NULL)
	{
		s_pInstance = new uart_ctrl();
	}
	return s_pInstance;
}


void uart_ctrl::init()
{
	ESP_LOGD(mTag, "init");
	const uart_config_t uart_config = {
        .baud_rate = 38400,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN, //CSE7761 8E1 bits
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, RXD_PIN, TXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // We won't use a buffer for sending data.
    const int uart_buffer_size = (RX_BUF_SIZE * 2);
    uart_driver_install(UART_NUM_2, uart_buffer_size, 0, 0, NULL, 0);
    //init receive response data
    xTaskCreate(rx_task, "UART Communication", 4096, NULL, tskIDLE_PRIORITY, NULL);

 //    // release the pre registered UART handler/subroutine
	// ESP_ERROR_CHECK(uart_isr_free(UART_NUM_2));

	// // register new UART subroutine
	// ESP_ERROR_CHECK(uart_isr_register(UART_NUM_2, uart_intr_handle, NULL, ESP_INTR_FLAG_IRAM, &handle_console));

	// // enable RX interrupt
	// ESP_ERROR_CHECK(uart_enable_rx_intr(UART_NUM_2));
}

bool uart_ctrl::sendData(const uint8_t* data, const size_t length, const bool needResponse)
{
	createAndTakeSemaphore();
	bool success = false;
	//const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_2, (char*)data, length);
    ESP_LOGD(mTag, "sendData: Wrote %d bytes => wait response: %s", txBytes, needResponse ? "true" : "false");
    /*for(uint16_t i = 0; i < length; i++)
    {
    	ESP_LOGD(mTag, "0x%02X", data[i]);
    }
    ESP_LOGD(mTag, "==========================================");*/
    if(length == txBytes)
    {
    	success = true;
    	memset(mLastCMD, 0x00, RX_BUF_SIZE);
    	for(uint16_t i = 0; i < length; i++)
    	{
    		mLastCMD[i] = data[i];
    	}
    	//strcpy(mLastCMD, data);
    }

    //syncronize
	if(success == true && needResponse == true)
	{
		//clear old data
		this->setData(NULL);
		this->isWaitingData = true;
		//timeout = 5s
		uint8_t countdown = 10;
		while(mLength == 0 && countdown > 0)
		{
			delay_ms(500);
			countdown--;
		}

		if(mLength == 0)
		{
			success = false;
		}

		this->isWaitingData = false;
	}
	releaseSemaphore();
	return success;
}

void uart_ctrl::setData(const uint8_t* data, const size_t length)
{
	for(int i = 0; i < RX_BUF_SIZE; i++)
	{
		mData[i] = 0x00;
	}

	if(length > 0 && data != NULL)
	{
		mLength = length;
		for(int i = 0; i < length; i++)
		{
			mData[i] = data[i];
		}
	}
	else
	{
		mLength = 0;
	}
}

uint8_t* uart_ctrl::readData(uint16_t &len)
{
	len = mLength;
	return mData;
}

void uart_ctrl::createAndTakeSemaphore()
{
    if(NULL == xSemaphore)
    {
        xSemaphore = xSemaphoreCreateMutex();
    }

    while( xSemaphoreTake( xSemaphore, ( TickType_t ) 100 ) == 0 );
}

void uart_ctrl::releaseSemaphore()
{
    xSemaphoreGive( xSemaphore );
}

