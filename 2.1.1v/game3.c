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
#define MAX_FOOD_ITEMS 5

#define FOOD_ICON 'F'
#define COMMON_FOOD_HEALTH_RESTORE 10

#define MAX_WEAPON_ITEMS 10

#define WEAPON_ICON_MACE 'M'
#define WEAPON_ICON_DAGGER 'D'
#define WEAPON_ICON_WAND 'W'
#define WEAPON_ICON_ARROW 'A'
#define WEAPON_ICON_SWORD 'S'

typedef enum
{
    COMMON_FOOD,   // ghazaye mamooli
    SUPERIOR_FOOD, // ghazaye momtaz
    MAGIC_FOOD,    // ghazaye jadooyi
    CORRUPTED_FOOD // ghazaye fased shode
} FoodType;

typedef struct
{
    int x, y;          // mokhtasat rooye naghshe
    FoodType type;     // no'e ghaza
    int healthRestore; // mizan salamat ke ba masraf be dast miayad
} Food;

typedef struct
{
    char *name;
    char icon;  // Use char for simplicity
    int damage; // This field can be used later for combat logic
} Weapon;

Food foodInventory[MAX_FOOD_ITEMS];
int foodCount = 0; // shomarande mojoodi ghaza

Weapon weaponInventory[MAX_WEAPON_ITEMS];
int weaponCount = 0;

int playerGold = 0;

// dar jayi bala, biron az tabe main ya har tabe digar:
bool mapRevealed = false;

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

typedef struct
{
    int x, y;
    int health;
    int gold;
    int hunger; // az 0 (por) ta 100 (goshne)
} Player;

typedef enum
{
    ENEMY_DEMON,
    ENEMY_FIRE, // Monster Breathing Fire
    ENEMY_GIANT,
    ENEMY_SNAKE,
    ENEMY_UNDEAD
} EnemyType;

typedef struct
{
    int x, y;            // Enemy position on the map
    EnemyType type;      // Type of enemy
    int damageThreshold; // Damage needed to kill the enemy
    int chasingSteps;    // Steps remaining to chase the player (if applicable)
    int isActive;        // Active in current room
} Enemy;

