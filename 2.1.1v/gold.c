#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define MAP_WIDTH 80
#define MAP_HEIGHT 24

// تعداد حداقل اتاق‌ها
#define MIN_ROOMS 6
// تعداد امتحانی حداکثر اتاق‌ها (برای پرهیز از بی‌نظمی)
#define MAX_ROOMS 10

// تعریف انواع عناصر نقشه
#define WALL_VERTICAL '|'
#define WALL_HORIZONTAL '_'
#define FLOOR '.'
#define DOOR '+'
#define CORRIDOR '#'
#define PILLAR 'O'
#define WINDOW '='

// تعریف آی‌کن‌ها و مقادیر طلا
#define GOLD_ICON '$'
#define BLACK_GOLD_ICON '$'
#define GOLD_VALUE 10
#define BLACK_GOLD_VALUE 50

// ساختار اتاق
typedef struct
{
    int x, y;             // مختصات گوشه بالا چپ اتاق
    int width, height;    // ابعاد اتاق
    int centerX, centerY; // مرکز اتاق
    int doorX, doorY;     // مختصات در (برای اتصال به راهرو)
    bool created;         // آیا اتاق ساخته شده؟
} Room;

// متغیرهای جهانی برای امتیاز و طلا
int playerGold = 0;

// تابعی برای بررسی همپوشانی دو اتاق
bool is_overlap(Room *a, Room *b)
{
    if (a->x + a->width < b->x || b->x + b->width < a->x)
        return false;
    if (a->y + a->height < b->y || b->y + b->height < a->y)
        return false;
    return true;
}

// تابع اولیه نقشه (پر کردن آرایه 2بعدی با فضای خالی)
void init_map(char map[MAP_HEIGHT][MAP_WIDTH])
{
    for (int i = 0; i < MAP_HEIGHT; i++)
    {
        for (int j = 0; j < MAP_WIDTH; j++)
        {
            map[i][j] = ' ';
        }
    }
}

// تابع نمایش نقشه با استفاده از ncurses
void display_map(char map[MAP_HEIGHT][MAP_WIDTH])
{
    for (int i = 0; i < MAP_HEIGHT; i++)
    {
        for (int j = 0; j < MAP_WIDTH; j++)
        {
            char cell = map[i][j];
            if (cell == GOLD_ICON) {
                attron(COLOR_PAIR(1));  // رنگ زرد برای طلای عادی
                mvaddch(i, j, cell);
                attroff(COLOR_PAIR(1));
            } else if (cell == BLACK_GOLD_ICON) {
                attron(COLOR_PAIR(2));  // رنگ قرمز برای طلای سیاه
                mvaddch(i, j, cell);
                attroff(COLOR_PAIR(2));
            } else {
                mvaddch(i, j, cell);
            }
        }
    }
    refresh();
}

// تابع رسم اتاق در نقشه
void draw_room(char map[MAP_HEIGHT][MAP_WIDTH], Room room)
{
    // رسم کف اتاق
    for (int i = room.y + 1; i < room.y + room.height - 1; i++)
    {
        for (int j = room.x + 1; j < room.x + room.width - 1; j++)
        {
            map[i][j] = FLOOR;
        }
    }

    // رسم دیوارهای افقی (بالا و پایین)
    for (int j = room.x; j < room.x + room.width; j++)
    {
        map[room.y][j] = WALL_HORIZONTAL;                   // بالای اتاق
        map[room.y + room.height - 1][j] = WALL_HORIZONTAL; // پایین اتاق
    }

    // رسم دیوارهای عمودی (سمت چپ و راست)
    for (int i = room.y; i < room.y + room.height; i++)
    {
        map[i][room.x] = WALL_VERTICAL;                  // چپ اتاق
        map[i][room.x + room.width - 1] = WALL_VERTICAL; // راست اتاق
    }

    // قرار دادن در به صورت تصادفی روی یکی از دیوارها
    int door_side = rand() % 4;
    int door_pos;
    switch (door_side)
    {
    case 0: // دیوار بالا
        door_pos = room.x + 1 + rand() % (room.width - 2);
        map[room.y][door_pos] = DOOR;
        break;
    case 1: // دیوار پایین
        door_pos = room.x + 1 + rand() % (room.width - 2);
        map[room.y + room.height - 1][door_pos] = DOOR;
        break;
    case 2: // دیوار چپ
        door_pos = room.y + 1 + rand() % (room.height - 2);
        map[door_pos][room.x] = DOOR;
        break;
    case 3: // دیوار راست
        door_pos = room.y + 1 + rand() % (room.height - 2);
        map[door_pos][room.x + room.width - 1] = DOOR;
        break;
    }
}

