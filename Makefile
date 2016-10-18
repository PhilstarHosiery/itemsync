all:
	clang++ -o itemsync -std=c++11 -O3 -I/usr/local/include -L/usr/local/lib -lboost_system -lcryptopp -lpqxx -lpq src/fileFinder.cpp src/dbfReader.cpp src/itemsync.cpp

install:
	cp itemsync /storage/philstar/bin/phsystem/

clean:
	rm itemsync
