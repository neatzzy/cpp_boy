# Nome do executável
TARGET = emulator

# Compilador e standard de C++
CXX = g++
CXXFLAGS = -std=c++23 -Wall

# Caminhos do Homebrew (Apple Silicon)
INCLUDES = -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lSDL2

# Todos os ficheiros .cpp do seu projeto
SRCS = main.cpp cpu.cpp memory.cpp cartridge.cpp timer.cpp ppu.cpp

# A regra principal
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o $(TARGET) $(LDFLAGS)

# Regra para limpar os ficheiros compilados
clean:
	rm -f $(TARGET)