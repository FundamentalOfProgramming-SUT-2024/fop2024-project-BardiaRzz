#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_LEN 50
#define USERS_FILE "users.txt"

void center_print(int row, const char *text) {
    int col = (COLS - strlen(text)) / 2;
    mvprintw(row, col, "%s", text);
}

int userExists(const char *username) {
    FILE *fp = fopen(USERS_FILE, "r");
    if (fp == NULL) {
        return 0;
    }
    char fileUsername[MAX_LEN];
    while (fscanf(fp, "%s", fileUsername) == 1) {
        if (strcmp(fileUsername, username) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int validatePassword(const char *password) {
    int len = strlen(password);
    if (len < 7) return 0;
    int hasDigit = 0, hasUpper = 0, hasLower = 0;
    for (int i = 0; i < len; i++) {
        if (isdigit(password[i])) hasDigit = 1;
        else if (isupper(password[i])) hasUpper = 1;
        else if (islower(password[i])) hasLower = 1;
    }
    return (hasDigit && hasUpper && hasLower);
}

int validateEmail(const char *email) {
    char *atPtr = strchr(email, '@');
    if (atPtr == NULL || atPtr == email || *(atPtr + 1) == '\0') return 0;
    if (strchr(email, '.') == NULL) return 0;
    return 1;
}

void saveUser(const char *username, const char *password, const char *email) {
    FILE *fp = fopen(USERS_FILE, "a");
    if (fp == NULL) {
        center_print(20, "Error: Cannot save user data!");
        return;
    }
    fprintf(fp, "%s %s %s\n", username, password, email);
    fclose(fp);
}

void registerUser() {
    char username[MAX_LEN], password[MAX_LEN], email[MAX_LEN];
    clear();
    center_print(2, "Register New User");

    center_print(4, "Enter Username: ");
    echo();
    mvgetnstr(5, (COLS - MAX_LEN) / 2, username, MAX_LEN - 1);
    noecho();

    if (userExists(username)) {
        center_print(7, "Username already exists!");
        getch();
        return;
    }

    center_print(7, "Enter Password: ");
    echo();
    mvgetnstr(8, (COLS - MAX_LEN) / 2, password, MAX_LEN - 1);
    noecho();

    if (!validatePassword(password)) {
        center_print(10, "Invalid password! Must be at least 7 characters with 1 digit, 1 uppercase, and 1 lowercase.");
        getch();
        return;
    }

    center_print(10, "Enter Email: ");
    echo();
    mvgetnstr(11, (COLS - MAX_LEN) / 2, email, MAX_LEN - 1);
    noecho();

    if (!validateEmail(email)) {
        center_print(13, "Invalid email format!");
        getch();
        return;
    }

    saveUser(username, password, email);
    center_print(15, "User registered successfully!");
    getch();
}

int validateUserLogin(const char *username, const char *password) {
    FILE *fp = fopen(USERS_FILE, "r");
    if (fp == NULL) return 0;
    char fileUsername[MAX_LEN], filePassword[MAX_LEN], fileEmail[MAX_LEN];
    while (fscanf(fp, "%s %s %s", fileUsername, filePassword, fileEmail) == 3) {
        if (strcmp(fileUsername, username) == 0 && strcmp(filePassword, password) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

void loginUser() {
    char username[MAX_LEN], password[MAX_LEN];
    clear();
    center_print(2, "Login");

    center_print(4, "Enter Username: ");
    echo();
    mvgetnstr(5, (COLS - MAX_LEN) / 2, username, MAX_LEN - 1);
    noecho();

    center_print(7, "Enter Password: ");
    echo();
    mvgetnstr(8, (COLS - MAX_LEN) / 2, password, MAX_LEN - 1);
    noecho();

    if (validateUserLogin(username, password)) {
        center_print(10, "Login successful! Welcome.");
    } else {
        center_print(10, "Invalid username or password!");
    }
    getch();
}

int main() {
    initscr();
    noecho();
    cbreak();
    curs_set(0);

    int choice;
    while (1) {
        clear();
        center_print(2, "Game Menu");
        center_print(4, "1) Register New User");
        center_print(5, "2) Login");
        center_print(6, "3) Guest Login");
        center_print(7, "Q) Quit");
        refresh();
        choice = getch();
        if (choice == 'Q' || choice == 'q') break;
        else if (choice == '1') registerUser();
        else if (choice == '2') loginUser();
        else if (choice == '3') {
            center_print(10, "Logged in as Guest");
            getch();
        }
    }
    endwin();
    return 0;
}
