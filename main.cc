#include <global.hh>
#include <getopt.h>


std::string removePunctuation(const std::string& word) {
    std::string result;
    for (char c : word) {
        if (std::isalpha(c)) {
            result += c;
        }
    }
    return(result);
}

// Función para dividir un texto en palabras
std::vector<std::string> splitText(const std::string &text) {
    std::vector<std::string> words;
    std::string word;
    std::istringstream iss(text);
    while (iss >> word) {
		
        //std::string lowercaseWord = word;
        for (char &c : word) {
            c = std::tolower(c);
        }
		
		word = removePunctuation(word);
        words.push_back(word);
    }
    return words;
}

int main(int argc, char *argv[]) {

    int opt;
    std::string fileName;
    int threads = 1; // Por defecto se utilizara un solo thread (es decir la solucion secuencial)

    // Se definen los argumentos que acepta el programa
    static struct option long_options[] = {
        {"threads", required_argument, 0, 't'},
        {"file", required_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    // Se procesan los argumentos utilizando la funcion getopt 
    
	while ((opt = getopt_long(argc, argv, "t:f:h", long_options, NULL)) != -1) {
        switch (opt) {
        // --threads N
        case 't':
            threads = std::stoi(optarg); // Se transforma el argumento a entero utilizando stoi (String to Integer)
            std::cout << "Se utilizarán " << threads << " threads." << std::endl;
            break;
        
        // --file FILENAME
        case 'f':
            fileName = optarg;
            break;
        // --help
        case 'h':
            std::cout << "Modo de Uso: " << argv[0] << " --threads N --file FILENAME [--help]" << std::endl;
            std::cout << "--threads: cantidad de threads a utilizar. Si es 1, entonces ejecuta la versión secuencial." << std::endl;
            std::cout << "--file: archivo a procesar." << std::endl;
            std::cout << "--help: muestra este mensaje y termina." << std::endl;
            return 0;

        default:
            break;
        }
    }

    // Se abre el archivo y se verifica que no haya errores
    std::ifstream file(fileName);
    if (!file) {
        std::cerr << "No se pudo abrir el archivo." << std::endl;
        return(EXIT_FAILURE);
    }
	
    std::vector<std::string> textInMemory;
    std::string line;

    
    while (std::getline(file, line)) { 
		textInMemory.push_back(line); 
	}
	file.close();
	
    std::map<std::string, int> wordHistogram;

    // Solucion secuencial
    if(threads == 1){
        for(auto line : textInMemory){ 
        // Dividir cada línea del texto en palabras
        std::vector<std::string> words = splitText(line); 
        
        // Incrementar el conteo de cada palabra en el histograma
            for (const std::string &word : words) {
			    wordHistogram[word]++;
            }
        }
    }
    else{

        // Solucion paralela
    
        int lineaXthread = textInMemory.size() / threads;   // Cantidad de lineas que va a procesar cada thread.
        int sobrantes = textInMemory.size() % threads;      // Cantidad de lineas que sobrantes de las lineas repartidas.
        int inicio = 0;                                     // Indice de inicio del thread. (El primer thread comienza en 0)
        int fin = lineaXthread;                             // Indice de fin del thread. (El primer thread termina en la cantidad de lineas que va a procesar)

        std::mutex mutex;                   // Mutex para evitar condicion de carrera al incrementar el conteo de una palabra en el histograma
        std::vector<std::thread> hilos;     // Vector de threads llamado hilos
        
        for(int i = 0; i < threads; i++){       // Se inicia el ciclo para crear cada thread
        
            // Logica para repartir las lineas por cada thread
            inicio = inicio + i * lineaXthread;
            fin = fin + i * lineaXthread;
            if(i == threads - 1){               // El ultimo thread se utiliza para procesar las lineas sobrantes
                fin += sobrantes;
            }

            // Se crea el thread utilizando la funcion threads.emplace_back
            hilos.emplace_back([&mutex, &wordHistogram, &textInMemory, inicio, fin](){
                for(int j = inicio; j < fin; j++){

                    // Rescatar una linea del texto en cada iteracion
                    auto line = textInMemory[j];                                     
                    // Dividir cada línea del texto en palabras
                    std::vector<std::string> words = splitText(line); 
        
                    // Incrementar el conteo de cada palabra en el histograma
                    for (const std::string &word : words) {
                        std::lock_guard<std::mutex> lock(mutex);    // Se utiliza un lock_guard para evitar que otro thread acceda al histograma para actualizarlo.
                        wordHistogram[word]++;
                    }
                }
            });
        }

        for(auto &thread : hilos){      // Se espera a que todos los threads terminen
            thread.join();
        }

    }
    
    // Mostrar el histograma de palabras
    for (const auto &entry : wordHistogram) {
        std::cout << entry.first << ": " << entry.second << std::endl;
    }

    return(EXIT_SUCCESS);
}


