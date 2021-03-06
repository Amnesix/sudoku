#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define ON 1
#define OFF 0

#define TRACES OFF

#define NORMAL 1
#define BASE   2
#define ERREUR 3
#define NORMAL_ 4
#define BASE_   5
#define ERREUR_ 6
#define VERT 7
#define RVERT 8

#define COL_NORMAL 0
#define COL_BASE 1
#define COL_ERREUR 2

#define CASE_VIDE '.'

static int couleurs[2][3] = {{NORMAL, BASE, ERREUR}, {NORMAL_, BASE_, ERREUR_}};

// Quelques grilles de tests.
#define tst1   "7186.5....3..42.....4.8.56..4729.65...........62.1784..73.5.4.....47..2....1.3975"
#define tst2   "..7..8.3..2...974.8..2.....48......3..2...6..3......57.....7..2.618...9..9.3..5.."
#define tst3   ".9...657171.9.5.....6.....36.41.7.......8.......5.37.41.....6.....8.1.495496...1."
#define tst4   "4...81..2.6.........3..2.84.....3..83...7...69..6.....78.2..9.........2.6..43...7"
#define tstE   "1.96.4.2.53.2.......8...1.5.....6.5.....4.....6.3.....2.1...7.......5.13.8.7.95.2"
#define tstCG1 "..6....5...37.....7...35..8....7..12...942...62..8....9..12...3.....36...5....7.."
#define tstCG2 "8..........36......7..9.2...5...7.......457.....1...3...1....68..85...1..9....4.."
#define tstX   "....7..8......9..5.9...3....37.91...4..7...93.....5....1..6......9214.3.....37.41"
static char *tsts[] = {tstCG1, tstCG2, tst1, tst2, tst3, tst4, tstE, tstX};
static int tst = 1;

static char table[81];
static char resolv[81];
static char tberreurs[81];
static char possibles[81][9];
static int xcur = 0;
static int ycur = 0;
static int verify = 0;
static int typeX = 0;
static int possible = 0;
static int affpossible = 0;
static int meme = 0;
static int offX, offY;

static char *touches[] = {"zqsd", "hjkl", "Flêches" };
static int jeutouches = 1;

int nbpossibles(int l, int c);
char singleton(int l, int c);


/**
 * Mesure précise du temps de calcul.
 */
static float mesure;
void starttime(struct timespec *t)
{
    clock_gettime(CLOCK_REALTIME, t);
}

double elapsed(struct timespec *t)
{
    struct timespec mesures;
    clock_gettime(CLOCK_REALTIME, &mesures);
    return (double)(mesures.tv_sec - t->tv_sec) + (double)(mesures.tv_nsec - t->tv_nsec) / 1000000000.;
}
/****************/

void traces()
{
    int l, c;

    for (l=0; l<9; l++)
        for (c=0; c<9; c++)
        {
            mvprintw(l, c*2, "%c", table[l*9+c]);
            mvprintw(l+10, c*2, "%c", resolv[l*9+c]);
            mvprintw(l+20, c*2, "%c", tberreurs[l*9+c] ? '#' : CASE_VIDE);
        }
}

/**
 * Vérification de la validité de la grille resolv
 * et mise à jour de la grille tberreurs.
 */
int verifier()
{
    int err = 0;
    char v;
    memset(tberreurs, 0, 81);
    for (int l=0; l<9; l++)
    {
        for (int c=0; c<9; c++)
        {
            if (tberreurs[l*9+c] || resolv[l*9+c] == CASE_VIDE)
                continue;
            for (int i=0; i<9; i++)
            {
                if (i != l && (v=resolv[i*9+c]) != CASE_VIDE && v == resolv[l*9+c])
                    tberreurs[l*9+c] = tberreurs[i*9+c] = 1;
                if (i != c && (v=resolv[l*9+i]) != CASE_VIDE && v == resolv[l*9+c])
                    tberreurs[l*9+c] = tberreurs[l*9+i] = 1;
                if (((l/3)*3+i/3)*9+(c/3)*3+i%3 != l*9+c &&
                    (v=resolv[((l/3)*3+i/3)*9+(c/3)*3+i%3]) != CASE_VIDE && v == resolv[l*9+c])
                    tberreurs[l*9+c] = tberreurs[((l/3)*3+i/3)*9+(c/3)*3+i%3] = 1;
            }
        }
        if (l == 8 || !typeX)
            continue;
        for (int i=l+1; i<9; i++)
        {
            if ((v=resolv[i*10]) != CASE_VIDE && v == resolv[l*10])
                tberreurs[l*10] = tberreurs[i*10] = 1;
            if ((v=resolv[(8-i)*9+i]) != CASE_VIDE && v == resolv[(8-l)*9+l])
                tberreurs[(8-l)*9+l] = tberreurs[(8-i)*9+i] = 1;
        }
    }
    for (int i=0; i<81; i++)
        err += tberreurs[i];
    return err;
}

