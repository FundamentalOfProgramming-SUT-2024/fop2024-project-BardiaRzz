#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define MAP_WIDTH 80
#define MAP_HEIGHT 24
#define MIN_ROOMS 6
#define MAX_ROOMS 10

#define WALL_VERTICAL '|'
#define WALL_HORIZONTAL '-'
#define FLOOR '.'
#define DOOR '+'
#define CORRIDOR '#'
#define PILLAR 'O'
#define WINDOW '='

#define GOLD_ICON '$'
#define BLACK_GOLD_ICON '%'
#define GOLD_VALUE 10
#define BLACK_GOLD_VALUE 50

int playerGold = 0;

//----------------------------------------------------------------------------
// ROOM STRUCTURE
//----------------------------------------------------------------------------
typedef struct
{
    int x, y;
    int width, height;
    int centerX, centerY;
    bool created;
    bool visited;
} Room;

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
int playerX; // col
int playerY; // row

//----------------------------------------------------------------------------
// HELPER FUNCTIONS
//----------------------------------------------------------------------------
bool is_overlap(Room *a, Room *b)
{
    // Returns true if rooms 'a' and 'b' overlap
    if (a->x + a->width < b->x || b->x + b->width < a->x)
        return false;
    if (a->y + a->height < b->y || b->y + b->height < a->y)
        return false;
    return true;
}

void init_map(char map[MAP_HEIGHT][MAP_WIDTH])
{
    // Fill entire map with space
    for (int row = 0; row < MAP_HEIGHT; row++)
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            map[row][col] = ' ';
        }
    }
}

void display_map(char map[MAP_HEIGHT][MAP_WIDTH])
{
    // Clear screen and display map
    clear();
    for (int row = 0; row < MAP_HEIGHT; row++)
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            mvaddch(row, col, map[row][col]);
        }
    }
    refresh();
}

bool isWalkable(char map[MAP_HEIGHT][MAP_WIDTH], int x, int y)
{
    // Check if a tile is walkable (floor, corridor, door, etc.)
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return false;

    char tile = map[y][x];
    return (tile == FLOOR || tile == CORRIDOR || tile == DOOR || tile == PILLAR || tile == GOLD_ICON || tile == BLACK_GOLD_ICON);
}

int roomIndexAt(int room_count, Room rooms[], int x, int y)
{
    // Return index of room if the given coordinate is inside it (including walls),
    // otherwise return -1
    for (int i = 0; i < room_count; i++)
    {
        Room r = rooms[i];
        if (x >= r.x && x < r.x + r.width &&
            y >= r.y && y < r.y + r.height)
        {
            return i;
        }
    }
    return -1;
}

//----------------------------------------------------------------------------
// MAP CREATION
//----------------------------------------------------------------------------
void draw_room(char map[MAP_HEIGHT][MAP_WIDTH], Room room)
{
    // Fill in the floor area
    for (int row = room.y + 1; row < room.y + room.height - 1; row++)
    {
        for (int col = room.x + 1; col < room.x + room.width - 1; col++)
        {
            map[row][col] = FLOOR;
        }
    }

    // Walls (horizontal)
    for (int col = room.x; col < room.x + room.width; col++)
    {
        map[room.y][col] = WALL_HORIZONTAL;
        map[room.y + room.height - 1][col] = WALL_HORIZONTAL;
    }

    // Walls (vertical)
    for (int row = room.y; row < room.y + room.height; row++)
    {
        map[row][room.x] = WALL_VERTICAL;
        map[row][room.x + room.width - 1] = WALL_VERTICAL;
    }
}

