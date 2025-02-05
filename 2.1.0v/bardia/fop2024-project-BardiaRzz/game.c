#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include <ctype.h>
#include <stdbool.h>

// ثابت‌های مربوط به نقشه
#define MAP_WIDTH 80
#define MAP_HEIGHT 24

// حداکثر ظرفیت کاربران در این دمو
#define MAX_USERS 10

// ساختار داده برای اطلاعات کاربر
typedef struct
{
    char username[50];
    char password[50];
    int score;
} User;

// ساختار داده برای بازیکن
typedef struct
{
    int x, y;
    int health;
    int gold;
    char symbol;
} Player;

// ساختار داده برای دشمن
typedef struct
{
    int x, y;
    int health;
    char symbol;
    bool active;
} Enemy;

// متغیرهای سراسری برای لیست کاربران
User users[MAX_USERS];
int usersCount = 0;

// ترمینال و صفحه اصلی ncurses
WINDOW *mainwin;

// پیش‌اعلام توابع
void initScreen();
void cleanupAndExit();
void showMainMenu();
void registerUser();
void loginUser();
bool validatePassword(const char *password);
void drawDungeonMap();
void showPreGameMenu();
void showScoreboard();
void handleGameLoop();
void drawBorders();
void drawPlayer(Player *p);
void drawEnemy(Enemy *e);

// تابع اصلی
int main(void)
{
    usersCount = getUserCount();
    initScreen();
    showMainMenu();
    cleanupAndExit();
    return 0;
}

// مقداردهی اولیه‌ی ncurses
void initScreen()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // فعال کردن رنگ‌ها (در صورت پشتیبانی)
    if (has_colors())
    {
        start_color();
        // تعریف چند رنگ نمونه
        init_pair(1, COLOR_WHITE, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_RED, COLOR_BLACK);
    }
    mainwin = newwin(MAP_HEIGHT, MAP_WIDTH, 0, 0);
    srand((unsigned int)time(NULL));
}

// پایان کار با ncurses
void cleanupAndExit()
{
    endwin();
}

// نمایش منوی اصلی
void showMainMenu()
{
    while (1)
    {
        werase(mainwin);
        box(mainwin, 0, 0);
        mvwprintw(mainwin, 2, 2, "***** Main Menu *****");
        mvwprintw(mainwin, 4, 4, "1) Register new user");
        mvwprintw(mainwin, 5, 4, "2) Login");
        mvwprintw(mainwin, 6, 4, "3) Exit");
        mvwprintw(mainwin, 7, 4, "4) Guest");
        mvwprintw(mainwin, 8, 4, "Enter your choice: ");
        wrefresh(mainwin);

        int choice = wgetch(mainwin);
        switch (choice)
        {
        case '1':
            registerUser();
            break;
        case '2':
            loginUser();
            break;
        case '3':
            return; // خروج از showMainMenu و رفتن به cleanupAndExit
        case '4':
            showPreGameMenu();
            break;
        }
    }
}

