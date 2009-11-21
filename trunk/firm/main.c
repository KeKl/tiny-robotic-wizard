#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <string.h>

#include "map.h"
#include "usbdrv.h"


typedef struct _BLINFO
{
	uint8_t		signature[3];	//< @brief ���̃f�o�C�X�̃V�O�l�`��
	uint8_t		pagesize;			//< @brief Flash�̃y�[�W�T�C�Y
	uint16_t	flashend;			//< @brief Flash�̏I���A�h���X
} BLINFO;

BLINFO _blinfo = 
	{
		{ 0x1e, 0x93, 0x08 }, 
		SPM_PAGESIZE, 
		FLASHEND,
	};

/**
 * @brief �������݂Ȃǂ�Bootloader�ɑ΂��郊�N�G�X�g��\��
 */
typedef struct _BLPAGE
{
	uint16_t	page;									//< @brief �������ݐ�̃y�[�W
	uint8_t		data[SPM_PAGESIZE];	//< @brief �y�[�W�ɏ������ރf�[�^
} BLPAGE;

/**
 * @brief Bootloader�p��ReportDescriptor
 */
PROGMEM uint8_t _bootHidReportDescriptor[30] = 
	{
		0x06, 0x00, 0xff,						//	USAGE_PAGE (Vendor Defined)
		0x09, 0x01,									//	USAGE (Vendor Usage 1)
		0xa1, 0x02,									//	COLLECTION (Logical)
			0x85, 0x01,								//		REPORT_ID	(0x01)
			0x75, 0x08,								//		REPORT_SIZE (8)
			0x95, sizeof(BLINFO),			//		REPORT_COUNT (BLINFO)
			0x09, 0x00,								//		USAGE (Undefined)
			0xb2, 0x02, 0x01,					//		FEATURE (Data,Var,Abs,Buf)
		
			0x85, 0x02,								//		REPORT_ID	(0x02)
			0x75, 0x08,								//		REPORT_SIZE (8)
			0x95, sizeof(BLPAGE),			//		REPORT_COUNT (BLPAGE)
			0x09, 0x00,								//		USAGE (Undefined)
			0xb2, 0x02, 0x01,					//		FEATURE (Data,Var,Abs,Buf)
		0xc0,												//	END_COLLECTION
	};

/**
 * @brief Bootloader�p�̃f�o�C�X�f�B�X�N���v�^
 */
PROGMEM uint8_t _deviceDescriptor[0x12] = 
	{
		0x12,								// 0x12 bytes
		USBDESCR_DEVICE,		// Device Descriptor
		0x01, 0x01,					// USB Specification Version = Version 1,1
		0x00, 0x00, 0x00,		// Class 0, Subclass 0, Protocol 0
		0x08,								// Max packet size = 8 bytes
		USB_CFG_VENDOR_ID,	// Vendor ID
		USB_CFG_DEVICE_ID,	// Product ID
		0x00, 0x01,					// Device release number = 0x100
		0x01,								// Index of string descriptor which contains manufacturer's name
		0x02,								// Index of string descriptor which contains product's name
		0x00,								// Index of string descriptor which contains serial number
		0x01,								// Number of configurations = 1
	};
/**
 * @brief Bootloader�p�̃R���t�B�M�����[�V�����f�X�N���v�^
 */
