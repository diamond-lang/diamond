debug: src/*.c src/*.h
	clang -g -fsanitize=address,undefined,null -fomit-frame-pointer src/*.c -o diamond

release: src/*.c src/*.h
	clang -o3 src/*.c -o diamond