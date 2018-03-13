#ifndef __FIFO_H__
#define __FIFO_H__

#include <stdint.h>

/* Поддержка синхронизации в очереди, позволяет обрабатывать очередь из нескольких потоков. */
#define FIFO_SYNC

typedef struct 
{
	int		head;
	int		tail;
	int		len;
	uint8_t	*buf;
} fifo_t, *pfifo_t;

/*
	fifo_init()
	
	Инициализация очереди fifo в буфере buf с длиной len.
 */
int fifo_init(fifo_t *fifo, uint8_t *buf, int len);

/*
	fifo_put()
	
	Положить 1 элемент в очередь.
 */
int fifo_put(fifo_t *fifo, uint8_t c);

/*
	fifo_putn()
	
	Положить n элементов в очередь.
 */
int fifo_putn(fifo_t *fifo, const uint8_t *dataaddr, int len);

/*
	fifo_puts()
	
	Положить строку в очередь.
 */
int fifo_puts(fifo_t *fifo, const uint8_t *datastr);

/*
	fifo_get()
	
	Взять 1 элемент из очереди.
 */
int fifo_get(fifo_t *fifo, uint8_t *pc);

/*
	fifo_getn()
	
	Взять n элементов из очереди.
 */
int fifo_getn(fifo_t *fifo, const uint8_t *dataaddr, int len);

/*
	fifo_avail_free()
	
	Возвращает свободное место в очереди.
 */
int  fifo_avail_free(fifo_t *fifo);

/*
	fifo_avail_data()
	
	Возвращает число элементов в очереди.
 */
int  fifo_avail_data(fifo_t *fifo);


/*
	fifo_flush()
	
	Очищает очередь.
 */
int fifo_flush(fifo_t *fifo);

/*
	fifo_free()
	
	Проводит деинициализацию очереди (в случае, если место для очереди 
	выделялось в куче, освобождает память.
 */
int fifo_free(fifo_t *fifo);

#endif // __FIFO_H__
