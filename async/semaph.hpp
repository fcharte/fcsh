/**
 *	@file	semaph.hpp
 *	@date 	mayo 2007
 *  @author Francisco Charte Ojeda
 *	@brief 	Definición de la clase TSemaforo
 */
#ifndef SEMAPH_HPP_
#define SEMAPH_HPP_
#include <semaphore.h>

/** @brief Clase que encapsula un semáforo */
class TSemaforo {
	sem_t _semaforo;

public:
    // El constructor inicializa el semáforo con el valor correspondiente
    TSemaforo(int Inicial = 0) { sem_init(&_semaforo, 0, Inicial); }
    // El destructor libera el semáforo
    ~TSemaforo() { sem_destroy(&_semaforo); }
    
    // Métodos para obtener y liberar el semáforo
    int Wait() { return sem_wait(&_semaforo); }
    int Signal() { return sem_post(&_semaforo); }
};

#endif /*SEMAPH_HPP_*/
