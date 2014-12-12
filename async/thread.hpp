/**
 *	@file	thread.hpp
 *	@date 	mayo 2007
 *  @author Francisco Charte Ojeda
 *	@brief 	Definici�n de la clase THilo
 */
#ifndef THREAD_H_
#define THREAD_H_
#include <pthread.h>
using namespace std;
 
/** @brief Clase que encapsula la creaci�n de un hilo y ejecuci�n de un m�todo */
class THilo {
	pthread_t _hilo;   /**< Identificador del hilo asociado a cada objeto de esta clase */
	bool _detached;
	
    /** Este m�todo est�tico se encarga de conectar la llamada a funci�n de phtread_create()
     * con la invocaci�n al m�todo protegido CodigoHilo() que deber� ser implementado por
     * cualquier clase derivada de �sta */
    static void* MetodoEntrada(void* par) 
     { ((THilo*)par)->CodigoHilo(); return NULL; }

public:
    /** El constructor recibe los par�metros que quieran asociarse al hilo */
    THilo(bool detached = false, void* parametros = NULL) : _detached(detached), _parametros(parametros)  {}
    /** Destructor virtual */
	virtual ~THilo() {}
	/** M�todo que pondr� en marcha la ejecuci�n del hilo */
	virtual void Ejecutar() 
	  { pthread_create(&_hilo, NULL, MetodoEntrada, this);
	  	if(_detached) pthread_detach(_hilo); }
	/** M�todo para esperar a la ejecuci�n del hilo */
	void Espera() 
	  { pthread_join(_hilo, NULL); }
	  
protected:
    /** M�todo virtual puro (abstracto) que contendr� el c�digo �til del hilo */
    virtual void CodigoHilo() = 0;
	void* _parametros; /**< Par�metros que se le quieran enviar al hilo */
};
#endif /*THREAD_H_*/
