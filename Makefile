CXX = clang++
LDFLAGS = -L/usr/local/opt/llvm/lib -lclang
CPPFLAGS  = -g -Wall -I/usr/local/opt/llvm/include

TARGET = bindgen

all: $(TARGET)

$(TARGET): $(TARGET).cc
	$(CXX) $(LDFLAGS) $(CPPFLAGS) -o $(TARGET) $(TARGET).cc

clean:
	$(RM) $(TARGET)