// تابع ثبت کاربر جدید
void registerUser()
{
    if (usersCount >= MAX_USERS)
    {
        mvwprintw(mainwin, 10, 2, "User limit reached!");
        wrefresh(mainwin);
        wgetch(mainwin);
        return;
    }
    echo(); // اجازه دادن به نمایش کاراکترهای تایپ‌شده
    char username[50], password[50], confirm[50];
    werase(mainwin);
    box(mainwin, 0, 0);
    mvwprintw(mainwin, 2, 2, "=== Register New User ===");
    mvwprintw(mainwin, 4, 2, "Username: ");
    wmove(mainwin, 4, 12);
    wgetnstr(mainwin, username, 49);

    // read users i from txt
    FILE *fp = fopen("users.txt", "r");
    if (fp == NULL)
    {
        return;
    }
    int count = 0;
    char fileUsername[50], filePassword[50];
    while (
        fscanf(fp, "%s %s", fileUsername, filePassword) == 2)
    {
        strcpy(users[count].username, fileUsername);
        strcpy(users[count].password, filePassword);
        count++;
    }

    // بررسی یکتا بودن نام کاربری
    for (int i = 0; i < usersCount; i++)
    {
        if (strcmp(users[i].username, username) == 0)
        {
            mvwprintw(mainwin, 6, 2, "Username already exists!");
            wrefresh(mainwin);
            noecho();
            wgetch(mainwin);
            return;
        }
    }

    mvwprintw(mainwin, 6, 2, "Password: ");
    wmove(mainwin, 6, 12);
    wgetnstr(mainwin, password, 49);

    mvwprintw(mainwin, 7, 2, "Confirm: ");
    wmove(mainwin, 7, 11);
    wgetnstr(mainwin, confirm, 49);

    // بررسی مطابقت رمزها + اعتبارسنجی رمز عبور
    if (strcmp(password, confirm) != 0)
    {
        mvwprintw(mainwin, 9, 2, "Passwords don't match!");
        wrefresh(mainwin);
        wgetch(mainwin);
        return;
    }
    if (!validatePassword(password))
    {
        mvwprintw(mainwin, 9, 2, "Weak Password! Need at least 7 chars with uppercase, lowercase & digits.");
        wrefresh(mainwin);
        wgetch(mainwin);
        return;
    }

    // get the email
    char email[50];
    mvwprintw(mainwin, 8, 2, "Email: ");
    wmove(mainwin, 8, 12);
    wgetnstr(mainwin, email, 49);
    noecho();

    // validate email
    if (!validateEmail(email))
    {
        mvwprintw(mainwin, 10, 2, "Invalid email format!");
        wrefresh(mainwin);
        wgetch(mainwin);
        return;
    }

    // ثبت کاربر
    strcpy(users[usersCount].username, username);
    strcpy(users[usersCount].password, password);
    users[usersCount].score = 0;
    usersCount++;

    // ذخیره اطلاعات کاربر در فایل
    FILE *f = fopen("users.txt", "a");
    if (f == NULL)
    {
        return;
    }
    fprintf(f, "%s %s\n", username, password);
    fclose(f);

    mvwprintw(mainwin, 11, 2, "Registration successful!");
    wrefresh(mainwin);
    wgetch(mainwin);
}

// تابع ساده برای بررسی قدرت رمز عبور
bool validatePassword(const char *password)
{
    if (strlen(password) < 7)
        return false;
    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;
    for (int i = 0; password[i]; i++)
    {
        if (isupper(password[i]))
            hasUpper = true;
        if (islower(password[i]))
            hasLower = true;
        if (isdigit(password[i]))
            hasDigit = true;
    }
    return (hasUpper && hasLower && hasDigit);
}

// تابع ورود کاربر
void loginUser()
{
    echo();
    char username[50], password[50];
    werase(mainwin);
    box(mainwin, 0, 0);
    mvwprintw(mainwin, 2, 2, "=== Login ===");
    mvwprintw(mainwin, 4, 2, "Username: ");
    wmove(mainwin, 4, 12);
    wgetnstr(mainwin, username, 49);

    mvwprintw(mainwin, 5, 2, "Password: ");
    wmove(mainwin, 5, 12);
    wgetnstr(mainwin, password, 49);

    noecho();

    // read users from txt
    FILE *fp = fopen("users.txt", "r");
    if (fp == NULL)
    {
        return;
    }
    int count = 0;
    char fileUsername[50], filePassword[50];
    while (
        fscanf(fp, "%s %s", fileUsername, filePassword) == 2)
    {
        strcpy(users[count].username, fileUsername);
        strcpy(users[count].password, filePassword);
        count++;
    }

    // جستجو در میان کاربران
    for (int i = 0; i < usersCount; i++)
    {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0)
        {
            mvwprintw(mainwin, 7, 2, "Login successful!");
            wrefresh(mainwin);
            wgetch(mainwin);
            showPreGameMenu(); // نمایش منوی پیش از بازی
            return;
        }
    }
    mvwprintw(mainwin, 7, 2, "Invalid credentials!");
    wrefresh(mainwin);
    wgetch(mainwin);
}

// منوی پیش از بازی (New Game, Continue,...)
void showPreGameMenu()
{
    while (1)
    {
        werase(mainwin);
        box(mainwin, 0, 0);
        mvwprintw(mainwin, 2, 2, "*** Pre-Game Menu ***");
        mvwprintw(mainwin, 4, 4, "1) Start New Game");
        mvwprintw(mainwin, 5, 4, "2) Continue Game (not implemented in demo)");
        mvwprintw(mainwin, 6, 4, "3) View Scoreboard");
        mvwprintw(mainwin, 7, 4, "4) Settings (not implemented in demo)");
        mvwprintw(mainwin, 8, 4, "5) Exit to Main Menu");
        mvwprintw(mainwin, 10, 4, "Enter your choice: ");
        wrefresh(mainwin);

        int choice = wgetch(mainwin);
        switch (choice)
        {
        case '1':
            // اجرای بازی جدید
            handleGameLoop();
            break;
        case '2':
            // ادامه بازی ذخیره‌شده (در این دمو پیاده‌سازی نشده)
            break;
        case '3':
            // نمایش امتیازات
            showScoreboard();
            break;
        case '4':
            // تنظیمات بازی (پیاده‌سازی نشده)
            break;
        case '5':
            return;
        }
    }
}