/**
 * Vérifie si la grille est complète et sans erreur.
 */
int complet()
{
    for (int i=0; i<81; i++)
    {
        if (resolv[i] == CASE_VIDE)
            return 0;
    }
    return verifier() == 0;
}

/**
 * Affichage complet
 */
void affiche()
{
    int i, j, c, l;
    int cols;
    char v1, v2;
    char *s1 = "+-------+-------+-------+";
    char cur = resolv[ycur*9+xcur];
    int fini = 0;
    int menu = offX + strlen(s1) + 8;

    //    traces();
    clear();
    mvprintw(0, 7, "SUDOKU by Jean MORLET");
    mvprintw(1, 6, "=======================");

    if (complet())
    {
        attron(COLOR_PAIR(VERT));
        mvprintw(2, 6, "       ! BRAVO !");
        attroff(COLOR_PAIR(NORMAL));
        fini = 1;
    }
    for (l=offY, j=0; j<9; j++, l++)
    {
        if (j%3 == 0)
        {
            if (fini)
                attron(COLOR_PAIR(VERT));
            mvprintw(l++, offX, s1);
            attron(COLOR_PAIR(NORMAL));
        }
        for (c=offX, i=0; i<9; i++)
        {
            if (i%3 == 0)
            {
                if (fini)
                    attron(COLOR_PAIR(VERT));
                mvaddch(l, c++, '|');
                attron(COLOR_PAIR(NORMAL));
                mvaddch(l, c++, ' ');
            }
            if (affpossible)
            {
                int nb = nbpossibles(j, i);
                if (resolv[j*9+i] == CASE_VIDE)
                {
                    if (nb == 1)
                        attrset(COLOR_PAIR(VERT));
                    mvaddch(l, c++, nb+'0');
                }
                else
                    mvaddch(l, c++, ' ');
            }
            else
            {
                v1 = table[j*9+i];
                v2 = resolv[j*9+i];
                if (tberreurs[j*9+i])
                    attrset(COLOR_PAIR(couleurs[0][COL_ERREUR]));
                else if (v1 != CASE_VIDE)
                    attrset(COLOR_PAIR(couleurs[0][COL_BASE]));
                else
                    attrset(COLOR_PAIR(couleurs[0][COL_NORMAL]));
                if (resolv[ycur*9+xcur] != CASE_VIDE && resolv[j*9+i] == resolv[ycur*9+xcur])
                    attron(A_BOLD);
                mvaddch(l, c++, v2 == CASE_VIDE ? v1 : v2);
                attroff(A_NORMAL);
            }
            attrset(COLOR_PAIR(NORMAL));
            mvaddch(l, c++, ' ');
        }
        if (fini)
            attron(COLOR_PAIR(VERT));
        addch('|');
        attron(COLOR_PAIR(NORMAL));
    }
    if (fini)
        attron(COLOR_PAIR(VERT));
    mvprintw(l++, offX, s1);
    attron(COLOR_PAIR(NORMAL));
    if (mesure > 0)
        mvprintw(l, offX, "Temps de calcul = %.03fms", mesure);

    l = 3;
    mvprintw(l++, menu, "1-9   Saisie d'un chiffre");
    mvprintw(l++, menu, "c     Effacer chiffre");
    mvprintw(l++, menu, "v     Vérifier en temps réel : ");
    if (verify)
    {
        attron(COLOR_PAIR(VERT));
        printw("ON");
        attroff(COLOR_PAIR(NORMAL));
    }
    else
    {
        attron(COLOR_PAIR(ERREUR));
        printw("OFF");
        attroff(COLOR_PAIR(NORMAL));
    }
    mvprintw(l++, menu, "x     Chercher la solution en X : ");
    if (typeX)
    {
        attron(COLOR_PAIR(VERT));
        printw("ON");
        attroff(COLOR_PAIR(NORMAL));
    }
    else
    {
        attron(COLOR_PAIR(ERREUR));
        printw("OFF");
        attroff(COLOR_PAIR(NORMAL));
    }
    mvprintw(l++, menu, "p     Afficher possibles : ");
    if (possible)
    {
        attron(COLOR_PAIR(VERT));
        printw("ON");
        attroff(COLOR_PAIR(NORMAL));
    }
    else
    {
        attron(COLOR_PAIR(ERREUR));
        printw("OFF");
        attroff(COLOR_PAIR(NORMAL));
    }
    mvprintw(l++, menu, "' '   Afficher nb possibles : ");
    if (affpossible)
    {
        attron(COLOR_PAIR(VERT));
        printw("ON");
        attroff(COLOR_PAIR(NORMAL));
    }
    else
    {
        attron(COLOR_PAIR(ERREUR));
        printw("OFF");
        attroff(COLOR_PAIR(NORMAL));
    }
    mvprintw(l++, menu, "p     Afficher les mêmes : ");
    mvprintw(l++, menu, "t     Déplacement : ");
    attron(COLOR_PAIR(BASE));
    printw(touches[jeutouches]);
    attroff(COLOR_PAIR(NORMAL));
    mvprintw(l++, menu, "$     Remplir singletons");
    mvprintw(l++, menu, "*     Chercher une(la?) solution");
    mvprintw(l++, menu, "r     Remise à zéro de la grille");
    mvprintw(l++, menu, "n     Saisir une nouvelle grille");
    mvprintw(l++, menu, "ESCx2 Sortir du jeu");

    if (possible)
        for (i=0; i<9; i++)
        {
            if (possibles[ycur*9+xcur][i])
                attron(COLOR_PAIR(VERT));
            mvprintw(i+5, offX+28, "%c", '1'+i);
            attroff(COLOR_PAIR(NORMAL));
            mvprintw(LINES-1, i, "%d", possibles[ycur*9+xcur][i]);
        }

    // Repositionnement du curseur
    move(offY+ycur+ycur/3+1, offX+(xcur+xcur/3+1)*2);

    refresh();
}

