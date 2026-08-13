/*
 * Serial.cpp
 *
 *  Created on: May 12, 2020
 *      Author: zacharypina
 */

#include <string.h>
#include "Serial.h"
#include "Serial_private.h"

#ifdef HAL_UART_MODULE_ENABLED
#ifdef UART_USE_HAL
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
#endif
#endif

/* Bound on how long write() will spin waiting for TX buffer space. This is a
 * spin count, not a calibrated time -- it exists solely so a stalled link,
 * or a write() made before begin()/with interrupts disabled, can't hang the
 * caller (and therefore the rest of the control loop) forever. */
#define TX_FULL_TIMEOUT_ITERATIONS 1000000UL

static HardwareSerial* usart1Iface = NULL;
static HardwareSerial* usart2Iface = NULL;
// static HardwareSerial* usart3Iface = NULL;
// static HardwareSerial* uart4Iface = NULL;
// static HardwareSerial* uart5Iface = NULL;

HardwareSerial::HardwareSerial()
{
	// _initialized = 0;
	// _handleDef = handle.Instance;
	// _pins = alt;
	// _priority = priority;

	// if(_handleDef == USART1)
	// 	usart1Iface = this;

	// if(_handleDef == USART2)
	// 	usart2Iface = this;

	// if(_handleDef == USART3)
	// 	usart3Iface = this;

	// if(_handleDef == UART4)
	// 		uart4Iface = this;

	// if(_handleDef == UART5)
	// 		uart5Iface = this;


}

uint8_t HardwareSerial::isInitialized(void)
{
	return _initialized;
}

//void store_char(unsigned char c, ring_buffer *buffer);
#ifdef HAL_UART_MODULE_ENABLED
#ifdef UART_USE_HAL
void HardwareSerial::begin(Board_UART_Handle huart, DMA_HandleTypeDef *hdma)
{
	_handleDef = huart;
	_handle_DMA = hdma;

	if(_handleDef->Instance == USART1)
		usart1Iface = this;

	if(_handleDef->Instance == USART2)
		usart2Iface = this;

	if(_handle_DMA != nullptr)
	{
		_enableDMA = true;
	}

	if(_enableDMA)
	{
		__HAL_DMA_DISABLE_IT(_handle_DMA, DMA_IT_HT);
	}
	else
	{
		/* Enable UART error and RX interrupts */
		_handleDef->Instance->CR3 |= USART_CR3_EIE;
		__HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);
	}

	_initialized = 1;
}
#else
void HardwareSerial::begin(Board_UART_Handle huart)
{
	_handleDef = huart;

	if(_handleDef == USART1)
		usart1Iface = this;

	if(_handleDef == USART2)
		usart2Iface = this;

	/* Enable UART error and RX interrupts */
	_handleDef->CR3 |= USART_CR3_EIE;
	LL_USART_EnableIT_RXNE(_handleDef);

	_initialized = 1;
}
#endif
#endif
int HardwareSerial::available(void)
{
	return (uint16_t)(UART_BUFFER_SIZE + _rx_head - _rx_tail) % UART_BUFFER_SIZE;
}

/* reads the data in the rx_buffer and increment the tail count in rx_buffer */
int HardwareSerial::read(void)
{
	// if the head isn't ahead of the tail, we don't have any characters
	if(_rx_head == _rx_tail)
	{
		return -1;
	}
	else
	{
		unsigned char c = _rx_buffer[_rx_tail];
		_rx_tail = (unsigned int)(_rx_tail + 1) % UART_BUFFER_SIZE;
		return c;
	}
}


// size_t HardwareSerial::write(const uint8_t *buffer, size_t size)
// {
//   size_t size_intermediate;
//   size_t ret = size;
//   size_t available = availableForWrite();
//   size_t available_till_buffer_end = SERIAL_TX_BUFFER_SIZE - _serial.tx_head;

