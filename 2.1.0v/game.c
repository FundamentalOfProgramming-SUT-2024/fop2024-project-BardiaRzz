#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include <ctype.h>
#include <stdbool.h>


#define MAP_WIDTH 80
#define MAP_HEIGHT 24


#define MAX_USERS 10


typedef struct
{
    char username[50];
    char password[50];
    long firstGameTime;
    int totalScore;     
    int totalGold;      
    int gamesPlayed;    
} User;


typedef struct
{
    int x, y;
    int health;
    int gold;
    char symbol;
} Player;


typedef struct
{
    int x, y;
    int health;
    char symbol;
    bool active;
} Enemy;


User users[MAX_USERS];
int usersCount = 0;


WINDOW *mainwin;


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
void saveUsersToFile();
void updateUserData(User *user, int gainedScore, int gainedGold);
int getUserCount();
int validateEmail(const char *email);
int compareUsers(const void *a, const void *b);


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
        strncpy(users[count].username, fileUsername, sizeof(users[count].username) - 1);
        users[count].username[sizeof(users[count].username) - 1] = '\0';
        strncpy(users[count].password, filePassword, sizeof(users[count].password) - 1);
        users[count].password[sizeof(users[count].password) - 1] = '\0';
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
    users[usersCount].firstGameTime = 0;
    users[usersCount].totalScore = 0;
    users[usersCount].totalGold = 0;
    users[usersCount].gamesPlayed = 0;
    usersCount++;

    // ذخیره اطلاعات کاربر در فایل
    FILE *f = fopen("users.txt", "a");
    if (f == NULL)
    {
        return;
    }
    fprintf(f, "%s %s %ld %d %d %d\n", username, password, 0L, 0, 0, 0);
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
    long firstGameTime;
    int totalScore, totalGold, gamesPlayed;
    while (
        fscanf(fp, "%s %s %ld %d %d %d", fileUsername, filePassword, &firstGameTime, &totalScore, &totalGold, &gamesPlayed) == 6)
    {
        strcpy(users[count].username, fileUsername);
        strcpy(users[count].password, filePassword);
        users[count].firstGameTime = firstGameTime;
        users[count].totalScore = totalScore;
        users[count].totalGold = totalGold;
        users[count].gamesPlayed = gamesPlayed;
        count++;
    }
    fclose(fp);

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
    // read scores from users.txt
    FILE *fp = fopen("users.txt", "r");
    if (fp == NULL)
    {
        return;
    }
    int count = 0;
    char fileUsername[50], filePassword[50];
    long firstGameTime;
    int totalScore, totalGold, gamesPlayed;
    while (
        fscanf(fp, "%s %s %ld %d %d %d", fileUsername, filePassword, &firstGameTime, &totalScore, &totalGold, &gamesPlayed) == 6)
    {
        strcpy(users[count].username, fileUsername);
        strcpy(users[count].password, filePassword);
        users[count].firstGameTime = firstGameTime;
        users[count].totalScore = totalScore;
        users[count].totalGold = totalGold;
        users[count].gamesPlayed = gamesPlayed;
        count++;
    }
    fclose(fp);

    // مرتب‌سازی کاربران بر اساس مجموع امتیاز
    qsort(users, count, sizeof(User), compareUsers);

    // نمایش جدول امتیازات
    clear();
    mvprintw(0, 0, "Jadval Emtiyazat");
    mvprintw(2, 0, "Rotbe | Nam Karbar        | Majmoe Emtiyaz | Majmoe Tala | Bazi-haye Tamam-shode | Tajrobe (Rooz)");
    mvprintw(3, 0, "-----------------------------------------------------------------------");

    time_t now = time(NULL); // زمان فعلی برای محاسبه تجربه

    for (int i = 0; i < count; i++)
    {
        int rank = i + 1;

        // محاسبه اختلاف زمانی (تجربه) بر حسب روز
        long diffSeconds = now - users[i].firstGameTime;
        int diffDays = diffSeconds / (60 * 60 * 24);

        // بررسی ویژگی کاربر جاری؛ اگر نام کاربری با currentUsername برابر بود، highlight شود
        bool isCurrentUser = (strcmp(users[i].username, users[i].username) == 0);

        // بررسی آیا کاربر جزو سه نفر اول است؟
        bool isTop3 = (i < 3);

        // انتخاب رنگ یا استایل چاپ
        if (isTop3)
        {
            // رنگ، فونت یا حالت خاص برای نفرات برتر
            // در ncurses می‌توان از pairX استفاده کرد
            attron(COLOR_PAIR(1) | A_BOLD);
            // همچنین می توانید القاب ویژه اضافه کنید
            const char *nickName;
            switch (i)
            {
            case 0:
                nickName = "Legend";
                break;
            case 1:
                nickName = "Goat";
                break;
            case 2:
                nickName = "Master";
                break;
            }
            // چاپ ردیف
            mvprintw(5 + i, 0, "%2d   | %-15s (%s) | %5d       | %5d    | %5d            | %4d",
                     rank, users[i].username, nickName, users[i].totalScore,
                     users[i].totalGold, users[i].gamesPlayed, diffDays);
            attroff(COLOR_PAIR(1) | A_BOLD);
        }
        else if (isCurrentUser)
        {
            // اگر همین کاربر است؛ متفاوت چاپ شود
            attron(A_BOLD | A_UNDERLINE);
            mvprintw(5 + i, 0, "%2d   | %-15s | %5d       | %5d    | %5d            | %4d",
                     rank, users[i].username, users[i].totalScore,
                     users[i].totalGold, users[i].gamesPlayed, diffDays);
            attroff(A_BOLD | A_UNDERLINE);
        }
        else
        {
            // چاپ پیش‌فرض
            mvprintw(5 + i, 0, "%2d   | %-15s | %5d       | %5d    | %5d            | %4d",
                     rank, users[i].username, users[i].totalScore,
                     users[i].totalGold, users[i].gamesPlayed, diffDays);
        }
    }

    refresh();
    getch(); // در صورت نیاز
    // بازگشت یا پاک کردن صفحه
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

// ذخیره کاربران در فایل
void saveUsersToFile()
{
    FILE *fp = fopen("users.txt", "w");
    if (fp == NULL)
    {
        return;
    }
    for (int i = 0; i < usersCount; i++)
    {
        fprintf(fp, "%s %s %ld %d %d %d\n", users[i].username, users[i].password, users[i].firstGameTime, users[i].totalScore, users[i].totalGold, users[i].gamesPlayed);
    }
    fclose(fp);
}

// به‌روزرسانی اطلاعات کاربر
void updateUserData(User *user, int gainedScore, int gainedGold)
{
    user->totalScore += gainedScore;
    user->totalGold += gainedGold;
    user->gamesPlayed++;
    if (user->firstGameTime == 0)
    {
        user->firstGameTime = time(NULL);
    }
    saveUsersToFile();
}

// مقایسه کاربران بر اساس مجموع امتیاز
int compareUsers(const void *a, const void *b)
{
    User *userA = (User *)a;
    User *userB = (User *)b;
    return userB->totalScore - userA->totalScore;
}