// تابع رسم راهرو بین دو نقاط: ابتدا به صورت افقی و سپس عمودی
void draw_corridor(char map[MAP_HEIGHT][MAP_WIDTH], int x1, int y1, int x2, int y2)
{
    // حرکت افقی
    int startX = (x1 < x2) ? x1 : x2;
    int endX = (x1 < x2) ? x2 : x1;
    for (int x = startX; x <= endX; x++)
    {
        if (map[y1][x] == ' ' || map[y1][x] == DOOR)
            map[y1][x] = CORRIDOR;
    }
    // حرکت عمودی
    int startY = (y1 < y2) ? y1 : y2;
    int endY = (y1 < y2) ? y2 : y1;
    for (int y = startY; y <= endY; y++)
    {
        if (map[y][x2] == ' ' || map[y][x2] == DOOR)
            map[y][x2] = CORRIDOR;
    }
}

// تابع اصلی تولید اتاق‌ها به صورت تصادفی و بررسی همپوشانی
int generate_rooms(Room rooms[], int max_rooms, char map[MAP_HEIGHT][MAP_WIDTH])
{
    int room_count = 0;
    int attempts = 0;
    while (room_count < max_rooms && attempts < 200)
    {
        attempts++;
        int width = 4 + rand() % 7;
        int height = 4 + rand() % 5;
        int x = 1 + rand() % (MAP_WIDTH - width - 1);
        int y = 1 + rand() % (MAP_HEIGHT - height - 1);

        Room new_room = {x, y, width, height, 0, 0, 0, 0, true};
        new_room.centerX = x + width / 2;
        new_room.centerY = y + height / 2;

        bool overlaps = false;
        for (int i = 0; i < room_count; i++)
        {
            Room temp = rooms[i];
            if (x <= temp.x + temp.width + 1 &&
                x + width + 1 >= temp.x &&
                y <= temp.y + temp.height + 1 &&
                y + height + 1 >= temp.y)
            {
                overlaps = true;
                break;
            }
        }
        if (!overlaps)
        {
            rooms[room_count++] = new_room;
            // رسم اتاق روی نقشه
            draw_room(map, new_room);
        }
    }
    return room_count;
}

// تابع قرار دادن طلا روی نقشه (طلا عادی)
void placeRegularGold(char map[MAP_HEIGHT][MAP_WIDTH])
{
    int numGoldBags = 20;
    while (numGoldBags > 0)
    {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;
        // فقط اگر مکانی کف (FLOOR) داشته باشد
        if (map[y][x] == FLOOR)
        {
            map[y][x] = GOLD_ICON;
            numGoldBags--;
        }
    }
}

// تابع قرار دادن طلای سیاه روی نقشه
void placeBlackGold(char map[MAP_HEIGHT][MAP_WIDTH])
{
    int numBlackGoldBags = 5;
    while (numBlackGoldBags > 0)
    {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;
        // طلا روی کف اتاق قرار گیرد
        if (map[y][x] == FLOOR)
        {
            map[y][x] = BLACK_GOLD_ICON;
            numBlackGoldBags--;
        }
    }
}

// تابع اطلاع رسانی به بازیکن هنگام برداشت طلا
void show_gold_message(int collected)
{
    attron(A_BOLD);
    mvprintw(MAP_HEIGHT, 0, "شما %d طلا جمع کردید! ", collected);
    attroff(A_BOLD);
    refresh();
    // موقت تا بازیکن متوجه شود
    napms(700);
    // پاک کردن پیام
    move(MAP_HEIGHT, 0);
    clrtoeol();
    refresh();
}