void draw_corridor(char map[MAP_HEIGHT][MAP_WIDTH], int x1, int y1, int x2, int y2)
{
    // Draw corridor horizontally
    int startX = (x1 < x2) ? x1 : x2;
    int endX = (x1 < x2) ? x2 : x1;

    for (int col = startX; col <= endX; col++)
    {
        if (map[y1][col] == ' ' || map[y1][col] == DOOR)
            map[y1][col] = CORRIDOR;
        if (map[y1][col] == '|' || map[y1][col] == '-')
            map[y1][col] = DOOR;
    }

    // Draw corridor vertically
    int startY = (y1 < y2) ? y1 : y2;
    int endY = (y1 < y2) ? y2 : y1;
    for (int row = startY; row <= endY; row++)
    {
        if (map[row][x2] == ' ' || map[row][x2] == DOOR)
            map[row][x2] = CORRIDOR;
    }
}

int generate_rooms(Room rooms[], int max_rooms, char map[MAP_HEIGHT][MAP_WIDTH])
{
    int room_count = 0;
    int attempts = 0;

    while (room_count < max_rooms && attempts < 200)
    {
        attempts++;
        int width = 4 + rand() % 7;  // possible range: 4..10
        int height = 4 + rand() % 5; // possible range: 4..8

        int x = 1 + rand() % (MAP_WIDTH - width - 1);
        int y = 1 + rand() % (MAP_HEIGHT - height - 1);

        Room new_room = {x, y, width, height, 0, 0, true, false};
        new_room.centerX = x + width / 2;
        new_room.centerY = y + height / 2;

        bool overlaps = false;
        for (int i = 0; i < room_count; i++)
        {
            Room temp = rooms[i];
            if (is_overlap(&new_room, &temp))
            {
                overlaps = true;
                break;
            }
        }
        if (!overlaps)
        {
            rooms[room_count++] = new_room;
        }
    }
    return room_count;
}

//----------------------------------------------------------------------------
// VISIBILITY
//----------------------------------------------------------------------------
void updateVisibility(Room rooms[], int room_count,
                      char dungeon[MAP_HEIGHT][MAP_WIDTH],
                      bool visible[MAP_HEIGHT][MAP_WIDTH])
{
    // Reset all cells to invisible
    for (int row = 0; row < MAP_HEIGHT; row++)
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            visible[row][col] = false;
        }
    }

    // Reveal all visited rooms
    for (int r = 0; r < room_count; r++)
    {
        if (rooms[r].visited)
        {
            for (int row = rooms[r].y; row < rooms[r].y + rooms[r].height; row++)
            {
                for (int col = rooms[r].x; col < rooms[r].x + rooms[r].width; col++)
                {
                    visible[row][col] = true;
                }
            }
        }
    }

    // Reveal corridors that border any already-visible cell
    for (int row = 0; row < MAP_HEIGHT; row++)
    {
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            if (dungeon[row][col] == CORRIDOR)
            {
                // If any neighbor is visible, reveal this corridor cell too
                for (int dy = -1; dy <= 1; dy++)
                {
                    for (int dx = -1; dx <= 1; dx++)
                    {
                        int ny = row + dy;
                        int nx = col + dx;
                        if (ny >= 0 && ny < MAP_HEIGHT &&
                            nx >= 0 && nx < MAP_WIDTH)
                        {
                            if (visible[ny][nx])
                            {
                                visible[row][col] = true;
                                goto nextCell;
                            }
                        }
                    }
                }
            }
        nextCell:;
        }
    }

    // Reveal cells around the player's current position in a 3x3 block
    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int adjX = playerX + dx;
            int adjY = playerY + dy;
            if (adjX >= 0 && adjX < MAP_WIDTH && adjY >= 0 && adjY < MAP_HEIGHT)
            {
                visible[adjY][adjX] = true;
            }
        }
    }
}

void printDungeon(char map[MAP_HEIGHT][MAP_WIDTH],
                  bool visible[MAP_HEIGHT][MAP_WIDTH],
                  Room rooms[], int room_count)
{
    // Clear screen and update visibility before printing
    clear();
    updateVisibility(rooms, room_count, map, visible);

    // Draw only the tiles that are marked visible
    for (int row = 0; row < MAP_HEIGHT; row++)
    {
        move(row, 0);
        for (int col = 0; col < MAP_WIDTH; col++)
        {
            if (visible[row][col])
                addch(map[row][col]);
            else
                addch(' ');
        }
    }

    // Print the player
    mvaddch(playerY, playerX, '@');
    refresh();
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
    mvprintw(MAP_HEIGHT, 0, "you have collected %d golds", collected);
    attroff(A_BOLD);
    refresh();
    // موقت تا بازیکن متوجه شود
    napms(700);
    // پاک کردن پیام
    move(MAP_HEIGHT, 0);
    clrtoeol();
    refresh();
}

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

