.PHONY: default main main_xwindow clean

default: main_xwindow

main:
	g++ -g -o main main.cpp

main_xwindow:
	g++ -g -o main_xwindow  main_xwindow.cpp -DVK_NO_PROTOTYPES -lxcb

clean:
	rm -f main main_xwindow
