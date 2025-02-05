/*
   Sample Dungeon Game with Enemies
   این برنامه یک نقشه dungeon تولید می‌کند و دشمنان را نیز مدیریت می‌کند.
   از کتابخانه ncurses برای نمایش رابط ترمینال استفاده می‌شود.
*/

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

// تعریف ابعاد نقشه
#define MAP_WIDTH 80
#define MAP_HEIGHT 24

// --------------------------
// ساختار اتاق و دشمن
// --------------------------

// ساختار اتاق
typedef struct {
    int x, y;             // مختصات گوشه سمت چپ اتاق
    int width, height;    // ابعاد اتاق
    int centerX, centerY; // مرکز اتاق
    int doorX, doorY;     // مختصات درب (برای اتصال به راهرو)
    bool created;         // آیا اتاق ایجاد شده است؟
} Room;

// --------------------------
// تعریف انواع دشمن و ساختار دشمن
// --------------------------

// تعریف نوع‌های مختلف دشمنان
typedef enum {
    DEMON,
    FIRE_BREATHER,
    GIANT,
    SNAKE,
    UNDEAD
} EnemyType;

// ساختار دشمن
typedef struct {
    EnemyType type;         // نوع دشمن
    int health;             // مقدار سلامتی (برای شکست دادن دشمن)
    int damage;             // میزان آسیبی که هنگام برخورد وارد می‌کند
    int x, y;               // مختصات دشمن در نقشه
    int chase_steps;        // تعداد گام‌های مجاز برای دنبال کردن (برای دشمنانی با محدودیت مانند غول یا خیراب)
    bool active;            // وضعیت فعال (در صورت حضور در همان اتاق بازیکن)
} Enemy;

#define MAX_ENEMIES 20
Enemy enemies[MAX_ENEMIES];
int enemy_count = 0;

// --------------------------
// تعریف متغیرهای نقشه و اتاق ها
// --------------------------
char map[MAP_HEIGHT][MAP_WIDTH];
#define MIN_ROOMS 6
#define MAX_ROOMS 10
Room rooms[MAX_ROOMS];
int room_count = 0;

// --------------------------
// توابع مربوط به نقشه و اتاق ها
// --------------------------

// تابع مقداردهی اولیه نقشه با فاصله خالی
void init_map(char map[MAP_HEIGHT][MAP_WIDTH]) {
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            map[i][j] = ' ';
        }
    }
}

// نمایش نقشه با استفاده از ncurses
void display_map(char map[MAP_HEIGHT][MAP_WIDTH]) {
    clear();
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            mvaddch(i, j, map[i][j]);
        }
    }
    refresh();
}

// رسم اتاق روی نقشه شامل دیوارها، کف و درب تصادفی
void draw_room(char map[MAP_HEIGHT][MAP_WIDTH], Room room) {
    // رسم کف اتاق
    for (int i = room.y + 1; i < room.y + room.height - 1; i++) {
        for (int j = room.x + 1; j < room.x + room.width - 1; j++) {
            map[i][j] = '.';
        }
    }
    
    // رسم دیوارهای عمودی
    for (int i = room.y; i < room.y + room.height; i++) {
        map[i][room.x] = '|';
        map[i][room.x + room.width - 1] = '|';
    }
    
    // رسم دیوارهای افقی
    for (int j = room.x; j < room.x + room.width; j++) {
        map[room.y][j] = '_';
        map[room.y + room.height - 1][j] = '_';
    }
    
    // انتخاب تصادفی یک نقطه برای درب در یکی از دیوارها
    int door_side = rand() % 4;
    switch (door_side) {
        case 0: // بالایی
            room.doorX = room.x + (rand() % (room.width - 2)) + 1;
            room.doorY = room.y;
            break;
        case 1: // پایینی
            room.doorX = room.x + (rand() % (room.width - 2)) + 1;
            room.doorY = room.y + room.height - 1;
            break;
        case 2: // چپ
            room.doorX = room.x;
            room.doorY = room.y + (rand() % (room.height - 2)) + 1;
            break;
        case 3: // راست
            room.doorX = room.x + room.width - 1;
            room.doorY = room.y + (rand() % (room.height - 2)) + 1;
            break;
    }
    
    map[room.doorY][room.doorX] = '+';
}

// رسم راهرو بین دو نقطه (ابتدا افقی سپس عمودی)
void draw_corridor(char map[MAP_HEIGHT][MAP_WIDTH], int x1, int y1, int x2, int y2) {
    // رسم مسیر افقی
    int start = (x1 < x2) ? x1 : x2;
    int end = (x1 < x2) ? x2 : x1;
    for (int x = start; x <= end; x++) {
        if (map[y1][x] == ' ') {
            map[y1][x] = '#';
        }
    }
    
    // رسم مسیر عمودی
    start = (y1 < y2) ? y1 : y2;
    end = (y1 < y2) ? y2 : y1;
    for (int y = start; y <= end; y++) {
        if (map[y][x2] == ' ') {
            map[y][x2] = '#';
        }
    }
}

