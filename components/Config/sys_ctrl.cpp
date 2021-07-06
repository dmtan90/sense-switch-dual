/*
  * sys_ctrl
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "sys_ctrl.h"
#include "file_storage.h"
#include "config.h"
#include "smart_switch_connector.h"

uint8_t sys_ctrl::s_TimerCountInSecond;

const char sys_ctrl::s_Tag[] = "sys_ctrl";

void sys_ctrl::init()
{
    gpio_pad_select_gpio(BTN_CONFIG_PIN);
    ESP_ERROR_CHECK(gpio_set_direction(BTN_CONFIG_PIN, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(BTN_CONFIG_PIN, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_intr_type(BTN_CONFIG_PIN, GPIO_INTR_NEGEDGE));

    // Intterrupt number see below
    ESP_ERROR_CHECK(gpio_intr_enable(BTN_CONFIG_PIN));

    //Config switch toggle pin SW1
    gpio_pad_select_gpio(SW_1_PIN);
    ESP_ERROR_CHECK(gpio_set_direction(SW_1_PIN, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(SW_1_PIN, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_intr_type(SW_1_PIN, GPIO_INTR_NEGEDGE));
    ESP_ERROR_CHECK(gpio_intr_enable(SW_1_PIN));

    //Config switch toggle pin SW2
    gpio_pad_select_gpio(SW_2_PIN);
    ESP_ERROR_CHECK(gpio_set_direction(SW_2_PIN, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(SW_2_PIN, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_intr_type(SW_2_PIN, GPIO_INTR_NEGEDGE));
    ESP_ERROR_CHECK(gpio_intr_enable(SW_2_PIN));

    ESP_ERROR_CHECK(gpio_isr_register(handleInterruptGpio, NULL, ESP_INTR_FLAG_LOWMED, NULL));

    s_TimerCountInSecond = 0;
}

RUNNING_MODE_CONFIG sys_ctrl::getRunningMode()
{
    return Config::getInstance()->getBootMode();
}

void sys_ctrl::setRunningMode(RUNNING_MODE_CONFIG mode)
{
    Config *cfg = Config::getInstance();
    cfg->setBootMode(mode);
    void *pointer = &mode;
    cfg->writeCF("boot_mode", pointer, CF_DATA_TYPE_UINT8);
    delay_ms(500);
}

void sys_ctrl::showMessage(void* mesg)
{
    ESP_LOGD(s_Tag, "%s", (char*)mesg);
    vTaskDelete(NULL);
}

void sys_ctrl::gotoConfigMode(void* data)
{
    while(1)
    {
        if(TIME_PRESS_BTN_IN_SECOND <= s_TimerCountInSecond)
        {
            setRunningMode(SYSTEM_MODE_CONFIG);
            s_TimerCountInSecond = 0;
            delay_ms(1000);
            sys_ctrl::rebootSystem(0);
        }

        if(0 == GPIO_INPUT_GET(BTN_CONFIG_PIN))
        {
            ESP_LOGI(s_Tag, "Press button count time: %d", s_TimerCountInSecond);
            ++s_TimerCountInSecond;
            delay_ms(1000);
        }
        else
        {
            sys_ctrl::rebootSystem(0);
            break;
        }
    }
    vTaskDelete(NULL);
}

void sys_ctrl::toggleChannel(void* data)
{
    uint8_t channel = atoi((char*)data);
    smart_switch_connector *devman = smart_switch_connector::getInstance();
    devman->toggleChannel((CONTROLLER_CHANNEL)channel);
    vTaskDelete(NULL);
}

void sys_ctrl::handleInterruptGpio(void* arg)
{
    uint32_t gpio_num = 0;
    uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);   //read status to get interrupt status for GPIO0-31
    uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);//read status1 to get interrupt status for GPIO32-39
    SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);    //Clear intr for gpio0-gpio31
    SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h); //Clear intr for gpio32-39
    do
    {
        if(gpio_num < 40)
        {
            if(gpio_intr_status & BIT(gpio_num))
            {
                //This is an isr handler, you should post an event to process it in RTOS queue.
                switch (gpio_num)
                {
                    case BTN_CONFIG_PIN:
                    {
                        s_TimerCountInSecond = 0;
                        xTaskCreate(gotoConfigMode, "gotoConfigMode", 4096, NULL, tskIDLE_PRIORITY, NULL);
                        break;
                    }
                    default:
                    {
                        char mesg[128];
                        sprintf(mesg, "Low PIN: Dont handle this interrupt gpio %d", (int)gpio_num);
                        xTaskCreate(showMessage, "showinfo", 1024, (void *)mesg, tskIDLE_PRIORITY, NULL);
                        break;
                    }
                }
            }
            else if(gpio_intr_status_h & BIT(gpio_num))
            {
                //This is an isr handler, you should post an event to process it in RTOS queue.
                switch (gpio_num)
                {
                    case SW_1_PIN:
                    {
                        char buffer[2];
                        sprintf(buffer, "%d", CONTROLLER_CHANNEL_1);
                        xTaskCreate(toggleChannel, "toggleChannel", 4096, (void *)buffer, tskIDLE_PRIORITY, NULL);
                        break;
                    }
                    case SW_2_PIN:
                    {
                        char buffer[2];
                        sprintf(buffer, "%d", CONTROLLER_CHANNEL_2);
                        xTaskCreate(toggleChannel, "toggleChannel", 4096, (void *)buffer, tskIDLE_PRIORITY, NULL);
                        break;
                    }
                    default:
                    {
                        char mesg[128];
                        sprintf(mesg, "HIGH PIN: Dont handle this interrupt gpio %d", (int)gpio_num);
                        xTaskCreate(showMessage, "showinfo", 1024, (void *)mesg, tskIDLE_PRIORITY, NULL);
                        break;
                    }
                }
            }
        }
        else
        {
            if(gpio_intr_status_h & BIT(gpio_num - 39))
            {
                char mesg[128];
                sprintf(mesg, "OVER PIN: Dont handle this interrupt gpio %d", (int)(gpio_num - 39));
                xTaskCreate(showMessage, "showinfo", 1024, (void *)mesg, tskIDLE_PRIORITY, NULL);
            }
        }
    } while(++gpio_num < GPIO_PIN_COUNT);
}

bool sys_ctrl::resetFactory()
{
    Config::getInstance()->restore();
    return file_storage::factoryReset();
}
void sys_ctrl::rebootSystem(uint8_t seconds)
{
    ESP_LOGI("System", "rebootSystem in %d", seconds);
    if(seconds > 0)
    {
        char* counter = new char[4];
        sprintf(counter, "%d", seconds);
        xTaskCreate(sys_ctrl::rebootTask, "sys_ctrl::rebootTask", 2048, (void*) counter, tskIDLE_PRIORITY+1, NULL);
    }
    else
    {
        ESP_LOGI("System", "Reboot system now");
        esp_restart();
    }
};

void sys_ctrl::rebootTask(void* data)
{
    uint8_t counter = 0;
    if(data == NULL)
    {
        counter = 3;
    }
    else
    {
        char* pointer = (char*)data;
        counter = atoi(pointer);
        delete[] pointer;
    }
    while(counter)
    {
        ESP_LOGI("System", "Reboot in %d seconds...", counter);
        delay_ms(1000);
        --counter;
    }

    esp_restart();
    vTaskDelete(NULL);
};