//----------------------------------------------------------------------------
// MAIN
//----------------------------------------------------------------------------
int main()
{
    srand((unsigned)time(NULL));

    initscr();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);

    if (has_colors() == FALSE)
    {
        endwin();
        printf("سیستم شما از رنگ پشتیبانی نمی‌کند\n");
        exit(1);
    }
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK); // طلای عادی
    init_pair(2, COLOR_RED, COLOR_BLACK);    // طلای سیاه

    // Allocate dungeon map and visibility array
    char map[MAP_HEIGHT][MAP_WIDTH];
    bool visible[MAP_HEIGHT][MAP_WIDTH];
    init_map(map);

    // Reserve memory for our rooms and create them
    Room rooms[MAX_ROOMS];
    int room_count = generate_rooms(rooms, MAX_ROOMS, map);

    // Ensure we generate at least MIN_ROOMS for a decent dungeon
    if (room_count < MIN_ROOMS)
        room_count = MIN_ROOMS;

    // Draw each room
    for (int i = 0; i < room_count; i++)
    {
        draw_room(map, rooms[i]);
    }

    // Connect rooms with corridors
    for (int i = 1; i < room_count; i++)
    {
        int prev_centerX = rooms[i - 1].centerX;
        int prev_centerY = rooms[i - 1].centerY;
        int curr_centerX = rooms[i].centerX;
        int curr_centerY = rooms[i].centerY;
        draw_corridor(map, prev_centerX, prev_centerY, curr_centerX, curr_centerY);
    }

    // Place the player in the first room
    playerX = rooms[0].x + rooms[0].width / 2;
    playerY = rooms[0].y + rooms[0].height / 2;
    rooms[0].visited = true; // Mark first room as visited

    placeRegularGold(map);
    placeBlackGold(map);

    display_map(map);

    bool all_visible[MAP_HEIGHT][MAP_WIDTH];
    // make them all be true
    for (int i = 0; i < MAP_HEIGHT; i++)
    {
        for (int j = 0; j < MAP_WIDTH; j++)
        {
            all_visible[i][j] = true;
        }
    }

    // Input loop
    int ch;
    while ((ch = getch()) != 'q')
    {
        int newX = playerX;
        int newY = playerY;

        if (ch == KEY_UP)
            newY--;
        else if (ch == KEY_DOWN)
            newY++;
        else if (ch == KEY_LEFT)
            newX--;
        else if (ch == KEY_RIGHT)
            newX++;

        // Check if the new position is walkable
        if (isWalkable(map, newX, newY))
        {
            playerX = newX;
            playerY = newY;

            // Update visited status for rooms
            int rIndex = roomIndexAt(room_count, rooms, playerX, playerY);
            if (rIndex != -1)
                rooms[rIndex].visited = true;

            // Redraw dungeon with updated visibility
        }
        char cell = map[newY][newX];
        if (cell == GOLD_ICON || cell == BLACK_GOLD_ICON)
        {
            int collected = 0;
            if (cell == GOLD_ICON)
                collected = GOLD_VALUE;
            else if (cell == BLACK_GOLD_ICON)
                collected = BLACK_GOLD_VALUE;
            playerGold += collected;
            show_gold_message(playerGold);
            // حذف طلا از روی زمین (جایگذاری کف)
            map[newY][newX] = FLOOR;
        }

        printDungeon(map, visible, rooms, room_count);
    }

    display_final_score();

    // Cleanup ncurses
    endwin();
    return 0;
}
