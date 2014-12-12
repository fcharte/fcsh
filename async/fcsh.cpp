/**
 *	@file	fcsh.cpp
 *	@date 	abril-mayo 2007
 *  @author Francisco Charte Ojeda
 *	@brief 	Implementaci�n de la clase FcSh
 */
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "fcsh.hpp"

/** @brief Funci�n auxiliar para generar una matriz char* a partir de un vector STL */
char** StlACpp(vector<string>& Parametros)
{
    char** argv = new char*[Parametros.size()+1];

	unsigned i;
    for (i=0; i<Parametros.size(); i++) {
        argv[i] = new char[Parametros[i].size()+1];
        strncpy(argv[i], Parametros[i].c_str(), Parametros[i].size()+1);
    }
    argv[i] = NULL; // el �ltimo puntero ha de ser NULL para que execvp funcione

	return argv;	
}

/*
 * Constructor
 * 
 * Inicializa el objeto aplicaci�n 
 */
FcSh::FcSh() : _nComando(0), _nAsincronos(0), _Semaforo(new TSemaforo(1)), 
              _MensajesPendientes(new stack<string>()) 
{
    // Controla la pulsaci�n de Control-C
	signal(SIGINT, GestorControlC); 

	// Muestro unas breves indicaciones sobre el funcionamiento del int�rprete
	system("clear");
	cout << endl << "Bienvenido a fcsh (Francisco Charte Shell 1.0 8-D)" << endl << endl
	     << "Introduce los comandos a ejecutar como lo har�as habitualmente en Linux," << endl
	     << "separando cada argumento y metacar�cter con espacios." << endl << endl 
	     << "Puedes utilizar los metacaracteres < y > para redireccionar entrada y salida," << endl
	     << "combin�ndolos si interesa, as� como el metacar�cter | para crear una" << endl  
	     << "interconexi�n entre dos procesos. No se pueden combinar < y/o > con |." << endl << endl
	     << "Disponiendo el car�cter & al final de la l�nea de comandos �sta se ejecutar� " 
	     << "en segundo plano, recibi�ndose una notificaci�n a medida que terminen." << endl << endl
	     << "Para salir de fcsh utiliza el comando 'exit'" << endl << endl;
}

/* 
 * Ejecutar
 * 
 * Este m�todo act�a como el nudo central del shell, mostrando el indicador, 
 * solicitando la l�nea de comandos, analiz�ndola y proces�ndola como corresponda.
 * 
 */
int FcSh::Ejecutar()
{
	bool Salir = false, Asincrono;
	
	do {
		string Comando, ArchivoIn, ArchivoOut;
		vector<string> Parametros, Pipe;

        // Si hay mensajes pendientes de procesar, mostrarlos antes de llegar al indicador de sistema
        _Semaforo->Wait();		
 		while(!_MensajesPendientes->empty()) {
			cout << endl << _MensajesPendientes->top() << endl;
			_MensajesPendientes->pop();
			--_nAsincronos;
		}
		_Semaforo->Signal();
		
		MostrarPrompt(); // Se muestra el indicador de entrada
		Comando = LeerComando(); // Se recupera una l�nea de comando
		// Se analiza su contenido
		if(AnalizaLineaComandos(Comando, Asincrono, Parametros, ArchivoIn, ArchivoOut, Pipe))
		    // y se procesa el comando 
			Salir = ProcesaComando(Comando, Asincrono, Parametros, ArchivoIn, ArchivoOut, Pipe);
	} while(!Salir);
	
	return 0;
}

/*
 * MostrarPrompt
 * 
 * M�todo encargado de mostrar el indicador del shell.
 * 
 */
void FcSh::MostrarPrompt()
{
	cout << "[" << setw(3) << ++_nComando << " (" << _nAsincronos << ")] -> ";
}

/*
 * LeerComando
 * 
 * Este m�todo recupera de la entrada est�ndar la l�nea de comandos a ejecutar, que
 * devuelve como resultado en un string.
 * 
 */
string FcSh::LeerComando()
{
	string Linea;
	
	getline(cin, Linea);
	return Linea;
}

/*
 * AnalizaLineaComandos
 * 
 * Este m�todo toma como entrada la l�nea de comandos completa y un vector de cadenas en
 * el que se devolver� la lista de par�metros. El valor de retorno, de tipo bool, indica
 * si se ha introducido algo o la l�nea de comandos estaba vac�a.
 * 
 */
bool FcSh::AnalizaLineaComandos(string& Comando, bool& Asincrono, vector<string>& Parametros, string& ArchivoIn, string& ArchivoOut, vector<string>& Pipe)
{
	stringstream Entrada(Comando); // Tratamos la cadena de entrada como un flujo o stream
	string Elemento;
	vector<string>* Destino = &Parametros;
		
	Asincrono = false;
	while(!Asincrono && Entrada >> Elemento) // Vamos obteniendo las palabras de la cadena
		// Comprobamos la aparici�n de <, > y |
		switch(Elemento[0]) {
			case '<': // Tras el car�cter < estar� el nombre de archivo
			    Entrada >> ArchivoIn;
				break;			    
			case '>':
			    Entrada >> ArchivoOut;
			    break;
			case '|':
			    Destino = &Pipe;
			    break; 
			case '&':
			    Asincrono = true;
			    break;
			default:  // por defecto 
	    		Destino->push_back(Elemento); // las almacenamos como elementos individuales del vector
		}
	
	if(Parametros.size())
		Comando = Parametros[0]; // El primer elemento es el ejecutable

	return Parametros.size();
}