//   _written = true;
//   if (isHalfDuplex()) {
//     if (_rx_enabled) {
//       _rx_enabled = false;
//       uart_enable_tx(&_serial);
//     }
//   }

//   // If the output buffer is full, there's nothing for it other than to
//   // wait for the interrupt handler to free space
//   while (!availableForWrite()) {
//     // nop, the interrupt handler will free up space for us
//   }

//   // HAL doesn't manage rollover, so split transfer till end of TX buffer
//   // Also, split transfer according to available space in buffer
//   while ((size > available_till_buffer_end) || (size > available)) {
//     size_intermediate = min(available, available_till_buffer_end);
//     write(buffer, size_intermediate);
//     size -= size_intermediate;
//     buffer += size_intermediate;
//     available = availableForWrite();
//     available_till_buffer_end = SERIAL_TX_BUFFER_SIZE - _serial.tx_head;
//   }

//   // Copy data to buffer. Take into account rollover if necessary.
//   if (_serial.tx_head + size <= SERIAL_TX_BUFFER_SIZE) {
//     memcpy(&_serial.tx_buff[_serial.tx_head], buffer, size);
//     size_intermediate = size;
//   } else {
//     // memcpy till end of buffer then continue memcpy from beginning of buffer
//     size_intermediate = SERIAL_TX_BUFFER_SIZE - _serial.tx_head;
//     memcpy(&_serial.tx_buff[_serial.tx_head], buffer, size_intermediate);
//     memcpy(&_serial.tx_buff[0], buffer + size_intermediate,
//            size - size_intermediate);
//   }

//   // Data are copied to buffer, move head pointer accordingly
//   _serial.tx_head = (_serial.tx_head + size) % SERIAL_TX_BUFFER_SIZE;

//   // Transfer data with HAL only is there is no TX data transfer ongoing
//   // otherwise, data transfer will be done asynchronously from callback
//   if (!serial_tx_active(&_serial)) {
//     // note: tx_size correspond to size of HAL data transfer,
//     // not the total amount of data in the buffer.
//     // To compute size of data in buffer compare head and tail
//     _serial.tx_size = size_intermediate;
//     uart_attach_tx_callback(&_serial, _tx_complete_irq, size_intermediate);
//   }

//   /* There is no real error management so just return transfer size requested*/
//   return ret;
// }

// size_t HardwareSerial::write(uint8_t c)
// {
//   uint8_t buff = c;
//   return write(&buff, 1);
// }

/* writes the data to the tx_buffer and increment the head count in tx_buffer */
size_t HardwareSerial::write(uint8_t c)
{
	if (!_initialized) return 0;

	uint16_t i = (_tx_head + 1) % UART_BUFFER_SIZE;

	/* If the output buffer is full, wait for the interrupt handler to empty
	 * it a bit, bounded so a stalled link can't hang the caller forever. */
	uint32_t waited = 0;
	while (i == _tx_tail)
	{
		if (++waited > TX_FULL_TIMEOUT_ITERATIONS) return 0;
	}

	_tx_buffer[_tx_head] = c;
	_tx_head = i;

#ifdef UART_USE_HAL
	__HAL_UART_ENABLE_IT(_handleDef, UART_IT_TXE); // Enable UART transmission interrupt
#else
	LL_USART_EnableIT_TXE(_handleDef);
#endif

	return 1;
}

/* Look for a particular string in the given buffer
 * @return 1, if the string is found and -1 if not found
 * @USAGE:: if (Look_for ("some string", buffer)) do something
 */
int HardwareSerial::look_for (char *str, char *buffertolookinto)
{
	size_t stringlength = strlen(str);
	size_t bufferlength = strlen(buffertolookinto);

	if (stringlength == 0 || stringlength > bufferlength) return -1;

	/* Bounded substring search -- the original index-chasing implementation
	 * had no bound on its scan and would read past the end of
	 * buffertolookinto (out-of-bounds read) whenever str wasn't present. */
	for (size_t start = 0; start <= bufferlength - stringlength; start++)
	{
		size_t matched = 0;
		while (matched < stringlength && buffertolookinto[start + matched] == str[matched]) matched++;
		if (matched == stringlength) return 1;
	}
	return -1;
}

