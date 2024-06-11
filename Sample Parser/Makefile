default: build
	$(CC) -O3 -o build/arabiki64 main.c datastructs.c mesh.c material.c animation.c parser.c optimizer.c dlist.c output.c opengl.c gbi.c -lm

build:
	mkdir -p $@

clean:
	rm -r -f build