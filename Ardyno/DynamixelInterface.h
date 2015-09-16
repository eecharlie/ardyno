

#ifndef DYNAMIXEL_INTERFACE_H
#define DYNAMIXEL_INTERFACE_H

#include <Arduino.h>
#include "Dynamixel.h"


template<class T>
static void setReadMode(T &aStream);

template<class T>
static void setWriteMode(T &mStream);


template<class T>
class DynamixelInterfaceImpl:public DynamixelInterface
{
	private:
	/** \brief Switch stream to read (receive)) mode */
	void readMode(){setReadMode(mStream);}
	
	/** \brief Switch stream to write (send) mode */
	void writeMode(){setWriteMode(mStream);}
	
	public:
	
	/**
	 * \brief Constructor
	 * \param[in] aStreamController : stream controller that abstract real stream
	*/
	DynamixelInterfaceImpl(T &aStream):mStream(aStream)
	{}
	
	/**
	 * \brief Start interface
	 * \param[in] aBaud : Baudrate
	 *
	 * Start the interface with desired baudrate, call once before using the interface
	*/
	void begin(unsigned long aBaud)
	{
		mStream.begin(aBaud);
		mStream.setTimeout(50); //warning : response delay seems much higher than expected for some operation (eg writing eeprom)
		readMode();
	}
	
	/**
	 * \brief Send a packet on bus
	 * \param[in] aPacket : Packet to send
	 *
	 * The function wait for the packet to be completly sent (using Stream.flush)
	*/
	void sendPacket(const DynamixelPacket &aPacket)
	{
		writeMode();
	
		mStream.write(0xFF);
		mStream.write(0xFF);
		mStream.write(aPacket.mID);
		mStream.write(aPacket.mLenght);
		mStream.write(aPacket.mInstruction);
		if(aPacket.mLenght>2)
		{
			mStream.write(aPacket.mData, aPacket.mLenght-2);
		}
		mStream.write(aPacket.mCheckSum);
		mStream.flush();
		readMode();
	}
	/**
	 * \brief Receive a packet on bus
	 * \param[out] aPacket : Received packet. mData field must be previously allocated
	 *
	 * The function wait for a new packet on the bus. Timeout depends of timeout of the underlying stream.
	 * Return error code in case of communication error (timeout, checksum error, ...)
	*/
	void receivePacket(DynamixelPacket &aPacket)
	{
		static uint8_t buffer[3];
		if(mStream.readBytes(buffer,2)<2)
		{
			aPacket.mStatus=DYN_STATUS_COM_ERROR | DYN_STATUS_TIMEOUT;
			return;
		}
		if(mStream.readBytes(buffer, 3)<3)
		{
			aPacket.mStatus=DYN_STATUS_COM_ERROR | DYN_STATUS_TIMEOUT;
			return;
		}
		aPacket.mID=buffer[0];
		aPacket.mLenght=buffer[1];
		aPacket.mStatus=buffer[2];
		if(aPacket.mLenght>2 && mStream.readBytes(reinterpret_cast<char*>(aPacket.mData), aPacket.mLenght-2)<(aPacket.mLenght-2))
		{
			aPacket.mStatus=DYN_STATUS_COM_ERROR | DYN_STATUS_TIMEOUT;
			return;
		}
		if(mStream.readBytes(reinterpret_cast<char*>(&(aPacket.mCheckSum)),1)<1)
		{
			aPacket.mStatus=DYN_STATUS_COM_ERROR | DYN_STATUS_TIMEOUT;
			return;
		}
		if(aPacket.checkSum()!=aPacket.mCheckSum)
		{
			aPacket.mStatus=DYN_STATUS_COM_ERROR | DYN_STATUS_CHECKSUM_ERROR;
		}
	}
	
	private:
	
	T &mStream;
};

#endif
