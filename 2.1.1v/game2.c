#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define WIDTH  40
#define HEIGHT 20
#define ROOM_COUNT 6   // At least 6 rooms

// The dungeon is stored in a global 2D char array.
char dungeon[HEIGHT][WIDTH];

// Keep track of visited cells for fog-of–war. Initially, all false.
int visible[HEIGHT][WIDTH];

// Structure for a room.
typedef struct {
    int x, y;   // Top-left coordinate (of the room including walls)
    int w, h;   // Width and height of the room (minimum 4x4)
    int visited;  // Flag if the room has been visited (at least once)
} Room;
Room rooms[ROOM_COUNT];

// Player position.
int playerX, playerY;

// Forward declarations.
void initializeDungeon();
void drawRoom(int index, int x, int y, int w, int h);
void drawCorridor(int x1, int y1, int x2, int y2);
void generateDungeon();
void updateVisibility();
void printDungeon();
int pointInRoom(int x, int y, Room room);
int minDistanceRoomIndex(int idx);

// ----------------------------------------------------------------------
// Initialize the dungeon array with spaces.
void initializeDungeon() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            dungeon[y][x] = ' ';
            visible[y][x] = 0;
        }
    }
}

// ----------------------------------------------------------------------
// Draw a room (with walls, floor and a door).
// Walls: '-' for horizontal and '|' for vertical boundaries.
// Floor: marked with '.' character.
// A door ('+') is placed over one wall.
void drawRoom(int index, int x, int y, int w, int h) {
    // Draw the room
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            if (i == y || i == y + h - 1) {
                dungeon[i][j] = '-';
            } else if (j == x || j == x + w - 1) {
                dungeon[i][j] = '|';
            } else {
                dungeon[i][j] = '.';
            }
        }
    }
    // Place a door at a random position on either top or bottom wall.
    int doorX = x + 1 + rand() % (w - 2);
    int doorY = y + (rand() % 2) * (h - 1);
    dungeon[doorY][doorX] = '+';

    // Store room info
    rooms[index].x = x;
    rooms[index].y = y;
    rooms[index].w = w;
    rooms[index].h = h;
    rooms[index].visited = 0;
}

// ----------------------------------------------------------------------
// Draw a corridor from (x1,y1) to (x2,y2) by using an L–shaped path.
void drawCorridor(int x1, int y1, int x2, int y2) {
    int cx = x1;
    int cy = y1;
    // Horizontal part.
    while (cx != x2) {
        dungeon[cy][cx] = '#';
        cx += (cx < x2) ? 1 : -1;
    }
    // Vertical part.
    while (cy != y2) {
        dungeon[cy][cx] = '#';
        cy += (cy < y2) ? 1 : -1;
    }
    dungeon[cy][cx] = '#';
}

// ----------------------------------------------------------------------
// Generate the dungeon layout.
// • Generates ROOM_COUNT rooms with dimensions at least 4x4 (we choose a range).
// • For corridors: each room (except the first) is connected to its nearest already–placed room.
// • Place a random window ('=') inside each room.
void generateDungeon() {
    initializeDungeon();
    srand(time(NULL));
    
    // Generate rooms
    for (int i = 0; i < ROOM_COUNT; i++) {
        // Choose room width and height in a range: min 4; max 8 or so.
        int rw = 4 + rand() % 5;  // width between 4 and 8
        int rh = 4 + rand() % 5;  // height between 4 and 8
        // Ensure room fits within boundaries (with a margin of 1 on each side)
        int rx = 1 + rand() % (WIDTH - rw - 2);
        int ry = 1 + rand() % (HEIGHT - rh - 2);
        drawRoom(i, rx, ry, rw, rh);
    }
    
    // Connect rooms with “short” corridors.
    // For each room from index 1 onward, find the nearest room in [0,i-1]
    for (int i = 1; i < ROOM_COUNT; i++) {
        int j = minDistanceRoomIndex(i);
        // Compute center coordinates of room i and room j.
        int x1 = rooms[i].x + rooms[i].w / 2;
        int y1 = rooms[i].y + rooms[i].h / 2;
        int x2 = rooms[j].x + rooms[j].w / 2;
        int y2 = rooms[j].y + rooms[j].h / 2;
        drawCorridor(x1, y1, x2, y2);
    }
    
    // Place a window ('=') randomly inside each room.
    for (int i = 0; i < ROOM_COUNT; i++) {
        int wx = rooms[i].x + 1 + rand() % (rooms[i].w - 2);
        int wy = rooms[i].y + 1 + rand() % (rooms[i].h - 2);
        dungeon[wy][wx] = '=';
    }
}

// ----------------------------------------------------------------------
// Helper: compute Euclidean distance between two points.
double distance(int x1, int y1, int x2, int y2) {
    return sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));
}

