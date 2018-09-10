# Use GNU compiler
CC = gcc -g -Wall -Werror -pthread

PARTS = part1 part2

all: git $(PARTS)

$(PARTS): %: %.c %.h
	$(CC) $(WARNINGS) $< -o $@

.PHONY: git
# DO NOT MODIFY
git:
	git add *.c *.h Makefile >> .local.git.out || echo
	git commit -a -m "Commit lab 4" >> .local.git.out || echo
	git push origin HEAD:master

.PHONY: git
clean:
	rm -f $(PARTS)
