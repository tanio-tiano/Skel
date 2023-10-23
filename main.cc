#include "include/global.hh"
#include <getopt.h>

std::mutex mtx;

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

void contadorPalabras(const std::vector<std::string>& porcion, std::map<std::string, int>& histograma) {
    
    for(const std::string& linea : porcion){
        // Dividir cada línea del texto en palabras
        std::vector<std::string> words = splitText(linea);
            
        // Incrementar el conteo de cada palabra en el histograma
        for (const std::string &word : words) {
            std::lock_guard<std::mutex> lock(mtx);
            histograma[word]++;
        }
    }
}

static struct option opciones_largas[] = {
    {"threads", required_argument, 0, 't'},
    {"file", required_argument, 0, 'f'},
    {"help", no_argument, 0, 'h'}
};

int main(int argc, char *argv[]) {
    int opt;
    int option_index;

    int hilos;
    std::string fileName;

    while ((opt = getopt_long(argc, argv, "t:f:h", opciones_largas, &option_index)) != -1) {
        switch (opt) {
            case 't':
                try {
                    hilos = std::stoi(optarg);
                }
                catch(const std::invalid_argument& e) {
                    std::cerr << "Numero de threads invalido" << '\n';
                    return(EXIT_FAILURE);
                }
                break;

            case 'f':
                fileName = optarg;
                break;

            case 'h':
                std::cout << "Modo de uso: ./histograma_mt --threads N --file FILENAME [--help]" << '\n'
                << "--threads: cantidad de threads a utilizar. Si es 1, entonces ejecuta la version secuencial."
                << '\n' << "--file : archivo a procesar." << '\n' << "--help : muestra este mensaje y termina." << std::endl;
                return(EXIT_SUCCESS);
        }
    }

    if (hilos == 0) {
        std::cerr << "No se especifico el numero de hilos" << std::endl;
        return(EXIT_FAILURE);
    }
    std::cout << "Numero de threads: " << hilos << std::endl;

	std::vector<std::string> textInMemory;
    std::ifstream file(fileName); 
	
    if (!file) {
        std::cerr << "No se pudo abrir el archivo." << std::endl;
        return(EXIT_FAILURE);
    }
	
    std::string line;
    while (std::getline(file, line)) {
		textInMemory.push_back(line);
	}
	file.close();

    std::vector<std::thread> threads;
    std::vector<std::map<std::string, int>> histogramas(hilos);

    int inicio = 0;
    int lineasPorHilo = textInMemory.size() / hilos;

    for (int i = 0; i < hilos; i++) {
        int fin = (i == hilos - 1) ? textInMemory.size() : inicio + lineasPorHilo;
        std::vector<std::string> porcion(textInMemory.begin() + inicio, textInMemory.begin() + fin);

        threads.emplace_back(contadorPalabras, porcion, std::ref(histogramas[i]));
        inicio = fin;
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::map<std::string, int> wordHistogram;
    for (const auto& histogramaHilo : histogramas) {
        for (const auto& par : histogramaHilo) {
            std::lock_guard<std::mutex> lock(mtx);
            wordHistogram[par.first] += par.second;
        }
    }

    // Mostrar el histograma de palabras
    for (const auto &entry : wordHistogram) {
        std::cout << entry.first << ": " << entry.second << std::endl;
    }


    return(EXIT_SUCCESS);
}