// تابع تولید اتاق‌های تصادفی در نقشه
int generate_rooms(Room rooms[], int max_rooms, char map[MAP_HEIGHT][MAP_WIDTH]) {
    int count = 0;
    int max_tries = 100;
    
    while (count < max_rooms && max_tries--) {
        Room room;
        room.width = 4 + rand() % 7;   // عرض از 4 تا 10
        room.height = 4 + rand() % 5;   // ارتفاع از 4 تا 8
        room.x = rand() % (MAP_WIDTH - room.width - 1);
        room.y = rand() % (MAP_HEIGHT - room.height - 1);
        room.centerX = room.x + room.width / 2;
        room.centerY = room.y + room.height / 2;
        room.created = true;
        
        bool overlap = false;
        for (int i = 0; i < count; i++) {
            Room other = rooms[i];
            if ((room.x < other.x + other.width + 1) && (room.x + room.width + 1 > other.x) &&
                (room.y < other.y + other.height + 1) && (room.y + room.height + 1 > other.y)) {
                overlap = true;
                break;
            }
        }
        if (!overlap) {
            rooms[count++] = room;
            draw_room(map, room);
        }
    }
    
    return count;
}

// تابع برای رفع مشکلات نقشه مانند درب‌های اشتباه
void fix(char map[MAP_HEIGHT][MAP_WIDTH]) {
    // مثال: اصلاح کردن درب‌هایی که به درستی متصل نیستند
    // در اینجا می‌توانید منطق دلخواه را اضافه کنید.
}

// --------------------------
// توابع مربوط به دشمنان
// --------------------------

// تابع افزودن دشمن به آرایه درون یک اتاق
void spawn_enemy_in_room(Room room) {
    if (enemy_count >= MAX_ENEMIES)
        return;
    
    int rand_type = rand() % 5;
    EnemyType type = (EnemyType)rand_type;
    
    Enemy new_enemy;
    new_enemy.type = type;
    
    // مقداردهی اولیه دشمن بر اساس نوع
    switch (type) {
        case DEMON:
            new_enemy.health = 5;
            new_enemy.damage = 1;
            new_enemy.chase_steps = 0; // دنبال کردن بدون محدودیت نیست؛ پس در حالت اولیه ممکن است ثابت شود
            break;
        case FIRE_BREATHER:
            new_enemy.health = 10;
            new_enemy.damage = 2;
            new_enemy.chase_steps = 0;
            break;
        case GIANT:
            new_enemy.health = 15;
            new_enemy.damage = 3;
            new_enemy.chase_steps = 5; // دنبال کردن تا 5 گام
            break;
        case SNAKE:
            new_enemy.health = 20;
            new_enemy.damage = 2;
            new_enemy.chase_steps = -1; // -1 برای دنبال کردن پیوسته
            break;
        case UNDEAD:
            new_enemy.health = 30;
            new_enemy.damage = 4;
            new_enemy.chase_steps = 5;
            break;
    }
    
    // قرار دادن دشمن در نقطه تصادفی داخل اتاق (با احتساب حاشیه)
    new_enemy.x = room.x + 1 + (rand() % (room.width - 2));
    new_enemy.y = room.y + 1 + (rand() % (room.height - 2));
    new_enemy.active = false;
    
    enemies[enemy_count++] = new_enemy;
}

// تابع به‌روزرسانی موقعیت دشمنان نسبت به موقعیت بازیکن در اتاق جاری
void update_enemy_positions(int playerX, int playerY, Room current_room) {
    for (int i = 0; i < enemy_count; i++) {
        Enemy *enemy = &enemies[i];
        
        // بررسی حضور دشمن در اتاق جاری
        if (enemy->x >= current_room.x && enemy->x < current_room.x + current_room.width &&
            enemy->y >= current_room.y && enemy->y < current_room.y + current_room.height) {
            enemy->active = true;
        } else {
            enemy->active = false;
            continue;
        }
        
        // برای دشمنانی با محدودیت گام (مثلاً GIANT و UNDEAD)
        if ((enemy->type == GIANT || enemy->type == UNDEAD) && enemy->chase_steps == 0) {
            continue;
        }
        
        // الگوریتم ساده: حرکت یک قدم به سمت بازیکن
        if (playerX > enemy->x)
            enemy->x++;
        else if (playerX < enemy->x)
            enemy->x--;
        
        if (playerY > enemy->y)
            enemy->y++;
        else if (playerY < enemy->y)
            enemy->y--;
        
        // در صورت محدودیت گام، کاهش گام باقی‌مانده
        if (enemy->chase_steps > 0)
            enemy->chase_steps--;
    }
}