#define MAX_ENEMIES 20 // Reduced the maximum number of enemies
Enemy enemies[MAX_ENEMIES];
int enemyCount = 0;

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
void showFoodMenu(Player *player)
{
    // namayesh navare goshnegi
    printw("sathe goshnegi: [");
    for (int i = 0; i < 10; i++)
    {
        if ((player->hunger / 10) > i)
            printw("#");
        else
            printw("-");
    }
    printw("]\n");

    // namayesh liste ghazahaye mojood
    printw("mojoodi ghaza:\n");
    for (int i = 0; i < foodCount; i++)
    {
        printw("%d: ", i + 1);
        switch (foodInventory[i].type)
        {
        case COMMON_FOOD:
            printw("ghazaye mamooli (bazgardani salamat: %d)\n", foodInventory[i].healthRestore);
            break;
        case SUPERIOR_FOOD:
            printw("ghazaye momtaz (bazgardani salamat: %d)\n", foodInventory[i].healthRestore);
            break;
        case MAGIC_FOOD:
            printw("ghazaye jadooyi (bazgardani salamat: %d)\n", foodInventory[i].healthRestore);
            break;
        case CORRUPTED_FOOD:
            printw("ghazaye fased shode (bazgardani salamat: %d, ehtiat!)\n", foodInventory[i].healthRestore);
            break;
        }
    }
    printw("baraye masraf ghaza, shomare marboote ra vared konid, ya har kelide digar baraye khorooj.\n");

    // daryaft voroodi karbar (masalan ba getch) va pardazesh entekhab
    int choice = getch() - '1';
    if (choice >= 0 && choice < foodCount)
    {
        consumeFood(player, choice);
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

    // Print the enemies
    for (int i = 0; i < enemyCount; i++)
    {
        if (enemies[i].isActive && visible[enemies[i].y][enemies[i].x])
        {
            char enemyIcon;
            switch (enemies[i].type)
            {
            case ENEMY_DEMON:
                enemyIcon = 'D';
                break;
            case ENEMY_FIRE:
                enemyIcon = 'F';
                break;
            case ENEMY_GIANT:
                enemyIcon = 'G';
                break;
            case ENEMY_SNAKE:
                enemyIcon = 'S';
                break;
            case ENEMY_UNDEAD:
                enemyIcon = 'U';
                break;
            }
            mvaddch(enemies[i].y, enemies[i].x, enemyIcon);
        }
    }

    // Print the player
    mvaddch(playerY, playerX, '@');
    refresh();
}

// tabe gharar dadan tala rooye naghshe (tala adi)
void placeRegularGold(char map[MAP_HEIGHT][MAP_WIDTH])
{
    int numGoldBags = 20;
    while (numGoldBags > 0)
    {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;
        // faghat agar makani kaf (FLOOR) dashte bashad
        if (map[y][x] == FLOOR)
        {
            map[y][x] = GOLD_ICON;
            numGoldBags--;
        }
    }
}

// tabe gharar dadan talaye siyah rooye naghshe
void placeBlackGold(char map[MAP_HEIGHT][MAP_WIDTH])
{
    int numBlackGoldBags = 5;
    while (numBlackGoldBags > 0)
    {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;
        // tala rooye kaf otagh gharar girad
        if (map[y][x] == FLOOR)
        {
            map[y][x] = BLACK_GOLD_ICON;
            numBlackGoldBags--;
        }
    }
}

// tabe etela resani be bazikon hengam bardasht tala
void show_gold_message(int collected)
{
    attron(A_BOLD);
    mvprintw(MAP_HEIGHT, 0, "you have collected %d golds", collected);
    attroff(A_BOLD);
    refresh();
    // movaghat ta bazikon motevajeh shavad
    napms(700);
    // pak kardan payam
    move(MAP_HEIGHT, 0);
    clrtoeol();
    refresh();
}

void display_final_score()
{
    clear();
    attron(A_BOLD);
    mvprintw(MAP_HEIGHT / 2 - 1, MAP_WIDTH / 2 - 10, "bazi be payan resid!");
    mvprintw(MAP_HEIGHT / 2, MAP_WIDTH / 2 - 10, "kol tala: %d", playerGold);
    mvprintw(MAP_HEIGHT / 2 + 1, MAP_WIDTH / 2 - 10, "emtiyaz shoma: %d", playerGold);
    attroff(A_BOLD);
    refresh();
    getch();
}

void placeFood(char map[MAP_HEIGHT][MAP_WIDTH])
{
    int numFoodItems = 10;
    while (numFoodItems > 0)
    {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;
        // Place food only on floor tiles
        if (map[y][x] == FLOOR)
        {
            map[y][x] = FOOD_ICON;
            numFoodItems--;
        }
    }
}

void consumeFood(Player *player, int foodIndex)
{
    if (foodIndex < 0 || foodIndex >= foodCount)
        return;

    Food food = foodInventory[foodIndex];
    player->health += food.healthRestore;
    if (player->health > 100)
        player->health = 100;

    // Remove the consumed food from inventory
    for (int i = foodIndex; i < foodCount - 1; i++)
    {
        foodInventory[i] = foodInventory[i + 1];
    }
    foodCount--;
}

void placeWeapon(char map[MAP_HEIGHT][MAP_WIDTH], int x, int y, Weapon weapon)
{
    map[y][x] = weapon.icon; // Place the weapon icon on the map
}

void placeWeapons(char map[MAP_HEIGHT][MAP_WIDTH])
{
    int numWeapons = 5;
    while (numWeapons > 0)
    {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;
        // Place weapon only on floor tiles
        if (map[y][x] == FLOOR)
        {
            Weapon weapon;
            switch (rand() % 5)
            {
            case 0:
                weapon = (Weapon){"Mace", WEAPON_ICON_MACE, 10};
                break;
            case 1:
                weapon = (Weapon){"Dagger", WEAPON_ICON_DAGGER, 5};
                break;
            case 2:
                weapon = (Weapon){"Wand", WEAPON_ICON_WAND, 8};
                break;
            case 3:
                weapon = (Weapon){"Arrow", WEAPON_ICON_ARROW, 7};
                break;
            case 4:
                weapon = (Weapon){"Sword", WEAPON_ICON_SWORD, 12};
                break;
            }
            placeWeapon(map, x, y, weapon);
            numWeapons--;
        }
    }
}

void collectItem(char map[MAP_HEIGHT][MAP_WIDTH], int x, int y)
{
    char cell = map[y][x];
    if (cell == WEAPON_ICON_MACE || cell == WEAPON_ICON_DAGGER ||
        cell == WEAPON_ICON_WAND || cell == WEAPON_ICON_ARROW ||
        cell == WEAPON_ICON_SWORD)
    {
        // Determine which weapon it is, then add to inventory:
        Weapon newWeapon;
        if (cell == WEAPON_ICON_MACE)
        {
            newWeapon = (Weapon){"Mace", WEAPON_ICON_MACE, 10};
        }
        else if (cell == WEAPON_ICON_DAGGER)
        {
            newWeapon = (Weapon){"Dagger", WEAPON_ICON_DAGGER, 5};
        }
        else if (cell == WEAPON_ICON_WAND)
        {
            newWeapon = (Weapon){"Wand", WEAPON_ICON_WAND, 8};
        }
        else if (cell == WEAPON_ICON_ARROW)
        {
            newWeapon = (Weapon){"Arrow", WEAPON_ICON_ARROW, 7};
        }
        else if (cell == WEAPON_ICON_SWORD)
        {
            newWeapon = (Weapon){"Sword", WEAPON_ICON_SWORD, 12};
        }

        // Add to the inventory if under max count
        if (weaponCount < MAX_WEAPON_ITEMS)
        {
            weaponInventory[weaponCount++] = newWeapon;
            mvprintw(MAP_HEIGHT, 0, "Collected a %s", newWeapon.name);
        }
        // Remove the icon from the map
        map[y][x] = FLOOR;
    }
    // Handle other items (gold, food) similarly
    // ...existing code...
}

void showWeaponInventory()
{
    clear(); // Clear the current window or designate a separate window for the menu
    mvprintw(0, 0, "Weapon Inventory:");
    for (int i = 0; i < weaponCount; i++)
    {
        mvprintw(i + 1, 0, "%d. %s (%c)", i + 1, weaponInventory[i].name, weaponInventory[i].icon);
    }
    mvprintw(weaponCount + 2, 0, "Press the number of the weapon to equip it as your default.");
    refresh();

    // Wait for user input to switch weapons (if applicable)
    int ch = getch();
    // Assuming user inputs a valid number, update the default weapon accordingly.
    if (ch >= '1' && ch <= '0' + weaponCount)
    {
        int index = ch - '1';
        // update default or current weapon
        // player.weapon = weaponInventory[index];
        mvprintw(weaponCount + 3, 0, "Default weapon changed to: %s", weaponInventory[index].name);
        refresh();
        // Optionally wait for a moment before clearing the screen
    }
}

void placeEnemiesInRoom(Room room, Enemy enemies[], int *enemyCount)
{
    if (*enemyCount >= MAX_ENEMIES)
        return; // Ensure we do not exceed the maximum number of enemies

    int enemyTypeIndex = rand() % 5; // Returns a number between 0 and 4

    Enemy enemy;
    enemy.x = room.centerX; // or random placement within room boundaries
    enemy.y = room.centerY;
    enemy.type = (EnemyType)enemyTypeIndex;
    enemy.isActive = 1;

    switch (enemy.type)
    {
    case ENEMY_DEMON:
        enemy.damageThreshold = 5;
        enemy.chasingSteps = 0; // No special chasing duration
        break;
    case ENEMY_FIRE:
        enemy.damageThreshold = 10;
        enemy.chasingSteps = 0;
        break;
    case ENEMY_GIANT:
        enemy.damageThreshold = 15;
        enemy.chasingSteps = 5; // Giant chases for 5 steps
        break;
    case ENEMY_SNAKE:
        enemy.damageThreshold = 20;
        enemy.chasingSteps = -1; // Special marker (-1) maybe indicates unlimited chasing
        break;
    case ENEMY_UNDEAD:
        enemy.damageThreshold = 30;
        enemy.chasingSteps = 5; // Undead chases for 5 steps when adjacent
        break;
    }

    enemies[(*enemyCount)++] = enemy;
}

void moveEnemyTowardsPlayer(Enemy *enemy, int playerX, int playerY, char map[MAP_HEIGHT][MAP_WIDTH])
{
    if (!enemy->isActive)
        return;

    if (enemy->chasingSteps == 0)
    {
        return;
    }

    int dx = 0, dy = 0;
    if (enemy->x < playerX)
        dx = 1;
    else if (enemy->x > playerX)
        dx = -1;

    if (enemy->y < playerY)
        dy = 1;
    else if (enemy->y > playerY)
        dy = -1;

    if (isWalkable(map, enemy->x + dx, enemy->y + dy))
    {
        enemy->x += dx;
        enemy->y += dy;
    }

    if (enemy->chasingSteps > 0)
        enemy->chasingSteps--;
}

void processCombat(Player *player, Enemy *enemy, int weaponDamage)
{
    enemy->damageThreshold -= weaponDamage;

    if (enemy->damageThreshold <= 0)
    {
        displayMessage("You have defeated the enemy!");
        enemy->isActive = 0;
    }
    else
    {
        int enemyAttackDamage = 2; // You can vary this per enemy type
        player->health -= enemyAttackDamage;
        char msg[100];
        sprintf(msg, "The enemy attacked you for %d damage!", enemyAttackDamage);
        displayMessage(msg);
    }
}

int isEnemyInRoom(Enemy *enemy, Room room)
{
    return (enemy->x >= room.x && enemy->x <= (room.x + room.width) &&
            enemy->y >= room.y && enemy->y <= (room.y + room.height));
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
        printf("sistem shoma az rang poshtibani nemikonad\n");
        exit(1);
    }
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK); // talaye adi
    init_pair(2, COLOR_RED, COLOR_BLACK);    // talaye siyah

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

    // Initialize player
    Player player = {0, 0, 100, 0, 0}; // Initial health, gold, and hunger

    // Place the player in the first room
    playerX = rooms[0].x + rooms[0].width / 2;
    playerY = rooms[0].y + rooms[0].height / 2;
    rooms[0].visited = true; // Mark first room as visited

    placeRegularGold(map);
    placeBlackGold(map);
    placeFood(map);
    placeWeapons(map);

    for (int i = 0; i < room_count; i++)
    {
        placeEnemiesInRoom(rooms[i], enemies, &enemyCount);
    }

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
        else if (ch == 'E' || ch == 'e')
        {
            showFoodMenu(&player);
            // pardazesh entekhab va be-rozresani salamat/goshnegi
        }
        else if (ch == 'i')
        {
            showWeaponInventory();
        }

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
            // hazf tala az rooye zamin (jaygozari kaf)
            map[newY][newX] = FLOOR;
        }
        else if (cell == FOOD_ICON)
        {
            if (foodCount < MAX_FOOD_ITEMS)
            {
                foodInventory[foodCount++] = (Food){newX, newY, COMMON_FOOD, COMMON_FOOD_HEALTH_RESTORE};
                map[newY][newX] = FLOOR;
            }
        }
        else
        {
            collectItem(map, newX, newY);
        }

        int rIndex = roomIndexAt(room_count, rooms, playerX, playerY); // Declare rIndex here

        for (int i = 0; i < enemyCount; i++)
        {
            if (isEnemyInRoom(&enemies[i], rooms[rIndex]))
            {
                enemies[i].isActive = 1;
                moveEnemyTowardsPlayer(&enemies[i], playerX, playerY, map);
            }
            else
            {
                enemies[i].isActive = 0;
            }
        }

        for (int i = 0; i < enemyCount; i++)
        {
            if (enemies[i].isActive && enemies[i].x == playerX && enemies[i].y == playerY)
            {
                processCombat(&player, &enemies[i], weaponInventory[0].damage); // Use the first weapon's damage
            }
        }

        printDungeon(map, visible, rooms, room_count);
    }

    display_final_score();

    // Cleanup ncurses
    endwin();
    return 0;
}

void displayMessage(const char *message)
{
    mvprintw(MAP_HEIGHT + 1, 0, "%s", message);
    refresh();
    napms(1000);
    move(MAP_HEIGHT + 1, 0);
    clrtoeol();
}
