/**
 *	@file	semaph.hpp
 *	@date 	mayo 2007
 *  @author Francisco Charte Ojeda
 *	@brief 	Definici�n de la clase TSemaforo
 */
#ifndef SEMAPH_HPP_
#define SEMAPH_HPP_
#include <semaphore.h>

/** @brief Clase que encapsula un sem�foro */
class TSemaforo {
	sem_t _semaforo;

public:
    // El constructor inicializa el sem�foro con el valor correspondiente
    TSemaforo(int Inicial = 0) { sem_init(&_semaforo, 0, Inicial); }
    // El destructor libera el sem�foro
    ~TSemaforo() { sem_destroy(&_semaforo); }
    
    // M�todos para obtener y liberar el sem�foro
    int Wait() { return sem_wait(&_semaforo); }
    int Signal() { return sem_post(&_semaforo); }
};

#endif /*SEMAPH_HPP_*/
