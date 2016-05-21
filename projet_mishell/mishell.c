#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <limits.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#define MAX_CHAR 100
#define PATH_MAX 4096
#define MAX_ARG 64
#define HISTORY "history"
#define BUF_SIZE 512


int lire(char *chaine, int longueur);
void viderBuffer();
int compte_mots(char* chaine);
int is_sep(char c);
int decoupe(char **tableau_elements, char *commande,char* decoupe);
void affiche_directory(char* last_dir, char* cwd);
void gestioncrtl_D();
int touch(char **liste_arg, int taille_args);
void enregistreligne(char* commande);
int history(char **liste_arg, int taille_args);
void fn_cd(char **liste_arg,char *cwd);
void fork_execvp(char **liste_arg);
int copy(char **liste_arg, int taille_args);
int cat(char **liste_arg, int taille_args);
char* tab_commande(int chiffre,char **liste_arg, char* commande);
void test_commandes(char** liste_arg,char *cwd, char* commande);
int retirer_retourChariot(char* chaine);
int is_in_path(char *argv);
int parse_envPath(char res[16][256], char *envPath, char symbole);
void fct_fork_exec(char **liste_arg);

int main (){
    char commande[MAX_CHAR] = "";
    char *liste_arg[MAX_ARG];
    char hostname[128];
    char last_dir[PATH_MAX] = "";
    char cwd[PATH_MAX + 1]; /*current working directory : tableau pour le chemin*/
    char cwdtmp[PATH_MAX + 1];
    getcwd(cwd, PATH_MAX + 1 );
    gethostname(hostname, sizeof hostname);


    is_in_path("HOME");


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

            enregistreligne(commande); /* enregistre la ligne de commande */

            test_commandes(liste_arg,cwd,commande);

        }
    }while(1);
    return 0;
}

void test_commandes(char** liste_arg,char *cwd, char* commande){
    int chiffre = 0;
    int taille_args = decoupe(liste_arg,commande," ");/* on decoupe la commande avec les espaces*/
    if (strcmp(liste_arg[0], "quit") == 0){
        exit(0);
    }else if(strcmp(liste_arg[0], "history") == 0){
        history(liste_arg, taille_args);
    }else if(strcmp(liste_arg[0], "touch") == 0){
        touch(liste_arg, taille_args);
    }else if(strcmp(liste_arg[0], "cd") == 0){
        fn_cd(liste_arg,cwd);
    }else if(sscanf(liste_arg[0], "!%d",&chiffre )){ /*exécute la commande numero $chiffre précédente*/
        memset(liste_arg,0,MAX_ARG); /* on met a zero le tableau d'argument*/
        commande = tab_commande(chiffre,liste_arg, commande);
        test_commandes(liste_arg,cwd,commande);
    }else if(strcmp(liste_arg[0], "cat") == 0){
        cat(liste_arg, taille_args);
    }else{
        fct_fork_exec(liste_arg);
    }
}

/*retourne le nombre de morceaux parsés par $symbole dans la chaîne $envPath, stocke ces morceaux dans le tableau $res*/
int parse_envPath(char res[16][256], char *envPath, char symbole){
    int i=0, j=0, ligne=0;
    while(envPath[i]!='\0'){
        if(envPath[i] ==  symbole){
            res[ligne][j]='\0';
            ligne++; /*pour écrire dans la prochaine ligne de res*/
            j=0;
            i++; /*pas enregistrer le symbole*/
        }
        res[ligne][j] = envPath[i];
        i++;
        j++;
    }
    return ligne;
}

void fct_fork_exec(char **liste_arg){
   char *path = getenv("PATH");
   char liste_path[16][256];
   /*ranger chaque directory du PATH, séparés par ':', dans liste_path*/
   int nbDir = parse_envPath(liste_path, path, ':');
   int i=0;
   int gotcha = 0; /*booléen pour savoir si on a trouvé le programme cherché dans les dossiers*/
   FILE *pFile;
   //printf("NUMBER OF DIR : %d\n", nbDir); 
   while(i<nbDir && !gotcha){
       /*concaténer pour avoir path/programme*/
       char path_total[256];
       //printf("liste path i = %s ; liste arg 0 = %s\n", liste_path[i], liste_arg[0]);
       strcpy(path_total, liste_path[i]);
       strcat(path_total, "/");
       strcat(path_total, liste_arg[0]);
       //printf("path : %s\n", path_total);
       pFile = fopen(path_total, "rb");/*on essaie d'ouvrir le fichier en lecture seule (r) et c'est un binary (b)*/
       if(pFile==NULL){
           /*le fichier n'existe pas, avançons dans la liste des directories*/
           i++;
       }else{
           /*alors on a trouvé le bon fichier à exécuter*/
           fclose(pFile);
           gotcha = 1;
           /*pour ne pas tuer mishell, on crée un nouveau processus fils pour l'exécution du programme appelé*/
           pid_t fils;
           fils = fork();
           if(fils == -1){ /*si le fork s'est mal passé*/
               exit(EXIT_FAILURE);
           }else{
               if(fils == 0){/*si c'est le fils*/
                   execv(path_total, liste_arg);
               }else{ /*si c'est le père*/
                   waitpid(fils, NULL, 0); /*on attend le fils*/
               }
           }
           execv(liste_path[i], liste_arg);
       }
   }
}

