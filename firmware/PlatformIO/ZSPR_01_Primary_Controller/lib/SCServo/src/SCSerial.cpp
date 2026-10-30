/*
 * SCSerial.h
 * FIT serial servo hardware interface layer program
 */

#include "SCSerial.h"

// How long readSCS() busy-waits for the next byte before yielding to the scheduler.
#define SCS_SPIN_US 2000

SCSerial::SCSerial()
{
	IOTimeOut = 100;
	pSerial = NULL;
	Err = 0;
}

SCSerial::SCSerial(u8 End):SCS(End)
{
	IOTimeOut = 100;
	pSerial = NULL;
	Err = 0;
}

SCSerial::SCSerial(u8 End, u8 Level):SCS(End, Level)
{
	IOTimeOut = 100;
	pSerial = NULL;
	Err = 0;
}

int SCSerial::readSCS(unsigned char *nDat, int nLen)
{
	int Size = 0;
	int ComData;
	unsigned long t_begin = millis();
	unsigned long t_spin = micros();
	unsigned long t_user;
	while(1){
		ComData = pSerial->read();
		if(ComData!=-1){
			if(nDat){
				nDat[Size] = ComData;
			}
			Size++;
			t_begin = millis();
			t_spin = micros();
		}else if(micros() - t_spin > SCS_SPIN_US){
			// A servo normally answers well inside SCS_SPIN_US; past that, block
			// for a tick so lower-priority tasks and the idle task get to run
			// instead of this loop spinning for up to IOTimeOut.
			vTaskDelay(1);
		}
		if(Size>=nLen){
			break;
		}
		t_user = millis() - t_begin;
		if(t_user>IOTimeOut){
			break;
		}
	}
	return Size;
}

int SCSerial::writeSCS(unsigned char *nDat, int nLen)
{
	if(nDat==NULL){
		return 0;
	}
	return pSerial->write(nDat, nLen);
}

int SCSerial::writeSCS(unsigned char bDat)
{
	return pSerial->write(&bDat, 1);
}

void SCSerial::rFlushSCS()
{
	while(pSerial->read()!=-1);
}

void SCSerial::wFlushSCS()
{
}