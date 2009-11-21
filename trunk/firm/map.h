#ifndef __MAP_H__
/**
 * @file map.h
 * 
 * ���̃t�@�C����gpiomap.py�ɂ�莩���I�ɐ�������܂����D
 * �|�[�g�̊��蓖�Ă̐ݒ���s���܂��D
 */

#define	MAP_CC(a, b)	a ## b
#define	MAP_OUT(name)	MAP_CC(PORT, name)
#define	MAP_IN(name)	MAP_CC(PIN, name)
#define	MAP_DIR(name)	MAP_CC(DDR, name)

//definition for BOOTMODE
#define	PORTNAME_BOOTMODE	D
#define	BIT_BOOTMODE	1
#define	PORT_BOOTMODE	MAP_OUT(PORTNAME_BOOTMODE)
#define	PIN_BOOTMODE	MAP_IN(PORTNAME_BOOTMODE)
#define	DDR_BOOTMODE	MAP_DIR(PORTNAME_BOOTMODE)


#endif	//__MAP_H__