PROGMEM uint8_t _configurationDescriptor[41] = 
	{
		0x09,										// 0x09 bytes
		USBDESCR_CONFIG,				// Configuration Descriptor
		41, 00,									// Total length of descriptor
		0x01,										// Number of interfaces = 1
		0x01,										// Configuration index = 1
		0x00,										// Description of this configuration = none
		USBATTR_BUSPOWER,				// Bus powered device
		USB_CFG_MAX_BUS_POWER/2,// Maximum power
		//**** begining of inlined Interface Descriptor ****
		0x09,										// 0x09 bytes
		USBDESCR_INTERFACE,			// Interface Descriptor
		0x00,										// Interface #0
		0x00,										// Alternate setting
		0x01,										// Number of endpoints = 1
		0x03, 0x00, 0x00,				// Class3 (HID), Subclass0 (noclass), Protocol0 (noprotocol)
		0x00,										// Description of this interface = none
		//**** begining of inlined HID Descriptor ****
		0x09,										// 0x09 bytes
		USBDESCR_HID,						// HID Descriptor
		0x01, 0x01,							// HID Specification Version = Version 1.1
		0x00,										// Country code = 0 (none)
		0x01,										// Number of HID class descriptors = 1
		USBDESCR_HID_REPORT,		// Report Descriptor
		sizeof(_bootHidReportDescriptor) & 0xff,	sizeof(_bootHidReportDescriptor) >> 8, // 
		//**** begining of inlined Endpoint Descriptor ****
		0x07,										// 0x07 bytes
		USBDESCR_ENDPOINT,			// Endpoint Descriptor
		0x81,	0x03,							// Interrupt-IN1 Endpoint
		0x08, 0x00,							// Maximum packed size = 0x08 bytes
		0x0a,										// Polling interval = 0x0a ms
		//**** begining of inlined Endpoint Descriptor ****
		0x07,										// 0x07 bytes
		USBDESCR_ENDPOINT,			// Endpoint Descriptor
		0x01,	0x03,							// Interrupt-OUT1 Endpoint
		0x08,	0x00,							// Maximum packed size = 0x08 bytes
		0x0a,										// Polling interval = 0x0a ms
	};


//static	uint8_t _reportId;				//< @brief Report��ID
uint8_t	volatile _currentAddress;	//< @brief ���ݓ]�����̃A�h���X
uint8_t	volatile _bytesRemaining;	//< @brief �c�]����
uint8_t	volatile _buffer[sizeof(BLPAGE) + 1];	//< @brief �R���g���[���]���p�o�b�t�@

uint8_t volatile _exitBootloader = 0;	//< @brief �u�[�g���[�_�𔲂��ăA�v���P�[�V�����Ɉڍs����ꍇ��true



#ifndef	USE_ASM_DESCRIPTOR
/**
 * @brief �v�����ꂽ�f�B�X�N���v�^��ݒ肷��
 */
uint8_t	usbFunctionDescriptor(usbRequest_t *rq)
{
	if( rq->wValue.bytes[1] == USBDESCR_DEVICE )
	{
		usbMsgPtr = _deviceDescriptor;
		return sizeof(_deviceDescriptor);
	}
	else if( rq->wValue.bytes[1] == USBDESCR_CONFIG )
	{
		usbMsgPtr = _configurationDescriptor;
		return sizeof(_configurationDescriptor);
	}
	else if( rq->wValue.bytes[1] == USBDESCR_HID_REPORT )
	{
		usbMsgPtr = _bootHidReportDescriptor;;
		return sizeof(_bootHidReportDescriptor);
	}

	return 0;
}
#endif	//USE_ASM_DESCRIPTOR

/**
 * @brief Control-In �]���̃f�[�^�]���������s��
 */
uint8_t   usbFunctionRead(uint8_t *data, uint8_t len)
{
	/*
	if(len > _bytesRemaining)
		len = _bytesRemaining;
	
	memcpy(data, (uint8_t*)&_blinfo + _currentAddress, len);
	
	_currentAddress += len;
	_bytesRemaining -= len;
	
	return len;
	*/
	// �����usbFunctionRead��p���Ȃ�
	return 1;
}

/**
 * @brief Control-Out �]���̃f�[�^�]���������s��
 */
