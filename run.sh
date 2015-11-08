rm a.out a.png
cc $(pkg-config --cflags cairo) calendar.c $(pkg-config --libs cairo) &&
	./a.out &&
	xdg-open a.png