void tstpossible(int y, int x)
{
    int i, l, c;
    if (resolv[y*9+x] != CASE_VIDE)
        return;
    //memset(possibles[y*9+x], 1, 9);
    for (i=0; i<9; i++) possibles[y*9+x][i]=1;
    for (i=0; i<9; i++)
    {
        for (l=0; l<9; l++)
            if (resolv[l*9+x] == '1'+i)
                possibles[y*9+c][i] = 0;
        for (c=0; c<9; c++)
            if (resolv[y*9+c] == '1'+i)
                possibles[y*9+c][i] = 0;
        for (l=0; l<3; l++)
            for (c=0; c<3; c++)
                if (resolv[((y/3)*3+l)*9+(x/3)*3+c] == '1'+i)
                    possibles[y*9+c][i] = 0;
    }
}

void setpossibles()
{
    int i, l, c, y, x;
    for (int y=0; y<9; y++)
    {
        for (int x=0; x<9; x++)
        {
            if (resolv[y*9+x] != CASE_VIDE)
            {
                memset(possibles[y*9+x], 0, 9);
                continue;
            }
            memset(possibles[y*9+x], 1, 9);
            //for (i=0; i<9; i++) possibles[y*9+x][i]=1;
            for (i=0; i<9; i++)
            {
                for (l=0; l<9; l++)
                    if (resolv[l*9+x] == '1'+i)
                        possibles[y*9+x][i] = 0;
                for (c=0; c<9; c++)
                    if (resolv[y*9+c] == '1'+i)
                        possibles[y*9+x][i] = 0;
                for (l=0; l<3; l++)
                    for (c=0; c<3; c++)
                        if (resolv[((y/3)*3+l)*9+(x/3)*3+c] == '1'+i)
                            possibles[y*9+x][i] = 0;
            }
        }
    }
}

