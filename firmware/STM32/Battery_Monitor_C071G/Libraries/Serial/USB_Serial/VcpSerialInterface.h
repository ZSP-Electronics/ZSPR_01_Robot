/*
 * VcpSerialInterface.h
 *
 *  Created on: 2015. okt. 25.
 *      Author: peti
 */

#ifndef VCPSERIALINTERFACE_H_
#define VCPSERIALINTERFACE_H_

#include "AbstractSerialInterface.h"
#include "usbd_cdc_if.h"
#include "stm32fxxx.h"

class VcpSerialInterface: public AbstractSerialInterface {
private:
	USBD_HandleTypeDef* usbDevice;
	USBD_CDC_ItfTypeDef* fops;

	uint8_t *txBuffer;
	uint8_t *rxBuffer;
	uint16_t txBufferSize;
	uint16_t txPosition;
	volatile uint16_t _rx_head;
	volatile uint16_t _rx_tail;
	uint16_t txOverrunCount;

	static VcpSerialInterface* instance;

public:
	static VcpSerialInterface* getExistingInstance();

	VcpSerialInterface(USBD_HandleTypeDef* usbDevice, USBD_CDC_ItfTypeDef* fops, uint16_t txBufferSize);
	virtual ~VcpSerialInterface();

	virtual int available(void);
	virtual void listen(bool protocol);
	virtual bool isOpen();
	virtual void handler();
	virtual int read(void);
	virtual bool read(uint8_t *pData, uint16_t length);
	virtual bool writeBytes(const uint8_t* bytes, uint16_t length);
	bool receiveBytes(const uint8_t* bytes, uint16_t len);
	bool readBytes(const uint8_t* bytes, uint16_t len);
};

#endif /* VCPSERIALINTERFACE_H_ */
