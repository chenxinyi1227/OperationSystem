src=*.c
test:${src}
	gcc ${src} -o test -lpthread

g:${src}
	gcc -g ${src} -o gtest -lpthread

clean:
	@rm -rf test gtest 