/* 
 * ProcesaComando
 * 
 * Funci�n encargada de analizar el comando y procesarlo como corresponda
 * 
 */
bool FcSh::ProcesaComando(string Comando, bool Asincrono, vector<string>& Parametros, string ArchivoIn, string ArchivoOut, vector<string>& Pipe)
{
	// Primero procesar los comandos internos del int�rprete
	if(Comando == "exit") return true;
	
	// No es un comando interno, as� que creo un nuevo proceso o varios, seg�n se precise
	
    // Compruebo si hay una interconexi�n con otro programa
	if(Pipe.size()) {
		int fds[2];
		pipe(fds); // Creo la tuber�a sin nombre para conectar dos procesos
		
		int f1 = fork();
		if(!f1) { // Primer hijo
			close(fds[0]); // Cierro el canal de lectura en la interconexi�n
			dup2(fds[1], STDOUT_FILENO); // y desv�o la salida est�ndar a la escritura en la tuber�a
			
			char** argv = StlACpp(Parametros); // Par�metros correspondientes a este programa
	        // sustituyo el proceso actual por el del comando indicado
	 	    if(execvp(Comando.c_str(), argv) == -1) {
	 	    	// teniendo en cuenta un posible fallo
	 	    	cout << "Fallo al intentar ejecutar " << Comando << endl;
	 	    	exit(-1);
	 	    }
		}
		
		int f2 = fork();
		if(!f2) { // segundo hijo
			close(fds[1]); // Cierro el canal de escritura en la interconexi�n
			dup2(fds[0], STDIN_FILENO); // y desv�o la entrada est�ndar a la lectura de la tuber�a
			
			char** argv = StlACpp(Pipe); // Par�metros correspondientes a este programa
	        // sustituyo el proceso actual por el del comando indicado
	 	    if(execvp(Pipe[0].c_str(), argv) == -1) {
	 	    	// teniendo en cuenta un posible fallo
	 	    	cout << "Fallo al intentar ejecutar " << Pipe[0] << endl;
	 	    	exit(-1);
	 	    }
		}
		
		// El padre cierre la lectura y escritura en la tuber�a y espera si es necesario
		close(fds[0]);
		close(fds[1]);
		
		if(Asincrono) { // Si la ejecuci�n es as�ncrona
		   HCP h1(new TParHCP(f1, Comando, _Semaforo, _MensajesPendientes));  // utilizar un hilo para controlar cada proceso
		   h1.Ejecutar();
		   HCP h2(new TParHCP(f2, Pipe[0], _Semaforo, _MensajesPendientes));
		   h2.Ejecutar();
		   _nAsincronos += 2;
		} else {
			wait(NULL);
			wait(NULL);
		}
		
		return false;
	}
	
	// No hay interconexi�n, solamente se ejecuta un programa y se tienen en cuenta redireccionamientos
	int f = fork();
	if(!f) { // Si �ste es el proceso hijo
		
		// Obtengo en una matriz de punteros a char los par�metros
        char** argv = StlACpp(Parametros);
        
		// Compruebo si hay redireccionamiento de entrada
		if(!ArchivoIn.empty()) {
			int fIn = open(ArchivoIn.c_str(), O_RDONLY);
			if(fIn == -1) {
				cout << "Fallo al abrir el archivo " << ArchivoIn << endl;
				exit(-1);
			}
			dup2(fIn, STDIN_FILENO);
		}
		
		// Compruebo si hay redireccionamiento de salida
		if(!ArchivoOut.empty()) {
			int fOut = open(ArchivoOut.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if(fOut == -1) {
				cout << "Fallo al crear el archivo " << ArchivoOut << endl;
				exit(-1);
			}
			dup2(fOut, STDOUT_FILENO);
		}

        // sustituyo el proceso actual por el del comando indicado
 	    if(execvp(Comando.c_str(), argv) == -1) {
 	    	// teniendo en cuenta un posible fallo
 	    	cout << "Fallo al intentar ejecutar " << Comando << endl;
 	    	exit(-1);
 	    }
	} else { // Si �ste es el proceso padre
		if(Asincrono) { // Si la ejecuci�n es as�ncrona
		   HCP h(new TParHCP(f, Comando, _Semaforo, _MensajesPendientes));  // utilizar un hilo para controlar el proceso
		   h.Ejecutar();
		   _nAsincronos++;
		} else
		   wait(NULL); // esperar a que termine el hijo si no se ha solicitado ejecuci�n as�ncrona
	}
		
	return false; // No se quiere salir del int�rprete
}

/* ------------------------- M�todos de la clase HCP -------------------------- */
void HCP::CodigoHilo()
{
	TParHCP* p = (TParHCP *)_parametros; // Convierto los par�metros a estructura TParHCP
	int CodSalida;
	
	// Esperar hasta que termine el pid del proceso en segundo plano
	waitpid(p->Pid, &CodSalida, 0);
	
	stringstream Mensaje;
	// Compongo el mensaje con los datos de salida
    Mensaje << "Proceso " << p->Comando << " (pid:" << p->Pid 
            << ") finalizado con c�digo de salida " << CodSalida;
            
    // y lo introduzco en la pila de mensajes pendientes
    p->Semaforo->Wait();
    p->Mensajes->push(Mensaje.str());
    p->Semaforo->Signal();	
}
