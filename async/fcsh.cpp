/**
 *	@file	fcsh.cpp
 *	@date 	abril-mayo 2007
 *  @author Francisco Charte Ojeda
 *	@brief 	Implementación de la clase FcSh
 */
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "fcsh.hpp"

/** @brief Función auxiliar para generar una matriz char* a partir de un vector STL */
char** StlACpp(vector<string>& Parametros)
{
    char** argv = new char*[Parametros.size()+1];

	unsigned i;
    for (i=0; i<Parametros.size(); i++) {
        argv[i] = new char[Parametros[i].size()+1];
        strncpy(argv[i], Parametros[i].c_str(), Parametros[i].size()+1);
    }
    argv[i] = NULL; // el último puntero ha de ser NULL para que execvp funcione

	return argv;	
}

/*
 * Constructor
 * 
 * Inicializa el objeto aplicación 
 */
FcSh::FcSh() : _nComando(0), _nAsincronos(0), _Semaforo(new TSemaforo(1)), 
              _MensajesPendientes(new stack<string>()) 
{
    // Controla la pulsación de Control-C
	signal(SIGINT, GestorControlC); 

	// Muestro unas breves indicaciones sobre el funcionamiento del intérprete
	system("clear");
	cout << endl << "Bienvenido a fcsh (Francisco Charte Shell 1.0 8-D)" << endl << endl
	     << "Introduce los comandos a ejecutar como lo harías habitualmente en Linux," << endl
	     << "separando cada argumento y metacarácter con espacios." << endl << endl 
	     << "Puedes utilizar los metacaracteres < y > para redireccionar entrada y salida," << endl
	     << "combinándolos si interesa, así como el metacarácter | para crear una" << endl  
	     << "interconexión entre dos procesos. No se pueden combinar < y/o > con |." << endl << endl
	     << "Disponiendo el carácter & al final de la línea de comandos ésta se ejecutará " 
	     << "en segundo plano, recibiéndose una notificación a medida que terminen." << endl << endl
	     << "Para salir de fcsh utiliza el comando 'exit'" << endl << endl;
}

/* 
 * Ejecutar
 * 
 * Este método actúa como el nudo central del shell, mostrando el indicador, 
 * solicitando la línea de comandos, analizándola y procesándola como corresponda.
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
		Comando = LeerComando(); // Se recupera una línea de comando
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
 * Método encargado de mostrar el indicador del shell.
 * 
 */
void FcSh::MostrarPrompt()
{
	cout << "[" << setw(3) << ++_nComando << " (" << _nAsincronos << ")] -> ";
}

/*
 * LeerComando
 * 
 * Este método recupera de la entrada estándar la línea de comandos a ejecutar, que
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
 * Este método toma como entrada la línea de comandos completa y un vector de cadenas en
 * el que se devolverá la lista de parámetros. El valor de retorno, de tipo bool, indica
 * si se ha introducido algo o la línea de comandos estaba vacía.
 * 
 */
bool FcSh::AnalizaLineaComandos(string& Comando, bool& Asincrono, vector<string>& Parametros, string& ArchivoIn, string& ArchivoOut, vector<string>& Pipe)
{
	stringstream Entrada(Comando); // Tratamos la cadena de entrada como un flujo o stream
	string Elemento;
	vector<string>* Destino = &Parametros;
		
	Asincrono = false;
	while(!Asincrono && Entrada >> Elemento) // Vamos obteniendo las palabras de la cadena
		// Comprobamos la aparición de <, > y |
		switch(Elemento[0]) {
			case '<': // Tras el carácter < estará el nombre de archivo
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
 * Función encargada de analizar el comando y procesarlo como corresponda
 * 
 */
bool FcSh::ProcesaComando(string Comando, bool Asincrono, vector<string>& Parametros, string ArchivoIn, string ArchivoOut, vector<string>& Pipe)
{
	// Primero procesar los comandos internos del intérprete
	if(Comando == "exit") return true;
	
	// No es un comando interno, así que creo un nuevo proceso o varios, según se precise
	
    // Compruebo si hay una interconexión con otro programa
	if(Pipe.size()) {
		int fds[2];
		pipe(fds); // Creo la tubería sin nombre para conectar dos procesos
		
		int f1 = fork();
		if(!f1) { // Primer hijo
			close(fds[0]); // Cierro el canal de lectura en la interconexión
			dup2(fds[1], STDOUT_FILENO); // y desvío la salida estándar a la escritura en la tubería
			
			char** argv = StlACpp(Parametros); // Parámetros correspondientes a este programa
	        // sustituyo el proceso actual por el del comando indicado
	 	    if(execvp(Comando.c_str(), argv) == -1) {
	 	    	// teniendo en cuenta un posible fallo
	 	    	cout << "Fallo al intentar ejecutar " << Comando << endl;
	 	    	exit(-1);
	 	    }
		}
		
		int f2 = fork();
		if(!f2) { // segundo hijo
			close(fds[1]); // Cierro el canal de escritura en la interconexión
			dup2(fds[0], STDIN_FILENO); // y desvío la entrada estándar a la lectura de la tubería
			
			char** argv = StlACpp(Pipe); // Parámetros correspondientes a este programa
	        // sustituyo el proceso actual por el del comando indicado
	 	    if(execvp(Pipe[0].c_str(), argv) == -1) {
	 	    	// teniendo en cuenta un posible fallo
	 	    	cout << "Fallo al intentar ejecutar " << Pipe[0] << endl;
	 	    	exit(-1);
	 	    }
		}
		
		// El padre cierre la lectura y escritura en la tubería y espera si es necesario
		close(fds[0]);
		close(fds[1]);
		
		if(Asincrono) { // Si la ejecución es asíncrona
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
	
	// No hay interconexión, solamente se ejecuta un programa y se tienen en cuenta redireccionamientos
	int f = fork();
	if(!f) { // Si éste es el proceso hijo
		
		// Obtengo en una matriz de punteros a char los parámetros
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
	} else { // Si éste es el proceso padre
		if(Asincrono) { // Si la ejecución es asíncrona
		   HCP h(new TParHCP(f, Comando, _Semaforo, _MensajesPendientes));  // utilizar un hilo para controlar el proceso
		   h.Ejecutar();
		   _nAsincronos++;
		} else
		   wait(NULL); // esperar a que termine el hijo si no se ha solicitado ejecución asíncrona
	}
		
	return false; // No se quiere salir del intérprete
}

/* ------------------------- Métodos de la clase HCP -------------------------- */
void HCP::CodigoHilo()
{
	TParHCP* p = (TParHCP *)_parametros; // Convierto los parámetros a estructura TParHCP
	int CodSalida;
	
	// Esperar hasta que termine el pid del proceso en segundo plano
	waitpid(p->Pid, &CodSalida, 0);
	
	stringstream Mensaje;
	// Compongo el mensaje con los datos de salida
    Mensaje << "Proceso " << p->Comando << " (pid:" << p->Pid 
            << ") finalizado con código de salida " << CodSalida;
            
    // y lo introduzco en la pila de mensajes pendientes
    p->Semaforo->Wait();
    p->Mensajes->push(Mensaje.str());
    p->Semaforo->Signal();	
}
