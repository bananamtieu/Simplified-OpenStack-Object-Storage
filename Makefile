storage:	client.cpp	server.cpp	preprocess.cpp	preprocess.h
			g++ -lstdc++ -lcrypto -lm -o client client.cpp
			g++ -lstdc++ -lcrypto -lm -o server preprocess.cpp storageHelper.cpp server.cpp