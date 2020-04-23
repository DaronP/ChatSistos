chat:
	protoc -I. --cpp_out=. mensaje.proto
	g++ -I. -lprotobuf -pthread -std=c++11 -o client client.cpp mensaje.pb.cc
	g++ -I. -lprotobuf -pthread -std=c++11 -o server Server.cpp mensaje.pb.cc
	
