# Une variable pour le compilateur
CC = gcc
# Une variable regroupant les options de compilation
CFLAGS = -g #-Wall 
# Une variable regroupant les options de l'edition de liens
LDFLAGS = 
# Une variable contenant le nom des executables a  generer
EXEC = mishell
# Une variable contenant la liste des fichiers sources
SRC = $(wildcard *.c)
# Une variable contenant la liste des fichiers objets
OBJ = $(SRC:.c=.o)

# $@ : Le nom de la cible
# $< : Le nom de la premiere dÃ©pendance
# $^ : La liste des dependances
# $? : La liste des dependances plus recentes que la cible
# $* : Le nom du fichier sans suffixe

all: $(EXEC)

mishell: $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)
	rm *.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o
	
mrproper: clean
	rm $(EXEC)
