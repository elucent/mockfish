CXX := clang++
CXXFLAGS := -std=c++11 -Os -nostdlib++

clean:
	rm -f main chess.o

main: main.cpp chess.o
	${CXX} ${CXXFLAGS} $^ -o $@

chess.o: chess.cpp chess.h
	${CXX} ${CXXFLAGS} -c $< -o $@