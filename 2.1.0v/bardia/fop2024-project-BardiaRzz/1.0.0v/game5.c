#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// Taff-e komaki baraye chap matn dar miyane safhe
void center_print(int row, const char *text) {
    int col = (COLS - (int)strlen(text)) / 2;
    mvprintw(row, col, "%s", text);
}

// Taff-e namayesh menu asli bazi
int showMainMenu() {
    clear();
    attron(A_BOLD | COLOR_PAIR(3));
    center_print(2, "Menu Asli Bazi");
    attroff(A_BOLD | COLOR_PAIR(3));

    // Namayesh gozinehaye menu
    center_print(4, "1) Sakht Bazi Jadid");
    center_print(5, "2) Edame Bazi Ghabl");
    center_print(6, "3) Jadval Emtiyazat");
    center_print(7, "4) Tanzimat");
    center_print(8, "5) Menu Profile");
    center_print(9, "6) Khorooj");
    refresh();

    int ch = getch();
    return ch;
}

// Taff-e sakht bazi jadid
void newGame() {
    clear();
    attron(COLOR_PAIR(1) | A_BOLD);
    center_print(4, "Shoroo Bazi Jadid az Marhale Ebtedai...");
    attroff(COLOR_PAIR(1) | A_BOLD);

    center_print(6, "Tanzimat Pish Bazi dar hale barlodi mibashand...");
    refresh();

    // Generate a unique save file name
    char saveFileName[50];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(saveFileName, sizeof(saveFileName), "save_%d-%02d-%02d_%02d-%02d-%02d.txt",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);

    // Open the file to write initial game state
    FILE *saveFile = fopen(saveFileName, "w");
    if (saveFile == NULL) {
        attron(COLOR_PAIR(2));
        center_print(8, "Nemitavan save ra ijad kard!");
        attroff(COLOR_PAIR(2));
        refresh();
        getch();
        return;
    }

    // Writing initial game data (example: level 1, score 0)
    fprintf(saveFile, "Level: 1\n");
    fprintf(saveFile, "Score: 0\n");
    fprintf(saveFile, "Health: 100\n");
    fprintf(saveFile, "Gold: 0\n");

    fclose(saveFile);

    // Confirmation message
    attron(COLOR_PAIR(1));
    center_print(8, "Bazi jadid save shod!");
    attroff(COLOR_PAIR(1));
    refresh();
    getch();

    // Terminate the current game before running the system call
    endwin();  // This will terminate the ncurses environment

    // Run the system call to compile and run the game
    system("gcc game.c -o game.out -lncurses && ./game.out");
}

// Taff-e edame bazi ghabl
void resumeGame() {
    clear();
    attron(COLOR_PAIR(1) | A_BOLD);
    center_print(2, "Edame Bazi Ghabl");
    attroff(COLOR_PAIR(1) | A_BOLD);

    // Namayesh list-e bazi haye zakhire shode. Dar in mesal, tanha yek namayesh sade ast.
    center_print(4, "List-e Bazi-haye Zakhire Shode:");
    center_print(6, "1) Bazi Shomare 1 - Zakhire dar: Marhale 3");
    center_print(7, "2) Bazi Shomare 2 - Zakhire dar: Marhale 5");
    center_print(8, "3) Bazi Shomare 3 - Zakhire dar: Marhale 2");
    refresh();

    center_print(10, "Baraye entekhab bazi, adade marbut ra feshar dahid...");
    int choice = getch();
    
    // Inja mitavanid bazi entekhab shode ra barghozari va be ohde-ye zakhire shode bargardid.
    clear();
    char msg[80];
    snprintf(msg, sizeof(msg), "Dar hale edame bazi shomare %c...", choice);
    center_print(5, msg);
    refresh();
    getch();
}