void HardwareSerial::store_char(int8_t c)
{
	uint16_t i = (uint16_t)(_rx_head + 1) % UART_BUFFER_SIZE;

	// if we should be storing the received character into the location
	// just before the tail (meaning that the head would advance to the
	// current location of the tail), we're about to overflow the buffer
	// and so we don't write the character or advance the head.
	if(i != _rx_tail) {
		_rx_buffer[_rx_head] = c;
		_rx_head = i;
	}
}

void HardwareSerial::store_char(unsigned char c)
{
	uint16_t i = (uint16_t)(_rx_head + 1) % UART_BUFFER_SIZE;

	// if we should be storing the received character into the location
	// just before the tail (meaning that the head would advance to the
	// current location of the tail), we're about to overflow the buffer
	// and so we don't write the character or advance the head.
	if(i != _rx_tail) {
		_rx_buffer[_rx_head] = c;
		_rx_head = i;
	}
}

/* Print a number with any base
 * base can be 10, 8 etc*/
void HardwareSerial::printbase(long n, uint8_t base)
{
	char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
	char *s = &buf[sizeof(buf) - 1];

	*s = '\0';

	// prevent crash if called with base == 1
	if (base < 2) base = 10;

	do {
		unsigned long m = n;
		n /= base;
		char c = m - base * n;
		*--s = c < 10 ? c + '0' : c + 'A' - 10;
	} while(n);

	while(*s) write(*s++);
}

/* Peek for the data in the Rx Bffer without incrementing the tail count
 * Returns the character
 * USAGE: if (Uart_peek () == 'M') do something
 */
int HardwareSerial::peek()
{
	if(_rx_head == _rx_tail)
	{
		return -1;
	}
	else
	{
		return _rx_buffer[_rx_tail];
	}
}

/* Copy the data from the Rx buffer into the bufferr, Upto and including the entered string
 * This copying will take place in the blocking mode, so you won't be able to perform any other operations
 * Returns 1 on success and -1 otherwise
 * USAGE: while (!(Copy_Upto ("some string", buffer)));
 */
int HardwareSerial::copy_upto (char *string, char *buffertocopyinto, size_t buffersize)
{
	size_t so_far = 0;
	size_t len = strlen(string);
	size_t indx = 0;

	if (len == 0 || buffersize == 0) return -1;

	again:
	while (!available());
	while (peek() != string[so_far])
	{
		/* Bail out instead of writing past the caller's buffer -- the
		 * original loop wrote for as long as incoming data didn't match,
		 * which is an unbounded write driven entirely by external UART
		 * input (buffer overflow). */
		if (indx >= buffersize) return -1;
		buffertocopyinto[indx++] = (uint8_t)read();
		while (!available());
	}
	while (peek() == string[so_far])
	{
		so_far++;
		if (indx >= buffersize) return -1;
		buffertocopyinto[indx++] = (uint8_t)read();
		if (so_far == len) return 1;
		while (!available());
	}

	if (so_far != len)
	{
		so_far = 0;
		goto again;
	}

	return -1;
}

/* Copies the entered number of characters (blocking mode) from the Rx buffer into the buffer, after
 * some particular string is detected
 * Returns 1 on success and -1 otherwise
 * USAGE: while (!(Get_after ("some string", 6, buffer)));
 */
int HardwareSerial::get_after(char *string, uint8_t numberofchars, char *buffertosave)
{

	while (wait_for(string) != 1);
	for (int indx=0; indx<numberofchars; indx++)
	{
		while (!(available()));
		buffertosave[indx] = read();
	}
	return 1;
}

/* Wait until a paricular string is detected in the Rx Buffer
 * Return 1 on success and -1 otherwise
 * USAGE: while (!(Wait_for("some string")));
 */