// بررسی برخورد دشمنان با بازیکن، اعمال خسارت و نمایش پیام
void check_enemy_collisions(int playerX, int playerY, int *playerHealth) {
    for (int i = 0; i < enemy_count; i++) {
        Enemy *enemy = &enemies[i];
        if (enemy->active && enemy->x == playerX && enemy->y == playerY) {
            // نمایش پیام برخورد با دشمن
            switch (enemy->type) {
                case DEMON:
                    mvprintw(0, 0, "یک دِمون به شما حمله می‌کند! آسیب جزئی دریافت کردید.");
                    break;
                case FIRE_BREATHER:
                    mvprintw(0, 0, "هیوندای آتش‌زده شما را می‌سوزاند! آسیب متوسط دریافت کردید.");
                    break;
                case GIANT:
                    mvprintw(0, 0, "غول شما را می‌کوبد! آسیب شدید وارد شد.");
                    break;
                case SNAKE:
                    mvprintw(0, 0, "یک مار به شما گاز می‌گیرد! آسیب متوسط دریافت می‌کنید.");
                    break;
                case UNDEAD:
                    mvprintw(0, 0, "خیراب به شما چنگ می‌زند! آسیب قابل توجه دریافت کردید.");
                    break;
            }
            
            // کاهش سلامتی بازیکن
            *playerHealth -= enemy->damage;
        }
    }
}

// نمایش دشمنان روی نقشه
void display_enemies(char map[MAP_HEIGHT][MAP_WIDTH]) {
    for (int i = 0; i < enemy_count; i++) {
        if (enemies[i].active) {
            char enemy_char;
            switch (enemies[i].type) {
                case DEMON: enemy_char = 'D'; break;
                case FIRE_BREATHER: enemy_char = 'F'; break;
                case GIANT: enemy_char = 'G'; break;
                case SNAKE: enemy_char = 'S'; break;
                case UNDEAD: enemy_char = 'U'; break;
            }
            mvaddch(enemies[i].y, enemies[i].x, enemy_char);
        }
    }
}

// --------------------------
// تابع اصلی
// --------------------------
int main() {
    srand(time(NULL));
    
    // مقداردهی اولیه ncurses
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    // مقداردهی اولیه نقشه
    init_map(map);
    
    // تولید اتاق‌ها
    room_count = generate_rooms(rooms, MAX_ROOMS, map);
    
    // اتصال اتاق‌ها با راهرو
    for (int i = 1; i < room_count; i++) {
        // اتصال مرکز اتاق قبلی به مرکز اتاق فعلی
        draw_corridor(map, rooms[i-1].centerX, rooms[i-1].centerY, rooms[i].centerX, rooms[i-1].centerY);
        draw_corridor(map, rooms[i].centerX, rooms[i-1].centerY, rooms[i].centerX, rooms[i].centerY);
    }
    
    // اصلاح نقشه (در صورت نیاز)
    fix(map);
    
    // افزودن دشمنان به برخی اتاق‌ها (مثلاً با احتمال 50%)
    for (int i = 0; i < room_count; i++) {
        if (rand() % 100 < 50) {
            spawn_enemy_in_room(rooms[i]);
        }
    }
    
    // سلامتی اولیه بازیکن
    int playerHealth = 100;
    // قرارگیری اولیه بازیکن در مرکز اولین اتاق
    int playerX = rooms[0].centerX;
    int playerY = rooms[0].centerY;
    
    // نمایش اولیه نقشه
    display_map(map);
    mvaddch(playerY, playerX, '@'); // نماد بازیکن
    display_enemies(map);
    mvprintw(MAP_HEIGHT - 1, 0, "سلامتی: %d", playerHealth);
    refresh();
    
    // حلقه اصلی بازی
    int ch;
    while ((ch = getch()) != 'q' && playerHealth > 0) {
        // پاکسازی بازیکن از موقعیت قبلی
        mvaddch(playerY, playerX, map[playerY][playerX]);
        
        // خواندن ورودی جهت حرکت
        switch(ch) {
            case KEY_UP:
            case 'w': playerY--; break;
            case KEY_DOWN:
            case 's': playerY++; break;
            case KEY_LEFT:
            case 'a': playerX--; break;
            case KEY_RIGHT:
            case 'd': playerX++; break;
            default: break;
        }
        
        // به‌روزرسانی موقعیت دشمنان در اتاق فعلی (اتاق فعلی را بسته به مختصات بازیکن تعیین می‌کنیم)
        // در این مثال فرض می‌کنیم بازیکن همیشه داخل یک اتاق است؛ در صورت نیاز می‌توانید منطق پیچیده‌تری اضافه کنید.
        for (int i = 0; i < room_count; i++) {
            if (playerX >= rooms[i].x && playerX < rooms[i].x + rooms[i].width &&
                playerY >= rooms[i].y && playerY < rooms[i].y + rooms[i].height) {
                update_enemy_positions(playerX, playerY, rooms[i]);
                break;
            }
        }
        
        // بررسی برخورد دشمن با بازیکن
        check_enemy_collisions(playerX, playerY, &playerHealth);
        
        // نمایش نقشه، دشمنان و بازیکن
        display_map(map);
        display_enemies(map);
        mvaddch(playerY, playerX, '@');
        mvprintw(MAP_HEIGHT - 1, 0, "سلامتی: %d", playerHealth);
        refresh();
    }
    
    // پایان برنامه
    endwin();
    return 0;
}
