# Compilador por defecto
CXX ?= g++
CXXFLAGS = -Wall -Wextra -Werror -fopenmp -g -I$(HOME)/intel/oneapi/advisor/latest/include
INCLUDE = -I./src

# Optimizaciones seguras por defecto
CCXOPTIMIZATIONS = -O2 -ffast-math -funroll-loops -ftree-vectorize -march=native -funsafe-math-optimizations -fno-math-errno

# Comando de compilación
CXXCMD = $(CXX) $(CXXFLAGS) $(CCXOPTIMIZATIONS) $(INCLUDE)

# Archivos
headers = $(wildcard ./src/*.hpp)
sources = $(wildcard ./src/*.cpp)
objects_names = $(sources:./src/%.cpp=%)
objects = $(objects_names:%=./src/%.o)

mains = graphics/burned_probabilities_data graphics/fire_animation_data

# Regla por defecto
all: $(mains)

# Compilar .o
%.o: %.cpp $(headers)
	$(CXXCMD) -c $< -o $@

# Compilar ejecutables
$(mains): %: %.cpp $(objects) $(headers)
	$(CXXCMD) $< $(objects) -o $@

# Objetivo para usar g++
gcc:
	$(MAKE) all CXX=g++

# Objetivo para usar clang++
# Evita fast-math y fuerza -fno-finite-math-only
clang:
	$(MAKE) all CXX=clang++ CXXFLAGS="$(CXXFLAGS) -fno-finite-math-only"

# Descargar y descomprimir datos
data.zip:
	wget https://cs.famaf.unc.edu.ar/~nicolasw/data.zip

data: data.zip
	unzip data.zip

# Limpiar
clean:
	rm -f $(objects) $(mains)

.PHONY: all clean gcc clang data