/**
 * Mise à jour de la table des possibles en supprimant une
 * valeur ayant été positionnée en lxc
 * Il faut supprimer la valeur sur la ligne, la colonne et la région.
 */
void majpossibles(int l, int c, int v)
{
    v--;
    for (int i=0; i<9; i++)
    {
        possibles[i*9+c][v] = 0;
        possibles[l*9+i][v] = 0;
        possibles[((l/3)*3+i/3)*9+(c/3)*3+i%3][v] = 0;
    }
}

/**
 * Retourne le nombre de possible pour une case donnée.
 */
int nbpossibles(int l, int c)
{
    int off = l*9+c;
    int nb = 0;
    for (int i=0; i<9; i++)
        nb += possibles[off][i];
    return nb;
}

/**
 * Vérifie qu'un touche de déplacement est
 * dans le groupe sélectionné.
 */
int veriftouche(char c)
{
    if (jeutouches == 2)
    {
        for (int i=0; i<4; i++)
            if (c == "ABCD"[i])
                return 0;
    }
    else
    {
        for (int i=0; i<4; i++)
            if (c == touches[jeutouches][i])
                return 0;
    }
    return 1;
}

void gauche(char c)
{
    if (veriftouche(c))
        return;
    if (xcur == 0)
        xcur = 8;
    else
        xcur--;
}

void bas(char c)
{
    if (veriftouche(c))
        return;
    if (ycur == 8)
        ycur = 0;
    else
        ycur++;
}

void haut(char c)
{
    if (veriftouche(c))
        return;
    if (ycur == 0)
        ycur = 8;
    else
        ycur--;
}

void droite(char c)
{
    if (veriftouche(c))
        return;
    if (xcur == 8)
        xcur = 0;
    else
        xcur++;
}

int chercheVide(int *x, int *y)
{
    int r, c;
    for (c=0; c<9; c++)
        for (r=0; r<9; r++)
            if (resolv[r*9+c] == '.')
            {
                *x=r;
                *y=c;
                return 1;
            }
    return 0;
}

int isOk(int r, int c, int val)
{
    int i, j;
    for (i=0; i<9; i++)
        if (resolv[i*9+c] == val)
            return 0;
    for (j=0; j<9; j++)
        if (resolv[r*9+j] == val)
            return 0;
    for (i=3*(r/3); i<3*(1+r/3); i++)
        for (j=3*(c/3); j<3*(1+c/3); j++)
                if (resolv[i*9+j] == val)
                    return 0;
    return 1;
}

/**
 * Résolution de la grille courante par force brute.
 */
int brut()
{
    int i;
    int r=0, c=0;
    if (!chercheVide(&r, &c))
        return 1;
    for (i='1'; i<='9'; i++)
    {
        if (isOk(r, c, i))
        {
            resolv[r*9+c] = i;
            if (brut())
                return 1;
            resolv[r*9+c] = '.';
        }
    }
    return 0;
}

/**
 * Saisie d'un chiffre à la main
 */
int chiffre(int c)
{
    // Si chiffre de base, on ne doit pas le modifier
    if (table[ycur*9+xcur] != CASE_VIDE)
        return c;
    // S'il s'agit bien d'un chiffre
    if (c > '0' && c <= '9')
    {
        // Le renseigner dans la table de résolution
        resolv[ycur*9+xcur] = (char)c;
        // Et le supprimer de la table des possibles
        majpossibles(ycur, xcur, c-'1');
        // Et effacer les possibles de cette case
        memset(possibles[ycur*9+xcur], 0, 9);
        return 0;
    }
    return c;
}

