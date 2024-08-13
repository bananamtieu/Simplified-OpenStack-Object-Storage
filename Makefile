storage:	client.cpp	server.cpp	preprocess.cpp	storageHelper.cpp
			g++ -lstdc++ -lcrypto -lm -o client client.cpp
			g++ -lstdc++ -lcrypto -lm -o server preprocess.cpp storageHelper.cpp server.cpp
clean:
			rm -f *.o storage