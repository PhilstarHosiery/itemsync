all:
	clang++ -o itemsync -std=c++17 -O2 -I/usr/local/include -L/usr/local/lib -l pthread -lboost_system -lpqxx -lpq src/fileFinder.cpp src/dbfReader.cpp src/itemsync.cpp

install:
	cp itemsync /storage/philstar/bin/phsystem/

clean:
	rm itemsync
