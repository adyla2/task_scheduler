DEFAULT: 1kEyes

1kEyes:
	g++ 1kEyes.cpp -o 1kEyes -lpthread -l sqlite3 -std=c++11 -Wwrite-strings

.PHONY: clean
clean:
	rm 1kEyes test.db

run: 1kEyes
	./1kEyes
