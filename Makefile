torneo: torneo.cpp
	g++ -Wall -Wextra -std=c++17 -pthread -o torneo main.cpp

clean:
	rm -f torneo