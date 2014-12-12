/**
 *	@file	thread.hpp
 *	@date 	mayo 2007
 *  @author Francisco Charte Ojeda
 *	@brief 	Definición de la clase THilo
 */
#ifndef THREAD_H_
#define THREAD_H_
#include <pthread.h>
using namespace std;
 
/** @brief Clase que encapsula la creación de un hilo y ejecución de un método */
class THilo {
	pthread_t _hilo;   /**< Identificador del hilo asociado a cada objeto de esta clase */
	bool _detached;
	
    /** Este método estático se encarga de conectar la llamada a función de phtread_create()
     * con la invocación al método protegido CodigoHilo() que deberá ser implementado por
     * cualquier clase derivada de ésta */
    static void* MetodoEntrada(void* par) 
     { ((THilo*)par)->CodigoHilo(); return NULL; }

public:
    /** El constructor recibe los parámetros que quieran asociarse al hilo */
    THilo(bool detached = false, void* parametros = NULL) : _detached(detached), _parametros(parametros)  {}
    /** Destructor virtual */
	virtual ~THilo() {}
	/** Método que pondrá en marcha la ejecución del hilo */
	virtual void Ejecutar() 
	  { pthread_create(&_hilo, NULL, MetodoEntrada, this);
	  	if(_detached) pthread_detach(_hilo); }
	/** Método para esperar a la ejecución del hilo */
	void Espera() 
	  { pthread_join(_hilo, NULL); }
	  
protected:
    /** Método virtual puro (abstracto) que contendrá el código útil del hilo */
    virtual void CodigoHilo() = 0;
	void* _parametros; /**< Parámetros que se le quieran enviar al hilo */
};
#endif /*THREAD_H_*/
