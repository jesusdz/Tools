.PHONY: main main-xwindow clean

main:
	g++ -g -o main main.cpp

main-xwindow:
	g++ -g -o main-xwindow  main-xwindow.cpp -lxcb

clean:
	rm main