uint8_t   usbFunctionWrite(uint8_t *data, uint8_t len)
{
	if(_bytesRemaining == 0)	// �����c���Ă��Ȃ��D�]���I���D
		return 1;
	if(len > _bytesRemaining)
		len = _bytesRemaining;
	
	if( _currentAddress == 0 && data[0] == 0x01 )
	{
		// ReportID 1�ɑ΂��鏑�����݂��������D
		// ���[�U�[�A�v���P�[�V�����ɏ������ڍs����D
		_exitBootloader = 1;
		return 1;
	}

	memcpy(_buffer + _currentAddress, data, len);

	_currentAddress += len;
	_bytesRemaining -= len;
	
	if( _bytesRemaining == 0 )
	{
		BLPAGE* blp = (BLPAGE*)(_buffer + 1);
		uint16_t page = blp->page;
		// �y�[�W������
		cli();
		boot_page_erase(page);
		sei();
		boot_spm_busy_wait();
		// �y�[�W�����[�h
		for(uint16_t i = 0; i < SPM_PAGESIZE; i += 2)
		{
			cli();
			boot_page_fill(page + i, *(uint16_t*)&blp->data[i]);
			sei();
		}
		// �y�[�W����������
		cli();
		boot_page_write(page);
		sei();
		boot_spm_busy_wait();

		return 1;
	}
	
	return 0;
}

#ifdef USE_ASM_SETUP
extern usbMsgLen_t usbFunctionSetup(uint8_t data[8]);
#else
/**
 * @brief �z�X�g����̃��N�G�X�g�ɑ΂��鏈�����s���D
 */
usbMsgLen_t usbFunctionSetup(uint8_t data[8])
{
	usbRequest_t    *rq = (void *)data;

	if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS)	// HID Class Request
	{
		if(rq->bRequest == USBRQ_HID_GET_REPORT)
		{
			// �ǂݏo����ReportID = 1�̃��|�[�g�����D
			// ������8�o�C�g�ȉ��Ȃ̂ŁC�����œ]������D
			usbMsgPtr = (uint8_t*)&_blinfo;
			return sizeof(_blinfo);
			/*
			_bytesRemaining = sizeof(BLINFO);
			_currentAddress = 0;
			return USB_NO_MSG;
			 */
		}
		else if(rq->bRequest == USBRQ_HID_SET_REPORT)
		{
			// �������݂�ReportID = 2�̃��|�[�g�����D
			_bytesRemaining = rq->wLength.bytes[0];
			_currentAddress = 0;
			return USB_NO_MSG;
		}
	}
	return 0;
}
#endif

#ifndef	USE_ASM_MAIN
/**
 * @brief ���C���ł�
 */
int main(void)
{
	// ���荞�݃x�N�^���u�[�g�Z�N�V�����̐擪�ɐݒ�
	MCUCR = _BV(IVCE);
	MCUCR = _BV(IVSEL);
	
	wdt_enable(WDTO_1S);
	
	// �|�[�g�̏���������
	DDR_BOOTMODE &= ~_BV(BIT_BOOTMODE);
	PORT_BOOTMODE |= _BV(BIT_BOOTMODE);
	#ifdef BIT_TESTLED
	DDR_TESTLED |= _BV(BIT_TESTLED);
	PORT_TESTLED &= ~_BV(BIT_TESTLED);
	#endif

	usbInit();
	
	usbDeviceDisconnect();
	uint8_t i = 0;
	while(--i > 0)
		_delay_ms(1);
	usbDeviceConnect();
	wdt_reset();

	//
	//*
	if(PIN_BOOTMODE & _BV(BIT_BOOTMODE))
	{
	//*/
#ifdef BIT_TESTLED
		PORT_TESTLED |= _BV(BIT_TESTLED);
#endif
		sei();
		
		while(!_exitBootloader)
		{
			wdt_reset();
			usbPoll();
		}
		
		cli();
	//*
	}
	//*/
	// �A�v���P�[�V���������s����
	usbDeviceDisconnect();
	wdt_disable();
	boot_rww_enable();
	MCUCR = _BV(IVCE);
	MCUCR = 0;
	//
	((void(*)())0)();
	return 0;
}
#endif	//USE_ASM_MAIN