int HardwareSerial::wait_for(char *string)
{
	int so_far =0;
	int len = strlen (string);

	again:
	while (!available());
	while (peek() != string[so_far])
	{
		/* Wait for more data before advancing again -- without this check
		 * the tail could be walked past the head once the buffer drained
		 * mid-scan, corrupting the ring buffer's head/tail relationship. */
		_rx_tail = (unsigned int)(_rx_tail + 1) % UART_BUFFER_SIZE;
		while (!available());
	}
	while (peek() == string [so_far])
	{
		so_far++;
		read();
		if (so_far == len) return 1;
		while (!available());
	}

	if (so_far != len)
	{
		so_far = 0;
		goto again;
	}

	if (so_far == len) return 1;
	else return -1;
}

//void Serial::print(char* str)
//{
//	while(*str) write(*str++);
//}

extern "C" void USART1_IRQHandler(void)
{
	/* usart1Iface is only set by begin(); guard against a spurious or
	 * leftover interrupt firing before begin() runs (or after a MSP
	 * de-init), which would otherwise dereference a NULL pointer. */
	if (usart1Iface != NULL)
	{
		usart1Iface->Uart_isr(BOARD_USART1);
	}
}

// extern "C" void USART2_IRQHandler(void)
// {
// 	usart2Iface->Uart_isr(&huart2);
// }

// extern "C" void USART3_IRQHandler(void)
// {
// 	usart3Iface->Uart_isr(USART3);
// }

// extern "C" void UART4_IRQHandler(void)
// {
// 	uart4Iface->Uart_isr(UART4);
// }

// extern "C" void UART5_IRQHandler(void)
// {
// 	uart5Iface->Uart_isr(UART5);
// }


