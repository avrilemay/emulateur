 /* *******************************************************
 * Nom           : cx25.1.c
 * Rôle          : Émulateur ordi papier en C + débogueur
 * Auteur        : Avrile Floro
 * Version       : 1.0
 * Date          : 2023-03-25
 * Licence       : L1 prog_imperative
 * *******************************************************
 *
 * Compilation   : gcc -Wall -std=gnu99 cx25.1.c -o cx25.1
 *
 * Makefile      : Un makefile a été créé. Il faut utiliser la commande
 *                 "make" dans le répertoire courant du dossier.
 *
 * Usage         : ./cx25.1 [programme].txt 0x[adresse_début] -stepper -ram
 *                 -journal -print -breakpoint
 *
 * Description   : Le programme est écrit en C. C'est un émulateur simplifié
 *                 qui lit un programme pris en entrée et exécute les
 *                 instructions. La mémoire de 256 bytes est représentée
 *                 par un tableau RAM[]. On utilise comme registres: le PC
 *                 et l'accumulateur.
 *                 Un débogueur est intégré au programme. Il dispose des
 *                 fonctionnalités suivante:
 *                      -ram : pour afficher l'initilisation de la RAM[];
 *                      -stepper : pour attendre une entrée utilisateur
 *                                 avant le passage à chaque instruction
 *                                 suivante (est réversible);
 *                      -print : pour afficher le détail
 *                               des cycles opératoires (est réversible
 *                               quand utilisé avec stepper);
 *                      -journal : pour imprimer dans un fichier journal
 *                                 le détail des cycles opératoires
 *                      -breakpoint : pour mettre un point d'arrêt à une adresse
 *                                    hex du programme. S'arrêtera à chaque
 *                                    fois que l'adresse est utilisée.
 *
 * Précisions    : On utilise les extensions de GCC avec -std=gnu99 afin
 *                 d'utiliser les fonctions non-standards.
 *                 Ne pas oublier le './' pour indiquer qu'on se trouve
 *                 dans le répertoire courant.
 *
 * ****************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

// codes opératoires
enum { ADD_SHARP = 0x20, ADD_AT = 0x60, ADD_STAR_AT = 0xE0, SUB_SHARP = 0x21, SUB_AT = 0x61, SUB_STAR_AT = 0xE1, NAND_SHARP = 0x22, NAND_AT = 0x62, NAND_STAR_AT = 0xE2, LOAD_SHARP = 0x0, LOAD_AT = 0x40, LOAD_STAR_AT = 0xC0, STORE_AT = 0x48, STORE_STAR_AT = 0xC8, IN_AT = 0x49, IN_STAR_AT = 0xC9, OUT_SHARP = 0x01, OUT_AT = 0x41, OUT_STAR_AT = 0xC1, JUMP_AT = 0x10, BRN_AT = 0x11, BRZ_AT = 0x12 } mnemonique;

// mémoire 256 bytes
unsigned char RAM[256];

// tableau pour affichage des mnémoniques dans les cycles
const char* nom_mnemonique[] = { [ADD_SHARP] = "ADD#", [ADD_AT] = "ADD@", [ADD_STAR_AT] = "ADD*@", [SUB_SHARP] = "SUB#", [SUB_AT] = "SUB@", [SUB_STAR_AT] = "SUB*@", [NAND_SHARP] = "NAND#", [NAND_AT] = "NAND@", [NAND_STAR_AT] = "NAND*@", [LOAD_SHARP] = "LOAD#", [LOAD_AT] = "LOAD@", [LOAD_STAR_AT] = "LOAD*@", [STORE_AT] = "STORE@", [STORE_STAR_AT] = "STORE*@", [IN_AT] = "IN@", [IN_STAR_AT] = "IN*@", [OUT_SHARP] = "OUT#", [OUT_AT] = "OUT@", [OUT_STAR_AT] = "OUT*@", [JUMP_AT] = "JUMP@", [BRN_AT] = "BRN@", [BRZ_AT] = "BRZ@" };

// prototype émulateur
void emulation(int* PC, int* accumulateur, bool* sortie_switch);

// prototype fonction stepper débogueur
void mon_stepper(int PC, int accumulateur, bool* stepper, bool* j_print, bool* ma_ram) ;

// prtotyle journal débogueur
void ecriture_journal(FILE* mon_journal, int compteur, int* PC, unsigned char *RAM, const char** nom_mnemonique, int accumulateur);

// impression verbeuse émulateur débogueur
void details_print(int compteur, int* PC, unsigned char* RAM, const char** nom_mnemonique, int accumulateur);

// prototype breakpoint
void breakpoint(bool* point_arret, int* adresse_point_arret, int PC);

// prototype définition breakpoint
void def_breakpoint(int* adresse_point_arret);

// message d'erreur sur stderr
void usage(char* message);


int main(int k, char* ldc[]) {

    if (k<3) { // s'il manque un élément essentiel
        usage("\nExemple d'utilisation: ./cx25.1 prog_10_5.txt 30\n\nMode d'emploi:\n\n\t- N'oubliez pas le './' pour indiquer que nous sommes dans le répertoire courant.\n\n\t-L'argument[0] est le nom du programme, ici 'cx25.1'\n\n\t-L'argument[1] est le nom du fichier à faire tourner dans l'ordinateur en papier:\n\t\t*Le fichier doit se trouver dans le répertoire courant. \n\t\t*Pensez à bien ajouter l'extension '.txt'. \n\t\t*Les fichiers pouvant être utilisés sont: \n\t\t\t- 'prog_test.txt'(qui teste l'ensemble des mnémoniques)--> début = 0x2E, \n\t\t\t- 'prog_10_5.txt'(qui affiche le produit de 2 nombres)--> début = 0x30, \n\t\t\t- 'prog_10_6.txt' (qui affiche le quotient entier de 2 nombres) --> début = 0x36, \n\t\t\t- 'prog_10_7.txt' (qui donne le cube d'un nombre) --> début = 0x50, \n\t\t\t- 'prog_13_8.txt' (qui ne fait rien - programme maintenant supprimé)--> début = 0x30.\n\n\t-L'argument[2] est l'adresse à partir de laquelle il faut lancer le fichier passé en argument: \n\t\t*L'adresse doit être HEXADÉCIMALE. \n\t\t\t- Vous pouvez ajouter (ou non) 0x avant la valeur. \n\t\t\t- La valeur entrée sera toujours interprétée comme une valeur hexédécimale.\n\n\nExemple d'utilisation du débogueur: \n./cx25.1 prog_10_5.txt 30 -ram -breakpoint -stepper -journal -print\n\nMode d'emploi du débogueur:\n\n\t- Pour afficher l'initialisation de la RAM, passez un argument (dans l'ordre que vous voulez mais après l'argument[2]) '-ram'\n\t\t*Cette fonctionnalité peut être utilisée en association avec le stepper.\n\n\t- Pour insérer un point d'arrêt, passez un argument (dans l'ordre que vous voulez mais après l'argument[2]) '-breakpoint'\n\t\t*L'utilisateur va être invité à entrer une valeur correspondant au point d'arrêt avant le lancement de l'émulateur. \n\t\t*La valeur du breakpoint doit correspondre à une adresse hexadécimale valide de RAM[]. \n\n\t- Pour utiliser le stepper, passez un argument (dans l'ordre que vous voulez mais après l'argument[2]) '-stepper'\n\t\t*Le stepper attend avant le passage à chaque instruction suivante du programme que l'utilisateur appuie sur la touche entrée. \n\t\t*Le stepper affiche la valeur de PC et de l'accumulateur.\n\t\t*Pour sortir du stepper, entrez 'stop_stepper'.\n\t\t*Pour activer l'impression détaillée des cycles au sein du stepper, entrez 'print'.\n\t\t*Pour désactiver l'affichage détaillé des cycles au sein du stepper, entrez 'stop_print'.\n\t\t*Pour sortir du stepper, entrez 'stop_stepper'.\n\n\t- Pour utiliser le journal, passez un argument (dans l'ordre que vous voulez mais après l'argument[2]) '-journal'.\n\t\t*Un fichier au nom personnalisé selon le programme passé en argument 'journal_prog_[...].txt' contenant les détails de chaque cycle d'opération sera créé.\n\n\t- Pour afficher les cycles détaillés (à l'identique du journal) dans le terminal, passez un argument (dans l'ordre que vous voulez mais après l'argument[2]) '-print'.\n\t\t*Les cycles détaillés peuvent être activés depuis le stepper avec l'option 'print'.\n\t\t*Les cycles détaillés peuvent être désactivés depuis le stepper avec l'option 'stop_print'.\n\n");}

    else if (strcmp(ldc[1], "prog_test.txt") == 0){
        if (!(strcmp(ldc[2], "0x2E") == 0 || strcmp(ldc[2], "2E") == 0)) {
        usage("Veuillez entrer pour adresse héxadécimale de début du programme prog_test.txt soit '0x2E' soit '2E'. Sinon, le programme ne s'exécutera pas correctement");}}

    else if (strcmp(ldc[1], "prog_10_5.txt") == 0){
        if (!(strcmp(ldc[2], "0x30") == 0 || strcmp(ldc[2], "30") == 0)) {
        usage("Veuillez entrer pour adresse héxadécimale de début du programme 10_5.txt soit '30' soit '0x30'. Sinon, le programme ne s'exécutera pas correctement");}}

    else if (strcmp(ldc[1], "prog_10_6.txt") == 0){
        if (!(strcmp(ldc[2], "0x36") == 0 || strcmp(ldc[2], "36") == 0)) {
        usage("Veuillez entrer pour adresse héxadécimale de début du programme 10_6.txt soit '36' soit '0x36'. Sinon, le programme ne s'exécutera pas correctement");}}

    else if (strcmp(ldc[1], "prog_10_7.txt") == 0){
        if (!(strcmp(ldc[2], "0x30") == 0 || strcmp(ldc[2], "30") == 0)) {
        usage("Veuillez entrer pour adresse héxadécimale de début du programme 10_7.txt soit '50' soit '0x50'. Sinon, le programme ne s'exécutera pas correctement");}}

    else if (strcmp(ldc[1], "prog_13_8.txt") == 0){
        if (!(strcmp(ldc[2], "0x30") == 0 || strcmp(ldc[2], "30") == 0)) {
        usage("Veuillez entrer pour adresse héxadécimale de début du programme 13_8.txt soit '30' soit '0x30'. Sinon, le programme ne s'exécutera pas correctement");}}

    if (k==3) { // si aucune option du débogueur n'est utilisée
        printf("\nExemple d'utilisation du débogueur: \n./cx25.1 prog_10_5.txt 30 -ram -breakpoint -stepper -journal -print\n\nMode d'emploi du débogueur:\n\n\t- Pour afficher l'initialisation de la RAM, passez un argument (dans l'ordre que vous voulez mais après l'argument[2]) '-ram'\n\t\t*Cette fonctionnalité peut être utilisée en association avec le stepper.\n\n\t- Pour insérer un point d'arrêt, passez un argument (dans l'ordre que vous voulez mais après l'argument[2]) '-breakpoint'\n\t\t*L'utilisateur va être invité à entrer une valeur correspondant au point d'arrêt avant le lancement de l'émulateur. \n\t\t*La valeur du breakpoint doit correspondre à une adresse hexadécimale valide de RAM[]. \n\n\t- Pour utiliser le stepper, passez un argument (dans l'ordre que vous voulez mais après l'argument[2]) '-stepper'\n\t\t*Le stepper attend avant le passage à chaque instruction suivante du programme que l'utilisateur appuie sur la touche entrée. \n\t\t*Le stepper affiche la valeur de PC et de l'accumulateur.\n\t\t*Pour sortir du stepper, entrez 'stop_stepper'.\n\t\t*Pour activer l'impression détaillée des cycles au sein du stepper, entrez 'print'.\n\t\t*Pour désactiver l'affichage détaillé des cycles au sein du stepper, entrez 'stop_print'.\n\t\t*Pour sortir du stepper, entrez 'stop_stepper'.\n\n\t- Pour utiliser le journal, passez un argument (dans l'ordre que vous voulez mais après l'argument[2]) '-journal'.\n\t\t*Un fichier 'journal.txt' contenant les détails de chaque cycle d'opération sera créé.\n\n\t- Pour afficher les cycles détaillés (sur le modèle du journal) dans le terminal, passez un argument (dans l'ordre que vous voulez mais après l'argument[2]) '-print'.\n\t\t*Les cycles détaillés peuvent être activés depuis le stepper avec l'option ' print'.\n\t\t*Les cycles détaillés peuvent être désactivés depuis le stepper avec l'option 'stop_print'.\n\n");}

    char chemin_programme[1024];
    strcpy(chemin_programme, "./"); // répertoire courant, pas obligatoire
    strcat(chemin_programme, ldc[1]); // + ajout nom fichier

    int debut = strtol(ldc[2], NULL, 16); // adresse de départ en héxadécimale

    FILE* programme = fopen(chemin_programme, "r") ; // programme à exécuter
    if (! programme){
        perror("fichier programme (main)");
        exit(EXIT_FAILURE);}

    FILE* mon_journal = NULL;
    int i;
    bool stepper = false; // initialisation valeur stepper
    bool journal = false; // journal
    bool j_print = false; // impression verbeuse
    bool ma_ram = false; // impression RAM
    bool point_arret = false; // point d'arrêt

    for (i=0; i < k; i++){ // si ___ en argument ldc

        // on passe les bool à vrai + insensible à la casse
        if (strcasecmp(ldc[i], "-stepper") == 0) {stepper = true;}
        if (strcasecmp(ldc[i], "-journal") == 0) {journal = true;}
        if (strcasecmp(ldc[i], "-print") == 0) {j_print = true;}
        if (strcasecmp(ldc[i], "-ram") == 0) {ma_ram = true;}
        if (strcasecmp(ldc[i], "-breakpoint") == 0) {point_arret = true;}}

    char nom_journal[100];

    // ssi journal en arg, on ouvre/créé le fichier journal personnalisé
    if (journal) {
        sprintf(nom_journal, "journal_cx25.1_%s", ldc[1]);
        mon_journal = fopen(nom_journal, "w");}
    // ssi en argument et impossibilité d'ouvrir
    if ((journal) && (!mon_journal)){
        perror("fichier journal (main)");
        exit(EXIT_FAILURE);}

    int PC = 0x0; // initialisation registres pour RAM
    int accumulateur = 0x0;
    int compteur = 0;

    // initilisation de la RAM[]
    int num, h = debut;

    while (fscanf(programme, "%x", &num) == 1) {
        RAM[h] = (unsigned char) num;

        // si option -ram du débogueur activée
        if (ma_ram) printf("\tRAM[0x%02x] = 0x%02x\n", h, RAM[h]);
        // si option -stepper & option -ram du débogueur activés
        if ((ma_ram)&&(stepper)) {mon_stepper(PC, accumulateur, &stepper, &j_print, &ma_ram);}
        h++;}

    if (ferror(programme)) { // gestion des erreurs
        perror("Erreur lors de la lecture du fichier programme à l'initialisation de la RAM");
        exit(EXIT_FAILURE);}


    PC = debut; // initialisation registres pour émulateur
    bool sortie_switch = false; // pour sortir switch quand "OUT"
    int adresse_point_arret; // pour breakpoint

    // utilisateur entre le point d'arrêté souhaité
    if (point_arret){def_breakpoint(&adresse_point_arret);}

        // boucle de l'émulateur
    while ((PC > 0x1F) && (PC < 0xFF)){ // dispo mémoire entre 16 et 254 (base 10)
        compteur ++;

        // si option -breakpoint du débogueur
        breakpoint(&point_arret, &adresse_point_arret, PC);

        // si option -journal du débogueur
        if (journal) {ecriture_journal(mon_journal, compteur, &PC, &RAM[0], &nom_mnemonique[0], accumulateur);}

        // si option -print du débogueur
        if (j_print) {details_print(compteur, &PC, &RAM[0], &nom_mnemonique[0], accumulateur);}

        // si option -stepper du débogueur
        if (stepper) {mon_stepper(PC, accumulateur, &stepper, &j_print, &ma_ram);}

        // coeur du programme émulateur
        emulation(&PC, &accumulateur, &sortie_switch);
        //on sort du switch quand "OUT"
        if (sortie_switch){break;}        }

    if (fclose(programme) != 0) {
    perror("Erreur lors de la fermeture du fichier programme");
    exit(EXIT_FAILURE);}

    if ((journal) && (fclose(mon_journal) != 0)) { // ssi le fichier a été créé
    perror("Erreur lors de la fermeture du fichier mon_journal");
    exit(EXIT_FAILURE);}
    return 0; }


// fonctionnement de l'ordinateur en papier
void emulation(int* PC, int* accumulateur, bool* sortie_switch){

            switch (RAM[*PC]){

            case ADD_SHARP://ADD# ajoute l'entier immédiat à acc
            *accumulateur += RAM[*PC+1];
            printf("\tADD# 0x%02x\n\tAccumulateur = 0x%02x\n\n\n", RAM[*PC+1], *accumulateur); *PC +=2; break;

            case ADD_AT://ADD@ ajoute la valeur de l'adresse spécifiée à acc
            *accumulateur += RAM[RAM[*PC+1]];
            printf("\tADD@ 0x%02x\n\tAccumulateur = 0x%02x\n\n\n", RAM[*PC+1], *accumulateur); *PC +=2; break;

            case ADD_STAR_AT: //ADD*@ ajoute la valeur d'un ptr de ptr à acc
            *accumulateur += RAM[RAM[RAM[*PC+1]]];
            printf("\tADD*@ 0x%02x\n\tAccumulateur = 0x%02x\n\n\n", RAM[*PC+1], *accumulateur); *PC +=2; break;

            case SUB_SHARP://SUB# soustrait entier immédiat
            *accumulateur -= RAM[*PC+1];
            printf("\tSUB# 0x%02x\n\tPC = [0x%02x]\n\tAccumulateur = 0x%02x\n\n\n", RAM[*PC+1], *PC, *accumulateur); *PC +=2; break;

            case SUB_AT://SUB@ soustrait la valeur de l'adresse spécifiée
            *accumulateur -= RAM[RAM[*PC+1]];
            printf("\tSUB@ 0x%02x\n\tAccumulateur = 0x%02x\n\n\n", RAM[*PC+1], *accumulateur); *PC +=2; break;

            case SUB_STAR_AT://SUB*@ soustrait la valeur via ptr de ptr
            *accumulateur -= RAM[RAM[RAM[*PC+1]]];
            printf("\tSUB*@ 0x%02x\n\tAccumulateur = 0x%02x\n\n\n", RAM[*PC+1], *accumulateur); *PC +=2; break;

            case NAND_SHARP://NAND# bit à bit avec entier immédiat
            *accumulateur = ~(*accumulateur & RAM[*PC+1]);
            printf("\tNAND# 0x%02x\n\n\n", RAM[*PC+1]);*PC +=2;break;

            case NAND_AT://NAND@ bit à bit avec valeur de l'ad. spécifiée
            *accumulateur = ~(*accumulateur & RAM[RAM[*PC+1]]);
            printf("\tNAND@ 0x%02x\n\n\n", RAM[*PC+1]);*PC +=2;break;

            case NAND_STAR_AT://NAND*@ bit à bit via ptr de ptr
            *accumulateur = ~(*accumulateur & RAM[RAM[RAM[*PC+1]]]);
            printf("\tNAND*@ 0x%02x\n\n\n", RAM[*PC+1]);*PC +=2;break;

            case LOAD_SHARP://LOAD# charge entier immédiat
            *accumulateur = RAM[*PC+1];
            printf("\tLOAD# 0x%02x\n\tAccumulateur = 0x%02x\n\n\n", RAM[*PC+1], *accumulateur); *PC +=2; break;

            case LOAD_AT://LOAD@ charge entier d'ad. mémoire
            *accumulateur = RAM[RAM[*PC+1]];
            printf("\tLOAD@ 0x%02x\n\tAccumulateur = 0x%02x\n\n\n", RAM[*PC+1], *accumulateur); *PC +=2;break;

            case LOAD_STAR_AT://LOAD*@ charge entier via ptr de ptr
            *accumulateur = RAM[RAM[RAM[*PC+1]]];
            printf("\tLOAD*@ 0x%02x\n\tAccumulateur = 0x%02x\n\n\n", RAM[*PC+1], *accumulateur); *PC +=2; break;

            case STORE_AT://STORE@ stocke acc. dans ad. mémoire
            RAM[RAM[*PC+1]] = *accumulateur;
            printf("\tSTORE@ 0x%02x\n\n\n", RAM[*PC+1]);*PC +=2;break;

            case STORE_STAR_AT://STORE*@ stocke acc. dans ptr de ptr
            printf("\n"); RAM[RAM[RAM[*PC+1]]] = *accumulateur;
            printf("\tSTORE*@ 0x%02x\n\n\n", RAM[*PC+1]); *PC +=2; break;

            case IN_AT://IN@ lit entrée user et la stocke dans ad. mémoire
            printf("\tIN@ 0x%02x",RAM[*PC+1]);
            printf("\n\tEntrez une valeur hexadécimale:");
            if (scanf("%hhx", &RAM[RAM[*PC+1]]) != 1) {
                perror("Erreur lors de la saisie de la valeur hexadécimale");
                exit(EXIT_FAILURE); }
            printf("\n");
            while(getchar() != '\n'); // vide buffer pour stepper
            printf("\tValeur entrée: 0x%02x\n\n\n", RAM[RAM[*PC+1]]);*PC +=2; break;

            case IN_STAR_AT://IN*@ lit entrée user et la stocke dans ptr de ptr
            printf("\tIN*@ 0x%02x\n\tEntrez une valeur hexadécimale:",RAM[*PC+1]);
            //if (scanf("%hhx", &RAM[RAM[RAM[*PC+1]]]) != 1) {
                //perror("Erreur lors de la saisie de la valeur hexadécimale");
                //exit(EXIT_FAILURE);}
            while(getchar() != '\n'); // vide buffer pour stepper
            printf("\tValeur entrée: 0x%02x\n\n\n", RAM[RAM[RAM[*PC+1]]]);*PC +=2; break;

            case OUT_SHARP: //OUT# affiche résultat immédiat en sortie
            printf("\tOUT# 0x%02x\n\tLe résultat est: 0x%02x\n\n\n",RAM[*PC+1], RAM[*PC+1]);
            *sortie_switch = true; break;

            case OUT_AT://OUT@ affiche résultat à une ad. mémoire sur la sortie
            printf("\tOUT@ 0x%02x\n\tLe résultat est: 0x%02x\n\n\n",RAM[*PC+1], RAM[RAM[*PC+1]]);
            *sortie_switch = true; break;

            case OUT_STAR_AT://OUT*@ affiche un résultat pointé par un ptr de ptr
            printf("\tOUT*@ 0x%02x\n\tLe résultat est: 0x%02x\n\n\n",RAM[*PC+1], RAM[RAM[RAM[*PC+1]]]);
            *sortie_switch = true; *PC +=2; break;

            case JUMP_AT://JUMP@ saut inconditionnel vers ad. mémoire
            printf("\tJUMP@ 0x%02x\n\n\n", RAM[*PC+1]);
            *PC = RAM[*PC+1]; break;

            case BRN_AT://BRN@ saut conditionnel si acc <0
            printf("\tBRN@ 0x%02x\n\tAccumulateur = 0x%02x\n\tSi l'accumulateur < 0, PC <- [0x%02x]\n\n\n",RAM[*PC+1], *accumulateur, RAM[*PC+1]);
            if (*accumulateur & 0x80) // vérifie si le bit est de signe 1
                *PC = RAM[*PC+1];
            else *PC+=2;
            break;

            case BRZ_AT://BRZ@ saut conditionnel si acc = 0
            printf("\tBRZ_AT 0x%02x\n\tAccumulateur = 0x%02x\n\tSi l'accumulateur = 0, PC <- [0x%02x]\n\n\n",RAM[*PC+1], *accumulateur, RAM[*PC+1]);
            if (*accumulateur == 0)
                *PC = RAM[*PC+1];
            else *PC+=2;
            break;

            default:
            printf("Instruction non reconnue : %d\n", RAM[*PC]);
            break;}}


// attend une action utilisateur avant chaque passage d'instruction
void mon_stepper(int PC, int accumulateur, bool* stepper, bool* j_print, bool* ma_ram) {
    char input[50] = {'\0'};

    while (input[0] == '\0') {
        if ((*ma_ram)&&(PC==0)) printf("Initialisation de la RAM...\n\tSTEPPER: PC: 0x%02x  Accumulateur: 0x%02x\n", PC, accumulateur);
        if ((*stepper)&&(PC>0)) printf("\nSTEPPER: PC: 0x%02x  Accumulateur: 0x%02x\n", PC, accumulateur);
        printf("Appuyez sur entrée pour continuer ou entrez 'stop_stepper' pour arrêter le stepper\n");
        if ((*j_print)&&(PC>0)) printf("Entrez 'stop_print' pour arrêter les entrées détaillées\n"); // pas cette option lors initialisation RAM
        if ((!*j_print)&&(PC>0)) printf("Entrez 'print' pour affichier les entrées détaillées\n");
        fgets(input, sizeof(input), stdin);
        if (strcasecmp(input, "stop_stepper\n") == 0) {
            *stepper = false; } // insensible casse
        if (strcasecmp(input, "stop_print\n") == 0) {
            *j_print = false; } // maj des bools
        if (strcasecmp(input, "print\n") == 0) {
            *j_print = true; }}}


// imprime les entrées détaillées dans le terminal
void details_print(int compteur, int* PC, unsigned char* RAM, const char** nom_mnemonique, int accumulateur) {

    printf("Cycle d'opération n°%d: \n\tPC = 0x%02x\n\tRAM[0x%02x] = 0x%02x\n\tRAM[0x%02x] = 0x%02x\n\tAccumulateur : 0x%02x\n\t\tMnémonique:\t 0x%02x (%s)\n\t\tArgument:\t0x%02x\n\n", compteur, *PC, *PC, RAM[*PC], *PC+1, RAM[*PC+1], accumulateur, RAM[*PC], nom_mnemonique[RAM[*PC]], RAM[*PC+1]);}


// inscrit les entrées détaillées dans le journal
void ecriture_journal(FILE* journal, int compteur, int* PC, unsigned char* RAM, const char** nom_mnemonique, int accumulateur) {

    fprintf(journal, "Cycle d'opération n°%d: \n\tPC = 0x%02x\n\tRAM[0x%02x] = 0x%02x\n\tRAM[0x%02x] = 0x%02x\n\tAccumulateur : 0x%02x\n\t\tMnémonique:\t 0x%02x (%s)\n\t\tArgument:\t0x%02x\n\n", compteur, *PC, *PC, RAM[*PC], *PC+1, RAM[*PC+1], accumulateur, RAM[*PC], nom_mnemonique[RAM[*PC]], RAM[*PC+1]);}


//arrête l'exécution au point d'arrêt
void breakpoint(bool* point_arret, int* adresse_point_arret, int PC) {

    if ((*point_arret)&&(PC == *adresse_point_arret)){
        printf("Le point d'arrêt RAM[0x%02x] est la mnémonique suivante.\n", *adresse_point_arret);
        printf("Appuyez sur une touche pour reprendre l'exécution.\n");
        getchar();}

    if ((*point_arret)&&(PC+1 == *adresse_point_arret)){
        printf("Le point d'arrêt RAM[0x%02x] est l'argument de l'instruction suivante.\n", *adresse_point_arret);
        printf("Appuyez sur une touche pour reprendre l'exécution.\n");
        getchar();}}


// entrée utilisateur de l'adresse breakpoint
void def_breakpoint(int* adresse_point_arret){

    printf("Entrez l'adresse hexadécimale (entre 00 et 0xFF) de l'instruction à laquelle mettre le point d'arrêt : ");
    if (scanf("%x", adresse_point_arret) != 1) {
        perror("Erreur lors de la saisie de l'adresse de point d'arrêt");
        exit(EXIT_FAILURE);}
    else if (*adresse_point_arret < 0 || *adresse_point_arret > 0xFF) {
        usage("Adresse invalide. L'adresse doit être comprise entre 0x0 et 0xFF.\n");}
    else {printf("Point d'arrêt défini à l'adresse 0x%02X.\n", *adresse_point_arret);}}


// affiche message d'erreur sur stderr
void usage(char* message) {fprintf(stderr, "%s\n", message) ; exit(1) ;}