char* tab_commande(int chiffre,char **liste_arg, char *chaine){
    FILE* fichier = NULL;
    strcpy(chaine,"");
    int i = 1;
    fichier = fopen(HISTORY, "r");
        if (fichier != NULL){
            while (fgets(chaine, MAX_CHAR, fichier) != NULL){ // On lit le fichier tant qu'on ne reçoit pas d'erreur (NULL)
                if (i == chiffre){
                    retirer_retourChariot(chaine);
                     return chaine;
                }
                i++;
            }
            fclose(fichier);
        }
    return NULL;
}

int retirer_retourChariot(char* chaine)
{
  int i;
  for(i=0;chaine[i] != '\0';i++)
  {
    if(chaine[i] == '\n')
    {

      chaine[i] = '\0';
      return i + 1;
    }
  }
  //printf("%s", chaine);
  return i;
}

/*on doit faire un fork sinon mishell meurt*/
void fork_execvp(char **liste_arg){
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


void fn_cd(char **liste_arg,char *cwd){
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
}

int history(char **liste_arg, int taille_args)
{
    FILE* fichier = NULL;
    int i = 1;
    char chaine[MAX_CHAR] = "";

    if(taille_args==1){ /*lis le fichier en entier*/

        fichier = fopen(HISTORY, "r");
        if (fichier != NULL){
            while (fgets(chaine, MAX_CHAR, fichier) != NULL){ // On lit le fichier tant qu'on ne reçoit pas d'erreur (NULL)
                printf(" %d %s",i, chaine); // On affiche la chaîne qu'on vient de lire
                i++;
            }
            fclose(fichier);
        }
        return 0;
    }else if(taille_args==2){ /*vérifier si c'est un nombre et retourner les n dernier elements*/
        for (i=0;i<strlen(liste_arg[1]);i++){
                if (!isdigit(liste_arg[1][i])) {
                     printf("history: numeric argument required \n");
                        return -1;
                }
        }
        int nbaafficher = atoi(liste_arg[1]);
        int nblignes =0;
        char chaine[MAX_CHAR] = "";
        fichier = fopen(HISTORY, "r");
        if (fichier != NULL){
            while (fgets(chaine, MAX_CHAR, fichier) != NULL){ /*On lit le fichier tant qu'on ne reçoit pas d'erreur (NULL)*/
                nblignes++; /* on compre le nmbre de lignes dans le ficher*/
            }
            fclose(fichier);
        }
        fichier = fopen(HISTORY, "r");
        if (fichier != NULL){
            while (fgets(chaine, MAX_CHAR, fichier) != NULL){ /* On lit le fichier tant qu'on ne reçoit pas d'erreur (NULL)*/
                if (nblignes-nbaafficher<i){
                    printf(" %d %s",i, chaine); /* On affiche la chaîne qu'on vient de lire*/
                }
                i++;
            }
            fclose(fichier);
        }
        return 0;

    }else{
        /*erreur*/
        printf("history : option invalide ; essayer history [n] \n");
        return -1;
  }

}

int is_in_path(char *argv){
    char *variable;

    variable = getenv("PATH");
    if (!variable) {
        printf("%s : n'existe pas\n", argv);
        return EXIT_FAILURE;
    } else {
        printf("%s : %s\n", argv, variable);
        return EXIT_SUCCESS;
    }
}




void enregistreligne(char* commande){
    char test[MAX_CHAR] = "";
    strcpy(test,commande);
    FILE *fichier;

    fichier = fopen(HISTORY,"a");
    if (fichier != NULL){
        strcat(test,"\n");
        fprintf(fichier,"%s",test);
        fclose(fichier);
    }

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
/*retourne le nombre de bouts découpés depuis ligne_entree = le nb d'entrées de liste_retour*/
int decoupe(char **liste_retour, char *ligne_entree, char* char_decoupe){
    strcpy(ligne_entree, ligne_entree);
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
  FILE *fichier;

  if(taille_args==2){
    /*créer juste le fichier*/
    fichier = fopen(liste_arg[1], "a");
    fclose(fichier);
  }else if(taille_args==3){
    /*vérifier que c'est un -m et modif modif*/
    if(strcmp(liste_arg[1], "-m")!=0){
      /*erreur*/
      printf("touch : option invalide ; essayer touch [-m] FICHIER\n");
      return -1;
    }

    fichier = fopen(liste_arg[taille_args-1], "r+");
    if(fichier == NULL){
        printf("Le fichier %s n'existe pas dans ce dossier.\n",liste_arg[taille_args-1]);
        return -1;
    }else{
      fclose(fichier);
    }
  }else{
    /*erreur*/
    printf("touch : option invalide ; essayer touch [-m] FICHIER\n");
    return -1;
  }

  return 0;
}

/*cat FILE1 ... FILEn
 * afficher les fichiers FILE1 ...  FILEn
 *
 * cat FILE1 ...FILEn -n
 * afficher les fichiers 1 à n à la suite, chaque ligne numérotée
 *
 * retourne -1 si erreur, 0 sinon
 * */
int cat(char **liste_arg, int taille_args){
    int i,j;
    int fichier;
    int compter = 0; /*role de booleen, passe à 1 si option -n*/
    int compteur = 1; /*pour afficher le numero de chaque ligne*/
    int endLine = 1; /* booleen pour savoir si on atteints un \n */

    /*les 5 variables suivantes permettent de parcourir le contenu
     * du fichier à afficher, par sections de 512*/
    char buf[BUF_SIZE];
    /*init buf*/
    i = 0;
    ssize_t nbRead;

    /*numéroter les lignes*/
    if(strcmp(liste_arg[taille_args-1], "-n")==0){
        compter = 1;
    }

    /*afficher les fichiers à la suite*/
    for(i=1; i < taille_args-compter; i++){
        fichier = open(liste_arg[i], O_RDONLY);
        if(fichier == -1){
            printf("Erreur : %s n'existe pas dans ce dossier.\n", liste_arg[i]);
            return -1;
        }
	/*nbRead va prendre le nombre de caractères lus dans le fichier courant*/
        while((nbRead = read(fichier, buf, BUF_SIZE)) >0){
            for(j=0; j < nbRead; j++){
                if(endLine && compter) {
                    printf("%d \t", compteur);
                    endLine = 0;
                }
                printf("%c", buf[j]);
                if(buf[j] == '\n') {
                    compteur++;
                    endLine = 1;
                }
            }
        }
        /* fin de lecture du fichier, on le ferme. */
        if(fichier >=0) {
            close(fichier);
        }
    }
    return 0;
}

/* copier source dans cible ;
Retourne 0 si succès, -1 sinon*/
int copy(char **liste_arg, int taille_args){
    if(taille_args > 3){
	printf("Erreur nombre d'arguments : copy SOURCE DESTINATION");
	return -1;
    }
    char *source = liste_arg[1];
    char *cible = liste_arg[2];
    int file_src, file_cib;
    char buf[512];
    struct stat statbuf[512];
    ssize_t nbRead;
    int errsv;

    /*Ouvre le fichier source*/
    file_src = open(source, O_RDONLY);
    if(file_src == -1){
	printf("Pas pu ouvrir le fichier source\n");
	return -1;
    }
    /*Ouvre sinon cree le fichier cible ; s'il existe alors O_TRUNC le vide avant d'ecrire dedans*/
    file_cib = open(cible, O_WRONLY | O_CREAT | O_TRUNC);
    if(file_cib == -1){
	printf("Pas pu ouvrir le fichier cible\n");
	return -1;
    }

    char *out_ptr = NULL;
    ssize_t nbWritten;

    while((nbRead = read(file_src, buf, sizeof buf)) > 0){
        out_ptr = buf;
        if(nbRead == -1){
            printf("Erreur lecture\n");
            errsv = errno;
            if(errsv == EAGAIN || errsv == EINTR){
                 printf("Erreur ; il faut recommencer l operation\n");
            }
            return -1;
        }
        nbWritten = write(file_cib, out_ptr, nbRead);
	if(nbWritten >= 0){
       	    nbRead -= nbWritten;
            out_ptr += nbWritten;
	}else if(nbWritten == -1){
      	    printf("Erreur ecriture\n");
            errsv = errno;
            if(errsv == EAGAIN || errsv == EINTR){
                printf("Erreur ; il faut recommencer l operation\n");
            }
            return -1;
	}
    }

    errsv = fstat(file_src, statbuf);
    if(errsv ==-1){
        printf("Erreur fstat\n");
        return -1;
    }
    errsv = fchmod(file_cib, statbuf->st_mode);
    if(errsv == -1){
        printf("Erreur fchmod\n");
        return -1;
    }

    if(file_src >= 0) close(file_src);
    if(file_cib >= 0) close(file_cib);
    return 0;
}

/*séparer les fichiers des options*/ /*
void definir_options(char **liste_arg, int taille_args, char **fichiers, char **options){

}*/
