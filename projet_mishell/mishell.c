#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <limits.h>
#include <ctype.h>
#define MAX_CHAR 100
#define PATH_MAX 4096
#define MAX_ARG 64
int lire(char *chaine, int longueur);
void viderBuffer();
int compte_mots(char* chaine);
int is_sep(char c);
int decoupe(char **tableau_elements, char *commande,char* decoupe);
void affiche_directory(char* last_dir, char* cwd);
void gestioncrtl_D();
int touch(char **liste_arg, int taille_args);


int main (){
    char commande[MAX_CHAR] = "";
    int testcommande=0;
    char *liste_arg[MAX_ARG];
    char hostname[128];
    char last_dir[PATH_MAX] = "";
    char cwd[PATH_MAX + 1]; /*current working directory : tableau pour le chemin*/
    char cwdtmp[PATH_MAX + 1];
    getcwd(cwd, PATH_MAX + 1 );
    gethostname(hostname, sizeof hostname);

    do{

        memset(liste_arg,0,MAX_ARG); /* on met a zero le tableau d'argument*/
        strcpy(cwdtmp,cwd); /* on copie le curent directory pour pas que strtock le modifie*/
        affiche_directory(last_dir,cwdtmp); /*fonction pour decouper la chaine du PATH*/

        printf("%s@%s:%s ",getlogin(),hostname,last_dir); /* on affiche le login le nom de la machine et le directory*/


        lire(commande, MAX_CHAR);/* lecture de l'entrée utilisateur*/
        gestioncrtl_D(); /* on regarde si on a pas touché sur crtl +D*/

        if(strlen(commande) == 0){ /* on vérifie si l'utilisateur ne rentre pas de commande*/
            printf("Rien écrit\n");
        }
        else{
            if(!strcmp(commande,"quit")){ /* si on rentre dans les arguments "quit" on quitte le programe*/
                exit(0);
            }
            int taille_args = decoupe(liste_arg,commande," ");/* on decoupe la commande avec les espaces*/
            int i;
            for (i=0; i<taille_args; i++){
                printf("%s\n",liste_arg[i]);
            }

            if(!strcmp(liste_arg[0], "touch")){
              touch(liste_arg, taille_args);
            }
            if(!strcmp(liste_arg[0],"cd")){ /* si la commande est cd */
                if (liste_arg[1] != NULL){ /* s'il a un 2eme argument*/
                    if( chdir( liste_arg[1] ) == 0 ) {
                        getcwd(cwd, PATH_MAX + 1 );
                    } else {
                        perror( liste_arg[1] );
                    }
                }
                else{ /* s'il n'y a pas de 2eme argument*/
                if( chdir(getenv("HOME")) == 0 ) { /* on va dans le home*/
                        getcwd(cwd, PATH_MAX + 1 );
                    } else {
                        perror( liste_arg[1] );
                    }
                }
            }else{
                pid_t fils; /* création du pid du processus*/

                fils = fork(); /* on fork le programe et on ajout du pid du fils dans la variable */
                if (fils == -1){ /* si le fork s'st mal passé*/
                    printf("fork error\n");
                    exit(EXIT_FAILURE);
                }
                else{
                    if (fils ==0){ /* si c'est le fils */

                        execvp(liste_arg[0],liste_arg); /* on execute la commande écrite*/

                    }
                    else{ /* si c'est le père*/
                        waitpid(fils,NULL,0); /* on atend le fils*/
                    }
                }
            }
        }

    }while(1);
    return 0;
}


void gestioncrtl_D(){
    if (feof(stdin)){ /* test de controle D*/
        printf("superrrrr\n");
        exit(0);
    }
}

void affiche_directory(char* last_dir, char* cwd){
    char *liste_dir[MAX_ARG];
    strcpy(last_dir,""); /* on remet a zero l'affichage */

    int nbdir = decoupe(liste_dir,cwd,"/"); /* on apelle la fonction qui decoupe avec les /*/

    int i;
    int nb_dir_affiche = 3;
    if (nbdir < nb_dir_affiche) /* si on a moins de dossier que le nombre que l'ont veut afficher on met le nombre d'affichage au max*/
        nb_dir_affiche = nbdir;
    for (i = nbdir-nb_dir_affiche; i < nbdir ;i++) /* on copie les derniers dossiers dans la chaine que l'ont voudra afficher*/
    {
        strcat(last_dir,liste_dir[i]);
        strcat(last_dir,"/");
    }
}

int decoupe(char **liste_retour, char *ligne_entree, char* char_decoupe){
    char *buffer;

    buffer = strtok(ligne_entree,char_decoupe); /* on decoupe la ligne entree en parametre avec le caractere mis en parametre*/

    int i=0;
    while (buffer != NULL){ /* tant que le buffer n'est pas à null*/
        liste_retour[i] = buffer; /* on enregistre l'adresse du buffer dans la liste d'argument*/
        buffer = strtok(NULL,char_decoupe);
        i++;
    }
    liste_retour[i+1] = buffer; /* metre NULL a la fin du tableau */
    return i;
}

int lire(char *chaine, int longueur){
    char *positionEntree = NULL;

    if (fgets(chaine, longueur, stdin) != NULL){
        positionEntree = strchr(chaine, '\n');
        if (positionEntree != NULL){
            *positionEntree = '\0';
        }
        else{
            viderBuffer();
        }
        return 1;
    }
    else{
        viderBuffer();
        return 0;
    }
}

void viderBuffer(){
    int c = 0;
    while (c != '\n' && c != EOF){
        c = getchar();
    }
}

/* touch [OPTIONS] FICHIER
touch -m FICHIER <=> modifie la date de modification du fichier
touch FICHIER <=> crée le fichier
Prend en entrée un tableau d'arguments donnés
Crée un fichier ou modifie l'horodatage
renvoie 0 si ça marche, -1 sinon.*/
int touch(char **liste_arg, int taille_args){
  int i;
  FILE *fichier;

  if(taille_args==1){
    /*créer juste le fichier*/
  }else if(taille_args==2){
    /*vérifier que c'est un -m et modif modif*/
    if(strcmp(liste_arg[1], "-m")!=0){
      /*erreur*/
      printf("touch : option invalide ; essayer touch [-m] FICHIER\n");
      return -1;
    }
    fichier = fopen(liste_arg[taille_args], "a");
    fclose(fichier);

  }else{
    /*erreur*/
    printf("touch : option invalide ; essayer touch [-m] FICHIER\n");
    return -1;
  }

  return 0;
}
