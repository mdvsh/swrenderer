CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -O2
TARGET := swrenderer
SRCS := main.cpp tgaimage.cpp model.cpp

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) *.tga

.PHONY: clean