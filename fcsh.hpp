/**
 *	@file	fcsh.hpp
 *	@date 	abril 2007
 *  @author Francisco Charte Ojeda
 *	@brief 	Definición de la clase FcSh
 */
#ifndef FCSH_H_
#define FCSH_H_

#include <iostream>
#include <iomanip>
#include <vector>

using namespace std;

/** @brief Clase que actúa como un intérprete de comandos básico */
class FcSh {
	int _nComando;
	
public:
	FcSh() : _nComando(0) {}
	int Ejecutar();
	
private:
	void MostrarPrompt();
	string LeerComando();
	bool AnalizaLineaComandos(string&, vector<string>&, string&, string&, vector<string>&);
	bool ProcesaComando(string, vector<string>&, string, string, vector<string>&);
};

#endif /*FCSH_H_*/