int main(void)
{
    int err;
    int c;
    for (int i=0; i<81; i++)
        table[i] = tsts[tst][i];
    memcpy(resolv, table, 81);
    memset(tberreurs, 0, 81);
    initscr();
    raw();
    noecho();
    start_color();
    init_pair(BASE, COLOR_CYAN, COLOR_BLACK);
    init_pair(NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(ERREUR, COLOR_RED, COLOR_BLACK);
    init_pair(BASE_, COLOR_BLUE, COLOR_YELLOW);
    init_pair(NORMAL_, COLOR_BLACK, COLOR_YELLOW);
    init_pair(ERREUR_, COLOR_RED, COLOR_YELLOW);
    init_pair(VERT, COLOR_GREEN, COLOR_BLACK);
    init_pair(RVERT, COLOR_BLACK, COLOR_GREEN);

#if TRACES
    offX = COLS/2-13;
    offY = 3;
#else
    offX = 5;
    offY = 3;
#endif

    affiche();

    for (;;)
    {
        mesure = 0;
        //setpossibles();
        c = getchar();

        switch (c)
        {
        case 0x1b:
            switch (getchar())
            {
            case 0x1b:
                endwin();
                exit(0);

            case '[':
                switch (getchar())
                {
                case 'A':
                    haut('A');
                    break;
                case 'B':
                    bas('B');
                    break;
                case 'C':
                    droite('C');
                    break;
                case 'D':
                    gauche('D');
                    break;
                }
            }
            break;

        case 'h':
        case 'H':
        case 'q':
        case 'Q':
            gauche(c);
            break;

        case 'j':
        case 's':
        case 'J':
        case 'S':
            bas(c);
            break;

        case 'k':
        case 'z':
        case 'K':
        case 'Z':
            haut(c);
            break;

        case 'l':
        case 'd':
        case 'L':
        case 'D':
            droite(c);
            break;

        case 'r':
        case 'R':
            memcpy(resolv, table, 81);
            if (possible)
                setpossibles();
            break;

        case 'c':
        case 'C':
            resolv[ycur*9+xcur] = table[ycur*9+xcur];
            break;

        case 'n':
        case 'N':
            memset(table, ' ', 81);
            memset(resolv, ' ', 81);
            for (ycur=0; ycur<9; ycur++)
                for (xcur=0; xcur<9; xcur++)
                {
                    affiche();
                    for (;;)
                    {
                        c = getchar();
                        if (c == CASE_VIDE || c == ';' || (c >= '1' && c <= '9'))
                        {
                            table[ycur*9+xcur] = c == ';' ? CASE_VIDE : c;
                            resolv[ycur*9+xcur] = c == ';' ? CASE_VIDE : c;
                            break;
                        }
                        else if (c == 'q')
                        {
                            endwin();
                            exit(0);
                        }
                        else if (c == 'h')
                        {
                            if (xcur == 0)
                            {
                                if (ycur > 0)
                                    ycur--, xcur=8;
                            }
                            else
                                xcur--;
                            affiche();
                        }
                    }
                }
            xcur = ycur = 0;
            break;

        case '*':
            {
                struct timespec debut;
                starttime(&debut);
                brut();
                mesure = elapsed(&debut);
                break;
            }

        case 'v':
        case 'V':
            verify ^= 1;
            memset(tberreurs, 0, 81);
            break;

        case 'x':
        case 'X':
            typeX ^= 1;
            memset(tberreurs, 0, 81);
            break;

        case 'p':
        case 'P':
            possible ^= 1;
            if (possible)
                setpossibles();
            break;

        case 't':
        case 'T':
            jeutouches = (jeutouches+1)%3;
            break;

        case '+':
            tst = (tst + 1) % (sizeof(tsts)/sizeof(char *));
            for (int i=0; i<81; i++)
                table[i] = tsts[tst][i];
            memcpy(resolv, table, 81);
            memset(tberreurs, 0, 81);
            if (possible)
                setpossibles();
            break;

        case ' ':
            affpossible ^= 1;

        default:
            chiffre(c);
            break;
        }

        if (verify)
            verifier();
        affiche();
    }
}

// Fin de fichier
