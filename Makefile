storage:	client.cpp	server.cpp	preprocess.cpp	preprocess.h
			g++ -lstdc++ -lcrypto -lm -o client preprocess.cpp client.cpp
			g++ -lstdc++ -lcrypto -lm -o server preprocess.cpp server.cpp