// Taff-e namayesh jadval emtiyazat
void showLeaderboard() {
    clear();
    attron(COLOR_PAIR(1) | A_BOLD);
    center_print(2, "Jadval Emtiyazat Nafarat-e Bartar Bazi");
    attroff(COLOR_PAIR(1) | A_BOLD);

    /* Dar in ghesmat farz mikonim yek jadval sabet baraye namayesh darim.
       Dar har satr etelaate zir namayesh dade khahad shod:
       1. Rotbe karbar
       2. Nam-e karbari
       3. Jam-e emtiyazat
       4. Jam-e zakhire talayi karbar
       5. Tedad bazi-haye be enteham reside
       6. Moddate tajrobe
       7. Neshan dadan sator mokhtalef baraye karbari ke login karde ast.
    */
    int baseRow = 4;
    // Sar jadval
    mvprintw(baseRow, 5, "Rotbe   Nam-e Karbari   Emtiyazat   Talayi Zakhire Shode   Bazi-ha-ye Entemam Shode   Tajrobe");
    // Satr-e namuno baraye karbar-e login shode (ba rang ya font-e mokhtalef)
    attron(A_BOLD);
    mvprintw(baseRow + 2, 5, "1      Ali         1500       500                10              2sa'at");
    attroff(A_BOLD);
    // Satr-haye digar
    mvprintw(baseRow + 4, 5, "2      Sara        1400       450                8               1sa'at");
    mvprintw(baseRow + 6, 5, "3      Reza        1300       400                7               50daqiqe");
    refresh();
    center_print(baseRow + 8, "Baraye bazgasht be menu, har kelidi ra feshar dahid...");
    getch();
}

// Taff-e tanzimat
void settingsMenu() {
    clear();
    attron(COLOR_PAIR(1) | A_BOLD);
    center_print(2, "Tanzimat Bazi");
    attroff(COLOR_PAIR(1) | A_BOLD);

    // Inja mitavan tanzimat mokhtalefi mesle seda, grafik, kontrol va ... ra gharar dad.
    center_print(4, "1) Tanzim-e Sorat Bazi");
    center_print(5, "2) Tanzim-e Mizan-e Sakhti");
    center_print(6, "3) Tanzim-e Gozinehaye Ezafi");
    refresh();

    center_print(8, "Baraye bazgasht be menu, har kelidi ra feshar dahid...");
    getch();
}

// Taff-e menu profile
void profileMenu() {
    clear();
    attron(COLOR_PAIR(1) | A_BOLD);
    center_print(2, "Menu Profile Karbari");
    attroff(COLOR_PAIR(1) | A_BOLD);

    // Namayesh etela'at-e karbar (mesal: nam-e karbari, emtiyaz-e kol, sathe tajrobe va gheyre)
    center_print(4, "Nam-e Karbari: User123");
    center_print(5, "Emtiyaz-e Kol: 1500");
    center_print(6, "Sathe Tajrobe: Motavaset");
    center_print(7, "Tedad Bazi-haye Entemam Shode: 10");

    // Gozinehaye taghyir etela'at-e profile
    center_print(9, "1) Virayesh Profile");
    center_print(10, "2) Taghyir Ramz-e Obor");
    refresh();

    center_print(12, "Baraye bazgasht be menu, har kelidi ra feshar dahid...");
    getch();
}

int main() {
    // Rahandazi ncurses
    initscr();
    noecho();
    cbreak();
    curs_set(0); // Makhfi kardan cursor

    // Rahandazi rang-ha
    if(has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);  // Payam haye movafagh
        init_pair(2, COLOR_RED, COLOR_BLACK);      // Payam haye khata
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);   // Onvaneha
    }

    int exitProgram = 0;
    while (!exitProgram) {
        int choice = showMainMenu();
        switch(choice) {
            case '1': // Sakht Bazi Jadid
                newGame();
                break;
            case '2': // Edame Bazi Ghabl
                resumeGame();
                break;
            case '3': // Jadval Emtiyazat
                showLeaderboard();
                break;
            case '4': // Tanzimat
                settingsMenu();
                break;
            case '5': // Menu Profile
                profileMenu();
                break;
            case '6': // Khorooj
                exitProgram = 1;
                break;
            default:
                // Dar soorate vorood kelid-e namotabar, payam-e khata namayesh dade shavad.
                clear();
                attron(COLOR_PAIR(2));
                center_print(5, "Gozine namotabar ast. Lotfan dobare talash konid.");
                attroff(COLOR_PAIR(2));
                refresh();
                getch();
                break;
        }
    }

    // Payan-e mohit-e ncurses
    endwin();

    return 0;
}