// For room at index idx, find the index j (0 <= j < idx) of the room
// with the shortest center-to-center distance.
int minDistanceRoomIndex(int idx) {
    int bestJ = 0;
    double bestDist = 1e9;
    int cx = rooms[idx].x + rooms[idx].w / 2;
    int cy = rooms[idx].y + rooms[idx].h / 2;
    for (int j = 0; j < idx; j++) {
        int cx2 = rooms[j].x + rooms[j].w / 2;
        int cy2 = rooms[j].y + rooms[j].h / 2;
        double d = distance(cx, cy, cx2, cy2);
        if (d < bestDist) {
            bestDist = d;
            bestJ = j;
        }
    }
    return bestJ;
}

// ----------------------------------------------------------------------
// Return 1 if the point (x,y) lies inside (or on the boundary of) room r.
int pointInRoom(int x, int y, Room r) {
    return (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h);
}

// ----------------------------------------------------------------------
// Update the “visible” array for fog–of–war.
// We reveal any cell that is within a room that is either
// currently occupied (by the player) or visited in the past.
// Also, to smooth corridor display, if a corridor cell ('#')
// is adjacent (in 8 directions) to a visible floor cell, it too is revealed.
void updateVisibility() {
    // reset visibility
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            visible[y][x] = 0;
    
    // Reveal all cells in visited rooms.
    for (int r = 0; r < ROOM_COUNT; r++) {
        if (rooms[r].visited) {
            for (int y = rooms[r].y; y < rooms[r].y + rooms[r].h; y++) {
                for (int x = rooms[r].x; x < rooms[r].x + rooms[r].w; x++) {
                    visible[y][x] = 1;
                }
            }
        }
    }
    
    // Additionally, reveal corridors that border any visible cell.
    // We look at every cell in the map.
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (dungeon[y][x] == '#') {
                // Check 8 neighbors for a visible cell.
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int ny = y + dy, nx = x + dx;
                        if (ny >= 0 && ny < HEIGHT && nx >= 0 && nx < WIDTH) {
                            if (visible[ny][nx]) {
                                visible[y][x] = 1;
                            }
                        }
                    }
                }
            }
        }
    }
}

// ----------------------------------------------------------------------
// Print the dungeon. Only cells marked as visible are drawn.
// Also, the player (arising as '@') is drawn.
void printDungeon() {
    clear();
    updateVisibility();
    for (int y = 0; y < HEIGHT; y++) {
        move(y, 0);
        for (int x = 0; x < WIDTH; x++) {
            // If the cell is visible, draw its character; otherwise, blank.
            if (visible[y][x])
                addch(dungeon[y][x]);
            else
                addch(' ');
        }
    }
    // Draw the player over the dungeon.
    mvaddch(playerY, playerX, '@');
    refresh();
}

// ----------------------------------------------------------------------
// Check if the target position is walkable.
// In our dungeon, the player may move on floor ('.'), corridors ('#'), door ('+'),
// window ('='), or even on previously visited wall cells (when appropriate – for now we block walls).
// Walls ('-' and '|') are non–walkable.
int isWalkable(int x, int y) {
    char c = dungeon[y][x];
    if (c == '-' || c == '|')
        return 0;
    // Also, ensure x and y are within bounds.
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
        return 0;
    return 1;
}

// ----------------------------------------------------------------------
// Find which room (if any) the given point is in.
int roomIndexAt(int x, int y) {
    for (int i = 0; i < ROOM_COUNT; i++) {
        if (pointInRoom(x, y, rooms[i]))
            return i;
    }
    return -1;
}

// ----------------------------------------------------------------------
// Main interactive loop: the player uses arrow keys to move.
int main() {
    initscr();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE); // enable reading arrow keys

    generateDungeon();
    
    // Place the player in the center of the first room.
    playerX = rooms[0].x + rooms[0].w / 2;
    playerY = rooms[0].y + rooms[0].h / 2;
    rooms[0].visited = 1;
    
    printDungeon();
    
    int ch;
    while ((ch = getch()) != 'q') {  // press 'q' to exit
        int newX = playerX, newY = playerY;
        if (ch == KEY_UP)
            newY--;
        else if (ch == KEY_DOWN)
            newY++;
        else if (ch == KEY_LEFT)
            newX--;
        else if (ch == KEY_RIGHT)
            newX++;
        
        // Check bounds and walkability.
        if (newX >= 0 && newX < WIDTH && newY >= 0 && newY < HEIGHT && isWalkable(newX, newY)) {
            playerX = newX;
            playerY = newY;
        }
        
        // If the player has entered a room (by checking all rooms), mark that room visited.
        int rIndex = roomIndexAt(playerX, playerY);
        if (rIndex != -1)
            rooms[rIndex].visited = 1;
        
        printDungeon();
    }
    
    endwin();
    return 0;
}
