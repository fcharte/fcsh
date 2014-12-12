/**
 *	@file	fcsh.hpp
 *	@date 	abril-mayo 2007
 *  @author Francisco Charte Ojeda
 *	@brief 	Definición de las clases FcSh (shell) y HCP (hilo de control de proceso)
 */
#ifndef FCSH_H_
#define FCSH_H_

#include <stack>
#include <string>
#include <iostream>
#include <iomanip>
#include <vector>
using namespace std;

#include "thread.hpp" // Para utilizar la clase Thread
#include "semaph.hpp" // Para utilizar la clase Semaforo

/** @brief Estructura con datos que facilitará la comunicación entre el shell y los hilos de control de proceso */
struct TParHCP {
	TParHCP(int p, string c, TSemaforo* s, stack<string>* m) 
	  : Pid(p), Comando(c), Semaforo(s), Mensajes(m) { }
	int Pid; // pid del proceso a vigilar
	string Comando; // Comando ejecutado
	TSemaforo* Semaforo; // Semáforo para sincronizar el acceso a Mensajes
	stack<string>* Mensajes; // Pila a la que se añadirán los mensajes de finalización
};

/** @brief Clase que actúa como un intérprete de comandos básico */
class FcSh {
	int _nComando, _nAsincronos;
	TSemaforo* _Semaforo;
	stack<string>* _MensajesPendientes;
	
public:
	FcSh();
    ~FcSh() { delete _MensajesPendientes; }	
	int Ejecutar();
	
private:
	void MostrarPrompt();
	string LeerComando();
	bool AnalizaLineaComandos(string&, bool&, vector<string>&, string&, string&, vector<string>&);
	bool ProcesaComando(string, bool, vector<string>&, string, string, vector<string>&);
	static void GestorControlC(int) { cout << endl << "Utilice 'exit' para salir" << endl; }
};

/** @brief Clase de control de procesos en segundo plano */
class HCP : public THilo {
public:
    HCP(TParHCP* p) : THilo(true, (void *)p) {} // Se crea el hilo de control desligado de éste	
protected:
    virtual void CodigoHilo();
};

#endif /*FCSH_H_*/
