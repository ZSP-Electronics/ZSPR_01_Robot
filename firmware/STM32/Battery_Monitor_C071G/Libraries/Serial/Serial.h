/*
 * Serial.h
 *
 *  Created on: May 12, 2020
 *      Author: zacharypina
 */

#ifndef DRIVERS_INC_SERIAL_H_
#define DRIVERS_INC_SERIAL_H_

#include <inttypes.h>

//#include "stm32f1xx_hal.h"
//#include "stm32f1xx_hal_conf.h"
// #include "LL_uart.h"
#include "Stream.h"
#include "stm32c0xx_hal.h"
// #include "stm32fxxx.h"
// #include "Print.h"

#ifdef HAL_UART_MODULE_ENABLED
#include "usart.h"
#endif

/* change the size of the buffer */
#define UART_BUFFER_SIZE 128

class HardwareSerial : public Stream
{
public:
	HardwareSerial();

	#ifdef HAL_UART_MODULE_ENABLED
	void Uart_isr (Board_UART_Handle huart);
	#ifdef UART_USE_HAL
	void begin(Board_UART_Handle huart, DMA_HandleTypeDef *hdma = nullptr);
	#else
	void begin(Board_UART_Handle huart);
	#endif
	#else
	void Uart_isr (USART_TypeDef *huart);
	void begin();
	#endif


	void end();
	virtual int available(void);
	//virtual int availableForWrite(void);
	//virtual void flush(void);
	virtual int read(void);
	virtual size_t write(uint8_t);
    inline size_t write(unsigned long n)
    {
      return write((uint8_t)n);
    }
    inline size_t write(long n)
    {
      return write((uint8_t)n);
    }
    inline size_t write(unsigned int n)
    {
      return write((uint8_t)n);
    }
    inline size_t write(int n)
    {
      return write((uint8_t)n);
    }
    // size_t write(const uint8_t *buffer, size_t size);
	using Print::write; // pull in write(str) from Print
	//void print(char* str);
	virtual int look_for (char *str, char *buffertolookinto);
	void printbase(long n, uint8_t base);
	virtual int peek();
	virtual int copy_upto (char *string, char *buffertocopyinto, size_t buffersize);
	virtual int get_after(char *string, uint8_t numberofchars, char *buffertosave);
	virtual int wait_for(char *string);
	operator bool() { return true; }
	uint8_t isInitialized(void);

private:
#ifdef HAL_UART_MODULE_ENABLED
	Board_UART_Handle _handleDef;
	#ifdef UART_USE_HAL
	DMA_HandleTypeDef *_handle_DMA;
	#endif
	#endif

	volatile uint8_t _priority;
	volatile uint8_t _initialized = 0;
	volatile uint8_t _pins;
	volatile uint8_t _rx_buffer[UART_BUFFER_SIZE];
	volatile uint8_t _tx_buffer[UART_BUFFER_SIZE];
	volatile uint16_t _tx_head = 0;
	volatile uint16_t _tx_tail = 0;
	void store_char(int8_t c);
	void store_char(unsigned char c);

protected:
	volatile uint16_t _rx_head;
	volatile uint16_t _rx_tail;
	bool _enableDMA = false;
};

extern HardwareSerial Serial;
//extern HardwareSerial Serial1;

#endif /* DRIVERS_INC_SERIAL_H_ */
