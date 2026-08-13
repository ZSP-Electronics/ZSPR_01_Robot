/*
 * VcpSerialInterface.cpp
 *
 *  Created on: 2015. okt. 25.
 *      Author: peti
 */

#include "VcpSerialInterface.h"
#include "AbstractUpLayer.h"

#define LOG_PRINTF(...)

VcpSerialInterface* VcpSerialInterface::instance = NULL;

static int8_t SerialIface_CDC_Receive_FS (uint8_t* Buf, uint32_t *Len);
static int8_t CDC_Callback_Receive_FS (uint8_t* Buf, uint32_t *Len);

VcpSerialInterface::VcpSerialInterface(USBD_HandleTypeDef* usbDevice, USBD_CDC_ItfTypeDef* fops, uint16_t txBufferSize)
{
	this->usbDevice = usbDevice;
	this->fops = fops;

	this->txBuffer = new uint8_t[txBufferSize];
	this->rxBuffer = new uint8_t[txBufferSize];
	this->txBufferSize = txBufferSize;
	this->txPosition = 0;
	this->txOverrunCount = 0;
	this->_rx_head = 0;
	this->_rx_tail = 0;
	instance = this;
}

VcpSerialInterface* VcpSerialInterface::getExistingInstance()
{
	return instance;
}

VcpSerialInterface::~VcpSerialInterface()
{
	delete[] txBuffer;
}

int VcpSerialInterface::available(void)
{
	return (uint16_t)(txBufferSize + _rx_head - _rx_tail) % txBufferSize;
}

void VcpSerialInterface::listen(bool protocol)
{
	if(protocol)
		fops->Receive = SerialIface_CDC_Receive_FS;
	else
		fops->Receive = CDC_Callback_Receive_FS;
}

bool VcpSerialInterface::isOpen() {
	return (usbDevice != NULL);
}

void VcpSerialInterface::handler() {
	// USB CDC internal TX buffer limitation
	static const uint16_t PacketSize = 256;

	if (usbDevice == NULL) {
		return;	// cannot use VCP before the USB device is connected
	}

	if (txPosition != 0) {
		// first packet start address
		uint8_t* data = txBuffer;
		while(data < &txBuffer[txPosition]) {
			// packet length is number of remaining bytes or max packet size
			uint16_t packetLen = MIN(PacketSize, &txBuffer[txPosition] - data);
			// transmit current packet (!non-blocking call!)
			CDC_Transmit_FS(data, packetLen);

			// get handle to USB CDC device
			USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*) usbDevice->pClassData;
			// wait for CDC to finish transmission
			while (hcdc->TxState == 1);

			data += packetLen;
		}
		txPosition = 0;
	}

	// create log message that will be sent out on the next call
	if (txOverrunCount > 0) {
		LOG_PRINTF("TX overrun %d", txOverrunCount);
		txOverrunCount = 0;
	}
}

bool VcpSerialInterface::writeBytes(const uint8_t* bytes, uint16_t length) {
	if (txPosition + length > txBufferSize) {
		txOverrunCount += length;
		return false;
	}
	memcpy(txBuffer + txPosition, bytes, length);
	txPosition += length;
	return true;
}

bool VcpSerialInterface::receiveBytes(const uint8_t* bytes, uint16_t len) {
	USBD_CDC_ReceivePacket(usbDevice);
	if (uplayer == NULL) {
		return false;
	}
	return (uplayer->receiveBytes(bytes, len));
}

bool VcpSerialInterface::readBytes(const uint8_t* bytes, uint16_t len) {
	USBD_CDC_ReceivePacket(usbDevice);
	if (_rx_head + len > txBufferSize)
	{
		//txOverrunCount += len;
		return false;
	}

	memcpy(rxBuffer + _rx_head, bytes, len);
	_rx_head += len;

	return true;
}

int VcpSerialInterface::read(void)
{
	// if the head isn't ahead of the tail, we don't have any characters
	if(_rx_head == _rx_tail)
	{
		return -1;
	}
	else
	{
		unsigned char c = rxBuffer[_rx_tail];
		_rx_tail = (unsigned int)(_rx_tail + 1) % txBufferSize;
		return c;
	}
}

bool VcpSerialInterface::read(uint8_t *pData, uint16_t length)
{
	if(_rx_head == _rx_tail)
	{
		return false;
	}

	if ((pData == NULL) || (length == 0U))
	{
		return false;
	}

	uint8_t  *pdata8bits = pData;

	while(length > 0)
	{
		*pdata8bits = (uint8_t)rxBuffer[_rx_tail];
		length--;
		pdata8bits++;
	}
	_rx_tail = (unsigned int)(_rx_tail + length) % txBufferSize;
	return true;
}

/* Function for abstract class */
static int8_t SerialIface_CDC_Receive_FS(uint8_t* Buf, uint32_t *Len) {
	bool success = VcpSerialInterface::getExistingInstance()->receiveBytes(Buf, (uint16_t) *Len);

	return success ? USBD_OK : USBD_BUSY;
}

/* Function for basic serial reading */
static int8_t CDC_Callback_Receive_FS(uint8_t* Buf, uint32_t *Len) {
	bool success = VcpSerialInterface::getExistingInstance()->readBytes(Buf, (uint16_t) *Len);

	return success ? USBD_OK : USBD_BUSY;
}
