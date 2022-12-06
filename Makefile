.PHONY: default main main-xwindow clean

default: main-xwindow

main:
	g++ -g -o main main.cpp

main-xwindow:
	g++ -g -o main-xwindow  main-xwindow.cpp -DVK_NO_PROTOTYPES -lxcb

clean:
	rm -f main main-xwindow
