#include "MessagingService.h"
#include "PPMDecoder/PPMDecoder.h"
#include "PXXEncoder/PXXEncoder.h"
#include "Settings/Settings.h"
#include "stdio.h"

#define MESSAGEID_SET_TXID 0x01  //["txid", "bind?"]
#define MESSAGEID_BIND 0x02      //[]
#define MESSAGEID_STOPBIND 0x03  //[]
#define MESSAGEID_RX_STATUS 0x04 //[txId, bind, channels]
#define MESSAGEID_TEST 0x36

MessagingService *MessagingService::shared;

void MessagingService::init(int rxPin, int txPin)
{
    MessagingService::shared = new MessagingService(rxPin, txPin);
}

void MessagingService::uart_callback()
{
    while (uart_is_readable(UART_ID))
    {
        uint8_t data = uart_getc(UART_ID);
        MessagingService::shared->receivedData(data);
    }
}

MessagingService::MessagingService(int rxPin, int txPin)
{
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(txPin, GPIO_FUNC_UART);
    gpio_set_function(rxPin, GPIO_FUNC_UART);

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);
    irq_set_exclusive_handler(UART_IRQ, MessagingService::uart_callback);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only, we don't care when the TX FIFO is empty
    uart_set_irq_enables(UART_ID, true, false);

    add_repeating_timer_ms(-250, timer_callback, NULL, &this->_timer);
}

bool MessagingService::timer_callback(repeating_timer_t *rt)
{
    uint16_t buffer[17];
    buffer[0] = PXXEncoder::shared->channel << 8 | PXXEncoder::shared->bind;
    for (int i = 0; i < 16; i++)
    {
        buffer[i + 1] = PPMDecoder::shared->ppm[i];
    }

    MessagingService::shared->sendMessage(MESSAGEID_RX_STATUS, (uint8_t *)buffer, 17 * 2);

    return true;
}

void MessagingService::receivedData(uint8_t data)
{
    switch (this->_state)
    {
    case IDLE:
        if (data == '$')
        {
            this->_state = HEADER_START;
        }
        break;
    case HEADER_START:
        if (data == 'T')
        {
            this->_state = HEADER;
        }
        break;
    case HEADER:
        this->_buffer[this->_offset++] = data;
        this->_checksum ^= data;
        if (this->_offset == sizeof(header_t))
        {
            header_t *header = (header_t *)&this->_buffer[0];
            if (header->size > BUFFER_SIZE)
            {
                this->_state = IDLE;
            }
            else
            {
                this->_message = header->message;
                this->_size = header->size;
                this->_offset = 0;
                this->_state = this->_size > 0 ? PAYLOAD : CHECKSUM; // If no payload - jump to checksum byte
            }
        }
        break;
    case PAYLOAD:

        this->_buffer[this->_offset++] = data;
        this->_checksum ^= data;
        if (this->_offset == this->_size)
        {
            this->_state = CHECKSUM;
        }
        break;
    case CHECKSUM:

        if (this->_checksum == data)
        {
            this->_state = RECEIVED;
        }
        else
        {
            this->_state = IDLE;
        }
        break;

    default:
        break;
    }

    if (this->_state == RECEIVED)
    {
        this->messageComplete();
        this->_state = IDLE;
    }

    if (this->_state == IDLE)
    {
        this->_offset = 0;
        this->_checksum = 0;
    }
}

void MessagingService::sendMessage(uint8_t messageId, uint8_t *buffer, uint16_t size)
{
#define CHECKSUM_STARTPOS 2
    uint8_t headerBuffer[16] = {'$', 'T'};
    uint8_t crcBuffer[2];
    int headerLength = 2;
    int crcLength = 0;
    header_t *header = (header_t *)&headerBuffer[headerLength];
    headerLength += sizeof(header_t);
    header->message = messageId;
    header->size = size;

    crcBuffer[crcLength] = createChecksum(0, headerBuffer + CHECKSUM_STARTPOS, headerLength - CHECKSUM_STARTPOS);
    crcBuffer[crcLength] = createChecksum(crcBuffer[crcLength], buffer, size);
    crcLength++;

    if (uart_is_writable(UART_ID))
    {
        for (int i = 0; i < headerLength; i++)
        {
            uart_putc_raw(UART_ID, headerBuffer[i]);
        }

        for (int i = 0; i < size; i++)
        {
            uart_putc_raw(UART_ID, buffer[i]);
        }

        for (int i = 0; i < crcLength; i++)
        {
            uart_putc_raw(UART_ID, crcBuffer[i]);
        }
    }
}

void MessagingService::messageComplete()
{
    switch (this->_message)
    {
    case MESSAGEID_SET_TXID:
        PXXEncoder::shared->channel = this->_buffer[0];
        Settings::shared->channel = this->_buffer[0];
        if (this->_offset > 1 && this->_buffer[1] > 0)
        {
            PXXEncoder::shared->bind = true;
        }
        break;
    case MESSAGEID_BIND:
        PXXEncoder::shared->bind = true;
        break;
    case MESSAGEID_STOPBIND:
        PXXEncoder::shared->bind = false;
        break;
    case MESSAGEID_TEST:
        break;
    }
}

uint8_t MessagingService::createChecksum(uint8_t checksum, const uint8_t *data, int len)
{
    while (len-- > 0)
    {
        checksum ^= *data++;
    }
    return checksum;
}