#ifdef HAL_UART_MODULE_ENABLED
void HardwareSerial::Uart_isr (Board_UART_Handle huart)
{
#ifdef UART_USE_HAL
	USART_TypeDef *inst = huart->Instance;
#else
	USART_TypeDef *inst = huart;
#endif
	uint32_t isrflags   = READ_REG(inst->ISR);
	uint32_t cr1its     = READ_REG(inst->CR1);

	/* if DR is not empty and the Rx Int is enabled */
	if (((isrflags & USART_ISR_RXNE_RXFNE) != RESET) && ((cr1its & USART_CR1_RXNEIE_RXFNEIE) != RESET))
	{
		/******************
		 *  @note   PE (Parity error), FE (Framing error), NE (Noise error), ORE (Overrun
		 *          error) and IDLE (Idle line detected) flags are cleared by software
		 *          sequence: a read operation to USART_SR register followed by a read
		 *          operation to USART_DR register.
		 * @note   RXNE flag can be also cleared by a read to the USART_DR register.
		 * @note   TC flag can be also cleared by software sequence: a read operation to
		 *          USART_SR register followed by a write operation to USART_DR register.
		 * @note   TXE flag is cleared only by a write to the USART_DR register.

		 *********************/

		inst->ISR;                      /* Read status register */
		unsigned char c = inst->RDR;     /* Read data register */
		store_char(c);  	/* store data in buffer */
#ifdef UART_USE_HAL
		__HAL_UART_CLEAR_FLAG(huart, (UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF | UART_CLEAR_IDLEF));
#else
		LL_USART_ClearFlag_PE(inst);
		LL_USART_ClearFlag_FE(inst);
		LL_USART_ClearFlag_NE(inst);
		LL_USART_ClearFlag_ORE(inst);
		LL_USART_ClearFlag_IDLE(inst);
#endif
		return;
	}

	/*If interrupt is caused due to Transmit Data Register Empty */
	if (((isrflags & USART_ISR_TXE_TXFNF) != RESET) && ((cr1its & USART_CR1_TXEIE_TXFNFIE) != RESET))
	{
		if(_tx_head == _tx_tail)
		{
			// Buffer empty, so disable interrupts
#ifdef UART_USE_HAL
			__HAL_UART_DISABLE_IT(huart, UART_IT_TXE);
#else
			LL_USART_DisableIT_TXE(inst);
#endif
		}

		else
		{
			// There is more data in the output buffer. Send the next byte
			unsigned char c = _tx_buffer[_tx_tail];
			_tx_tail = (_tx_tail + 1) % UART_BUFFER_SIZE;

			inst->ISR;
			inst->TDR = c;

		}
		return;
	}

	/* Neither RXNE nor TXE was the cause: a standalone parity/framing/noise/
	 * overrun error (enabled via CR3_EIE) can assert without RXNE, and if
	 * left uncleared the hardware keeps re-requesting service, livelocking
	 * on this ISR forever. Clear it so a noisy line can't hang the MCU. */
#ifdef UART_USE_HAL
	__HAL_UART_CLEAR_FLAG(huart, (UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF | UART_CLEAR_IDLEF));
#else
	LL_USART_ClearFlag_PE(inst);
	LL_USART_ClearFlag_FE(inst);
	LL_USART_ClearFlag_NE(inst);
	LL_USART_ClearFlag_ORE(inst);
	LL_USART_ClearFlag_IDLE(inst);
#endif
}
#else
void HardwareSerial::Uart_isr (USART_TypeDef *huart)
{
	uint32_t isrflags = huart->SR;
	uint32_t cr1its   = huart->CR1;

	/* if DR is not empty and the Rx Int is enabled */
	if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
	{
		/******************
		 *  @note   PE (Parity error), FE (Framing error), NE (Noise error), ORE (Overrun
		 *          error) and IDLE (Idle line detected) flags are cleared by software
		 *          sequence: a read operation to USART_SR register followed by a read
		 *          operation to USART_DR register.
		 * @note   RXNE flag can be also cleared by a read to the USART_DR register.
		 * @note   TC flag can be also cleared by software sequence: a read operation to
		 *          USART_SR register followed by a write operation to USART_DR register.
		 * @note   TXE flag is cleared only by a write to the USART_DR register.

		 *********************/
		huart->SR;                       /* Read status register */
		unsigned char c = huart->DR;     /* Read data register */
		store_char(c);  // store data in buffer
		return;
	}

	/*If interrupt is caused due to Transmit Data Register Empty */
	if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
	{
		if(_tx_head == _tx_tail)
		{
			// Buffer empty, so disable interrupts
			//__HAL_UART_DISABLE_IT(huart, UART_IT_TXE);
			//USART_CR1_REG(_handleDef, ~(USART_CR1_TXEIE));
			huart->CR1 &= ~(USART_CR1_TXEIE);

		}

		else
		{
			// There is more data in the output buffer. Send the next byte
			unsigned char c = _tx_buffer[_tx_tail];
			_tx_tail = (_tx_tail + 1) % UART_BUFFER_SIZE;

			/******************
			 *  @note   PE (Parity error), FE (Framing error), NE (Noise error), ORE (Overrun
			 *          error) and IDLE (Idle line detected) flags are cleared by software
			 *          sequence: a read operation to USART_SR register followed by a read
			 *          operation to USART_DR register.
			 * @note   RXNE flag can be also cleared by a read to the USART_DR register.
			 * @note   TC flag can be also cleared by software sequence: a read operation to
			 *          USART_SR register followed by a write operation to USART_DR register.
			 * @note   TXE flag is cleared only by a write to the USART_DR register.

			 *********************/

			huart->SR;
			huart->DR = c;

		}
		return;
	}

	/* Neither RXNE nor TXE was the cause: clear any standalone error flag
	 * (PE/FE/NE/ORE/IDLE) via the SR-then-DR read sequence so it can't keep
	 * re-triggering this ISR. */
	(void)huart->SR;
	(void)huart->DR;
}
#endif
