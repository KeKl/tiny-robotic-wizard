#include <avr/io.h>
#include <avr/interrupt.h>

#include "queue.h"

#define NULL	((void*)0)

/***
 * �w�肵���L���[������������
 */
void queueClear(Queue_t* q)
{
	q->rd = q->wr = 0;
}
/***
 * �w�肵���L���[����1�o�C�g�ǂ�
 */
uint8_t queueRead(Queue_t* q)
{
	while(q->rd == q->wr);
	uint8_t sreg = SREG;
	cli();
	uint8_t data = q->buf[q->rd];
	q->rd = (q->rd + 1) & q->mask;
	if( sreg & _BV(SREG_I) ) sei();
	return data;
}
/***
 * �w�肵���L���[�̐擪1�o�C�g���擾����D
 * �ǂݏo���|�C���^�͐�ɐi�܂Ȃ��D
 */
uint8_t queuePeek(Queue_t* q)
{
	while(queueIsEmpty(q));
	return q->buf[q->rd];
}
/***
 * �w�肵���L���[�̖�����1�o�C�g�ǉ�����D
 * �L���[�������ς��Œǉ��ł��Ȃ�������false
 * �ǉ��ł�����true��Ԃ�
 */
int8_t queueWrite(Queue_t* q, uint8_t c)
{
	uint8_t next = (q->wr + 1) & q->mask;
	if( next == q->rd ) return 0;
	
	uint8_t sreg = SREG;
	cli();
	q->buf[q->wr] = c;
	q->wr = next;
	if( sreg & _BV(SREG_I) ) sei();
	
	return 1;
}
#if 0
/***
 * �w�肵���L���[����o�b�t�@�ɓǂݍ���
 */
uint8_t queueReadBuffer(Queue_t* q, uint8_t* buf, uint8_t len)
{
	uint8_t bytesRead = 0;
	while(!queueIsEmpty(q) && len--)
	{
		*(buf++) = queueRead(q);
		bytesRead++;
	}
	return bytesRead;
}

/***
 * �w�肵���L���[����w�肵���������f�[�^��j������
 */
void queueRemove(Queue_t* q, uint8_t len)
{
	// �c�肪len��菭�Ȃ����ǂ����̃`�F�b�N�͏ȗ�
	q->rd = (q->rd + len) & q->mask;
}
#endif