// نمایش امتیازات (Simple Demo)
void showScoreboard()
{
    // werase(mainwin);
    // box(mainwin, 0, 0);
    // mvwprintw(mainwin, 2, 2, "===== SCOREBOARD =====");
    // for (int i = 0; i < usersCount; i++)
    // {
    //     mvwprintw(mainwin, 4 + i, 4, "%d) %s - Score: %d", i + 1, users[i].username, users[i].score);
    // }
    // wrefresh(mainwin);
    // wgetch(mainwin);

    // read scores from scores.txt
    FILE *fp = fopen("scores.txt", "r");
    if (fp == NULL)
    {
        return;
    }
    int count = 0;
    char fileUsername[50];
    int fileScore;
    // delete before print
    werase(mainwin);
    while (
        fscanf(fp, "%s %d", fileUsername, &fileScore) == 2)
    {
        mvwprintw(mainwin, 4 + count, 4, "%d) %s - Score: %d", count + 1, fileUsername, fileScore);
        count++;
    }
    fclose(fp);
    wrefresh(mainwin);
    wgetch(mainwin);


}

// نمایش ساده نقشه سیاه‌چال (Dungeon)
void drawDungeonMap()
{
    // مقداردهی اولیه پس‌زمینه
    for (int y = 0; y < MAP_HEIGHT - 2; y++)
    {
        for (int x = 0; x < MAP_WIDTH - 2; x++)
        {
            mvwaddch(mainwin, y + 1, x + 1, '.');
        }
    }
    // ترسیم دیوارهای ساده
    for (int x = 0; x < MAP_WIDTH - 2; x++)
    {
        mvwaddch(mainwin, 1, x + 1, '_');              // بالای نقشه
        mvwaddch(mainwin, MAP_HEIGHT - 2, x + 1, '_'); // پایین نقشه
    }
    for (int y = 0; y < MAP_HEIGHT - 2; y++)
    {
        mvwaddch(mainwin, y + 1, 1, '|');             // چپ نقشه
        mvwaddch(mainwin, y + 1, MAP_WIDTH - 2, '|'); // راست نقشه
    }

    // مثال ورود دو اتاق ساده به صورت نمادین
    // اتاق ۱
    for (int y = 5; y < 10; y++)
    {
        for (int x = 5; x < 15; x++)
        {
            mvwaddch(mainwin, y, x, ' ');
        }
    }
    // درب ساده برای اتاق ۱
    mvwaddch(mainwin, 7, 15, '+');

    // اتاق ۲
    for (int y = 12; y < 16; y++)
    {
        for (int x = 30; x < 40; x++)
        {
            mvwaddch(mainwin, y, x, ' ');
        }
    }
    mvwaddch(mainwin, 14, 30, '+');

    // کریدور برای اتصال دو اتاق
    for (int x = 15; x < 30; x++)
    {
        mvwaddch(mainwin, 7, x, '#');
    }
}

// حلقه اصلی بازی
void handleGameLoop()
{
    // run file game3.c
    system("gcc game3.c -o game.out -lncurses -lm && ./game.out");
}

// رسم بازیکن
void drawPlayer(Player *p)
{
    mvwaddch(mainwin, p->y, p->x, p->symbol);
}

// رسم دشمن
void drawEnemy(Enemy *e)
{
    if (e->active)
    {
        mvwaddch(mainwin, e->y, e->x, e->symbol);
    }
}

int getUserCount()
{
    FILE *fp = fopen("users.txt", "r");
    if (fp == NULL)
    {
        return 0;
    }
    char fileUsername[50], filePassword[50];
    int count = 0;
    while (
        fscanf(fp, "%s %s", fileUsername, filePassword) == 2)
    {
        count++;
    }
    return count;
}

// write validate email to be x@y.z
int validateEmail(const char *email)
{
    char *atPtr = strchr(email, '@');
    if (atPtr == NULL || atPtr == email || *(atPtr + 1) == '\0')
    {
        return 0;
    }
    char *dotPtr = strchr(atPtr, '.');
    if (dotPtr == NULL || dotPtr == atPtr + 1 || *(dotPtr + 1) == '\0')
    {
        return 0;
    }
    return 1;
}