// تابع جابه‌جایی و کنترل حرکت بازیکن و جمع‌آوری طلا
void move_player(int *playerX, int *playerY, int newX, int newY, char map[MAP_HEIGHT][MAP_WIDTH])
{
    // بررسی مرزهای نقشه و موانع (مثلا دیوارها)
    if (newX < 0 || newX >= MAP_WIDTH || newY < 0 || newY >= MAP_HEIGHT)
        return;
    char cell = map[newY][newX];
    // اگر روی دیوار یا راهرو نباشد، جلو نروید
    if (cell == WALL_HORIZONTAL || cell == WALL_VERTICAL)
        return;

    // اگر بر روی چیدن طلا باشد
    if (cell == GOLD_ICON || cell == BLACK_GOLD_ICON)
    {
        int collected = 0;
        if (cell == GOLD_ICON)
            collected = GOLD_VALUE;
        else if (cell == BLACK_GOLD_ICON)
            collected = BLACK_GOLD_VALUE;
        playerGold += collected;
        show_gold_message(collected);
        // حذف طلا از روی زمین (جایگذاری کف)
        map[newY][newX] = FLOOR;
    }
    // پاک کردن کاراکتر بازیکن از موقعیت قبلی
    mvaddch(*playerY, *playerX, FLOOR);
    // به‌روزرسانی موقعیت بازیکن
    *playerX = newX;
    *playerY = newY;
    // رسم بازیکن با نماد '@'
    mvaddch(*playerY, *playerX, '@');
    refresh();
}

// تابع نمایش امتیاز نهایی در پایان بازی
void display_final_score()
{
    clear();
    attron(A_BOLD);
    mvprintw(MAP_HEIGHT / 2 - 1, MAP_WIDTH / 2 - 10, "بازی به پایان رسید!");
    mvprintw(MAP_HEIGHT / 2, MAP_WIDTH / 2 - 10, "کل طلا: %d", playerGold);
    mvprintw(MAP_HEIGHT / 2 + 1, MAP_WIDTH / 2 - 10, "امتیاز شما: %d", playerGold);
    attroff(A_BOLD);
    refresh();
    getch();
}

int main()
{
    srand(time(NULL));

    // شروع حالت ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0); // پنهان کردن اشاره‌گر

    // تنظیم رنگ‌ها
    if (has_colors() == FALSE)
    {
        endwin();
        printf("سیستم شما از رنگ پشتیبانی نمی‌کند\n");
        exit(1);
    }
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK); // طلای عادی
    init_pair(2, COLOR_RED, COLOR_BLACK);    // طلای سیاه

    // ایجاد و مقداردهی اولیه نقشه
    char map[MAP_HEIGHT][MAP_WIDTH];
    init_map(map);

    // تولید و رسم اتاق‌ها
    Room rooms[MAX_ROOMS];
    int room_count = generate_rooms(rooms, MAX_ROOMS, map);

    // رسم راهروها بین اتاق‌های تولید شده (به صورت ساده)
    for (int i = 1; i < room_count; i++)
    {
        int x1 = rooms[i - 1].centerX;
        int y1 = rooms[i - 1].centerY;
        int x2 = rooms[i].centerX;
        int y2 = rooms[i].centerY;
        draw_corridor(map, x1, y1, x2, y2);
    }

    // قرار دادن طلا های عادی و سیاه روی نقشه
    placeRegularGold(map);
    placeBlackGold(map);

    // نمایش اولیه نقشه
    display_map(map);

    // تعیین موقعیت اولیه بازیکن (مثلا مرکز اولین اتاق)
    int playerX = rooms[0].centerX;
    int playerY = rooms[0].centerY;
    mvaddch(playerY, playerX, '@');
    refresh();

    // حلقه اصلی بازی؛ با فشردن کلید q بازی متوقف می‌شود.
    int ch;
    while ((ch = getch()) != 'q')
    {
        int newX = playerX;
        int newY = playerY;
        switch(ch)
        {
            case KEY_UP:
                newY--;
                break;
            case KEY_DOWN:
                newY++;
                break;
            case KEY_LEFT:
                newX--;
                break;
            case KEY_RIGHT:
                newX++;
                break;
            default:
                break;
        }
        move_player(&playerX, &playerY, newX, newY, map);
    }

    // پایان بازی، نمایش امتیاز
    display_final_score();

    // پایان حالت ncurses
    endwin();
    return 0;
}
