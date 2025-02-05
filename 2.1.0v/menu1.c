#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

void runGame()
{
    // Run the compilation and execution command
    system("gcc game.c -o game.out -lncurses -lm && ./game.out");
}

#define MAX_LEN 50
#define USERS_FILE "users.txt"

// تابع کمکی برای چاپ متن در وسط صفحه
void center_print(int row, const char *text)
{
    int col = (COLS - strlen(text)) / 2;
    mvprintw(row, col, "%s", text);
}

// تابع بررسی وجود کاربر در فایل (برای ثبت نام)
int userExists(const char *username)
{
    FILE *fp = fopen(USERS_FILE, "r");
    if (fp == NULL)
    {
        // اگر فایل وجود ندارد، هیچ کاربری ثبت نشده است.
        return 0;
    }
    char fileUsername[MAX_LEN], filePassword[MAX_LEN], fileEmail[MAX_LEN];
    while (fscanf(fp, "%s %s %s", fileUsername, filePassword, fileEmail) == 3)
    {
        if (strcmp(fileUsername, username) == 0)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// تابع بررسی صحت ورود (بررسی نام کاربری و رمز عبور)
int validateUserLogin(const char *username, const char *password)
{
    FILE *fp = fopen(USERS_FILE, "r");
    if (fp == NULL)
    {
        return 0;
    }
    char fileUsername[MAX_LEN], filePassword[MAX_LEN], fileEmail[MAX_LEN];
    while (fscanf(fp, "%s %s %s", fileUsername, filePassword, fileEmail) == 3)
    {
        if (strcmp(fileUsername, username) == 0 && strcmp(filePassword, password) == 0)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// تابع اعتبارسنجی رمز عبور (حداقل 7 کاراکتر شامل 1 رقم، 1 حرف بزرگ و 1 حرف کوچک)
int validatePassword(const char *password)
{
    int len = strlen(password);
    if (len < 7)
    {
        return 0;
    }
    int hasDigit = 0, hasUpper = 0, hasLower = 0;
    for (int i = 0; i < len; i++)
    {
        if (isdigit(password[i]))
            hasDigit = 1;
        else if (isupper(password[i]))
            hasUpper = 1;
        else if (islower(password[i]))
            hasLower = 1;
    }
    return (hasDigit && hasUpper && hasLower);
}

// تابع اعتبارسنجی ایمیل (چک کردن وجود "@" و ".")
int validateEmail(const char *email)
{
    char *atPtr = strchr(email, '@');
    if (atPtr == NULL || atPtr == email || *(atPtr + 1) == '\0')
    {
        return 0;
    }
    if (strchr(email, '.') == NULL)
    {
        return 0;
    }
    return 1;
}

// ذخیره اطلاعات کاربر در فایل
void saveUser(const char *username, const char *password, const char *email)
{
    FILE *fp = fopen(USERS_FILE, "a");
    if (fp == NULL)
    {
        center_print(20, "Error: Nemishe etela'at ra zakhire kard!");
        return;
    }
    fprintf(fp, "%s %s %s\n", username, password, email);
    fclose(fp);
}

// تولید رمز عبور تصادفی با رعایت شرایط
void generateRandomPassword(char *password, int length)
{
    if (length < 7)
        length = 7; // حداقل 7 کاراکتر
    const char *upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char *lower = "abcdefghijklmnopqrstuvwxyz";
    const char *digits = "0123456789";
    const char *allChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    // اطمینان از وجود حداقلی 1 حرف بزرگ، 1 حرف کوچک و 1 عدد
    password[0] = upper[rand() % strlen(upper)];
    password[1] = lower[rand() % strlen(lower)];
    password[2] = digits[rand() % strlen(digits)];

    // پر کردن بقیه کاراکترها به صورت تصادفی
    for (int i = 3; i < length; i++)
    {
        password[i] = allChars[rand() % strlen(allChars)];
    }
    password[length] = '\0';

    // شنافر کردن (Shuffle) کاراکترها
    for (int i = 0; i < length; i++)
    {
        int j = rand() % length;
        char temp = password[i];
        password[i] = password[j];
        password[j] = temp;
    }
}

// تابع نمایش منوی اصلی
int showMainMenu()
{
    clear();
    attron(A_BOLD | COLOR_PAIR(3));
    center_print(2, "MENU-ye BARNAME");
    attroff(A_BOLD | COLOR_PAIR(3));

    center_print(4, "1) Sakht Karbar Jadid");
    center_print(5, "2) Vared shodan be onvan-e Guest");
    center_print(6, "3) Vared shodan be hesab karbari");
    center_print(7, "Q) Khorooj");
    refresh();

    int ch = getch();
    return ch;
}

// -------------------------------------------------
// Exit handler: This function will be executed when the program ends.
// It compiles the code (assuming the source file is named "game5.c")
// and then runs the newly compiled executable.
void compileAndRun(void)
{
    // End ncurses mode in case it's still active
    endwin();
    // The following system() call compiles and runs the program.
    // NOTE: This will cause an infinite loop if not used carefully.
    system("gcc game5.c -o game5.out -lncurses && ./game5.out");
}

int main()
{
    srand(time(NULL));

    // Register the exit handler so that when main returns, compileAndRun() is called.
    atexit(compileAndRun);

    // راه‌اندازی ncurses
    initscr();
    noecho();
    cbreak();
    curs_set(1);

    // شروع پشتیبانی رنگی
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);  // پیام موفقیت
    init_pair(2, COLOR_RED, COLOR_BLACK);    // پیام خطا
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // عنوان منو

    int choice = showMainMenu();

    if (choice == 'Q' || choice == 'q')
    {
        endwin(); // پایان ncurses
        return 0;
    }

    // گزینه ورود به عنوان مهمان
    if (choice == '2')
    {
        clear();
        attron(COLOR_PAIR(1) | A_BOLD);
        center_print(10, "Shoma be onvan-e Guest vared shodid!");
        attroff(COLOR_PAIR(1) | A_BOLD);
        center_print(12, "Ba zadan har kelidi barname khatam mishavad...");
        refresh();
        getch();
        endwin();
        return 0;
    }

    // گزینه ورود به حساب کاربری
    if (choice == '3')
    {
        char username[MAX_LEN], password[MAX_LEN];
        int col = (COLS - MAX_LEN) / 2;
        clear();

        attron(COLOR_PAIR(3) | A_BOLD);
        center_print(2, "Vared shodan be hesab karbari");
        attroff(COLOR_PAIR(3) | A_BOLD);
        refresh();

        // دریافت نام کاربری
        center_print(4, "Nam Karbari: ");
        echo();
        mvgetnstr(5, col, username, MAX_LEN - 1);
        noecho();
        refresh();

        // دریافت رمز عبور
        center_print(7, "Ramz oboor: ");
        echo();
        mvgetnstr(8, col, password, MAX_LEN - 1);
        noecho();
        refresh();

        // چک کردن صحت ورود کاربر
        if (validateUserLogin(username, password))
        {
            attron(COLOR_PAIR(1));
            center_print(10, "Vared shodid! Khosh amadid be hesab karbari.");
            attroff(COLOR_PAIR(1));
        }
        else
        {
            attron(COLOR_PAIR(2));
            center_print(10, "Nam karbari ya ramz oboor eshtebah ast!");
            attroff(COLOR_PAIR(2));
        }

        center_print(12, "Ba zadan har kelidi barname khatam mishavad...");
        refresh();
        getch();
        endwin();
        runGame();
        return 0;
    }

    // در غیر این صورت، گزینه ساخت کاربر جدید (choice == '1')
    if (choice == '1')
    {
        char username[MAX_LEN], password[MAX_LEN], email[MAX_LEN];
        int col = (COLS - MAX_LEN) / 2;
        clear();

        // عنوان صفحه ساخت کاربر جدید
        attron(COLOR_PAIR(3) | A_BOLD);
        center_print(2, "Sakht Karbar Jadid");
        attroff(COLOR_PAIR(3) | A_BOLD);
        refresh();

        // دریافت نام کاربری
        center_print(4, "Nam Karbari: ");
        echo();
        mvgetnstr(5, col, username, MAX_LEN - 1);
        noecho();
        refresh();

        // بررسی تکراری نبودن نام کاربری
        if (userExists(username))
        {
            attron(COLOR_PAIR(2));
            center_print(7, "Karbari ba in nam ghablan sabt shode ast!");
            attroff(COLOR_PAIR(2));
            center_print(9, "Ba zadan har kelidi, barname baste khahad shod...");
            refresh();
            getch();
            endwin();
            return 0;
        }

        // انتخاب نوع ورود برای رمز عبور: تولید تصادفی یا وارد کردن دستی
        center_print(7, "Baraye tolid ramz oboor tasadofi, kelid 'r' ra feshar dahid.");
        center_print(8, "Dar gheir in soorat, kelid digari ra feshar dahid ta ramz ra dasti vared konid.");
        refresh();

        int ch = getch();
        if (ch == 'r' || ch == 'R')
        {
            generateRandomPassword(password, 8);
            attron(COLOR_PAIR(1));
            char tempMsg[100];
            snprintf(tempMsg, sizeof(tempMsg), "Ramz oboor tasadofi tolid shod: %s", password);
            center_print(10, tempMsg);
            attroff(COLOR_PAIR(1));
        }
        else
        {
            center_print(10, "Ramz oboor (hadaghal 7 karakter, shamel 1 harf bozorg, 1 harf koochak va 1 adad): ");
            echo();
            mvgetnstr(11, col, password, MAX_LEN - 1);
            noecho();
        }
        refresh();

        // اعتبارسنجی رمز عبور در صورت ورودی دستی
        if (!(ch == 'r' || ch == 'R'))
        {
            if (!validatePassword(password))
            {
                attron(COLOR_PAIR(2));
                center_print(13, "Ramz oboor naamotabar ast. Sharayet: 7 karakter, 1 harf bozorg, 1 harf koochak va 1 adad.");
                attroff(COLOR_PAIR(2));
                refresh();
                center_print(15, "Ba zadan har kelidi, barname baste mishavad...");
                getch();
                endwin();
                return 0;
            }
        }

        // دریافت ایمیل
        center_print(13, "Email (be format zzz.y@xxx): ");
        echo();
        mvgetnstr(14, col, email, MAX_LEN - 1);
        noecho();
        refresh();

        // اعتبارسنجی ایمیل
        if (!validateEmail(email))
        {
            attron(COLOR_PAIR(2));
            center_print(16, "Format email vared shode sahih nist!");
            attroff(COLOR_PAIR(2));
            refresh();
            center_print(18, "Ba zadan har kelidi, barname baste mishavad...");
            getch();
            endwin();
            return 0;
        }

        // ذخیره اطلاعات کاربر جدید
        saveUser(username, password, email);
        attron(COLOR_PAIR(1));
        center_print(16, "Karbar jadid ba movafaghiat ijad shod!");
        attroff(COLOR_PAIR(1));
        refresh();

        center_print(18, "Ba zadan har kelidi barname baste mishavad...");
        getch();
    }

    endwin();
    return 0;
}
