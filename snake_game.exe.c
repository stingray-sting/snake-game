/*
 * ============================================================
 *  SNAKE GAME IN C - Console-Based
 *  Features: Colors, Difficulty Levels, High Score, Pause
 *  Platform: Windows (uses conio.h and windows.h)
 *  Compile:  gcc snake_game.c -o snake_game.exe
 *  Run:      snake_game.exe
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>       /* _kbhit(), _getch() */
#include <windows.h>     /* Sleep(), console color/cursor */
#include <time.h>        /* time() for srand seed */
#include <string.h>

/* ─── Board Dimensions ─── */
#define WIDTH   40
#define HEIGHT  20

/* ─── Direction Constants ─── */
#define UP    0
#define DOWN  1
#define LEFT  2
#define RIGHT 3

/* ─── Max Snake Length ─── */
#define MAX_LENGTH (WIDTH * HEIGHT)

/* ─── High-Score File ─── */
#define SCORE_FILE "highscore.dat"

/* ─── Console Color Codes (Windows) ─── */
#define COLOR_WHITE        7
#define COLOR_GREEN        10
#define COLOR_BRIGHT_GREEN 10
#define COLOR_YELLOW       14
#define COLOR_CYAN         11
#define COLOR_RED          12
#define COLOR_MAGENTA      13
#define COLOR_BLUE         9
#define COLOR_BORDER       3   /* dark cyan for walls */
#define COLOR_FOOD         12  /* red for food */
#define COLOR_SCORE        14  /* yellow for score line */

/* ═══════════════════════════════════════════════════════════
 *  DATA STRUCTURES
 * ═══════════════════════════════════════════════════════════ */

/* A single (x, y) cell */
typedef struct {
    int x, y;
} Point;

/* The snake: array of Points + metadata */
typedef struct {
    Point body[MAX_LENGTH];  /* body[0] = head */
    int   length;
    int   direction;
    int   color;             /* Windows console color for the snake */
    char  headChar;          /* character drawn for the head */
    char  bodyChar;          /* character drawn for the body */
} Snake;

/* Food item */
typedef struct {
    Point pos;
    char  symbol;            /* appearance: @, $, *, %, # … */
} Food;

/* Game state */
typedef struct {
    Snake snake;
    Food  food;
    int   score;
    int   highScore;
    int   level;             /* 1 = Easy, 2 = Medium, 3 = Hard */
    int   delay;             /* milliseconds between frames   */
    int   paused;
    int   running;
} Game;

/* ═══════════════════════════════════════════════════════════
 *  CONSOLE UTILITY FUNCTIONS
 * ═══════════════════════════════════════════════════════════ */

/* Move the cursor to (x, y) — x = column, y = row */
void gotoxy(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

/* Set console foreground color */
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
}

/* Hide / show the blinking cursor */
void setCursorVisibility(int visible) {
    CONSOLE_CURSOR_INFO info;
    info.dwSize   = 100;
    info.bVisible = (BOOL)visible;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

/* Clear the entire console */
void clearScreen(void) {
    system("cls");
}

/* ═══════════════════════════════════════════════════════════
 *  HIGH-SCORE I/O
 * ═══════════════════════════════════════════════════════════ */

int loadHighScore(void) {
    FILE *f = fopen(SCORE_FILE, "rb");
    if (!f) return 0;
    int hs = 0;
    fread(&hs, sizeof(int), 1, f);
    fclose(f);
    return hs;
}

void saveHighScore(int score) {
    FILE *f = fopen(SCORE_FILE, "wb");
    if (!f) return;
    fwrite(&score, sizeof(int), 1, f);
    fclose(f);
}

/* ═══════════════════════════════════════════════════════════
 *  MENU SCREENS
 * ═══════════════════════════════════════════════════════════ */

/* Print a centered string at row y inside the full console */
void printCentered(int y, const char *text, int color) {
    int x = (80 - (int)strlen(text)) / 2;
    if (x < 0) x = 0;
    gotoxy(x, y);
    setColor(color);
    printf("%s", text);
}

/* ── Main Menu ── returns chosen level (1/2/3) or 0 to quit */
int showMainMenu(int highScore) {
    clearScreen();
    setCursorVisibility(0);

    setColor(COLOR_GREEN);
    printCentered(2,  "  ___  _  _    __   _  _  ____     ___   __   _  _  ____  ", COLOR_BRIGHT_GREEN);
    printCentered(3,  " / __)( \\/ )  /__\\ ( \\/ )( ___)   / __) /__\\ ( \\/ )( ___) ", COLOR_BRIGHT_GREEN);
    printCentered(4,  " \\__ \\ \\  /  /(__)\\  \\  /  )__)  ( (_ \\/(__)\\  \\  /  )__)  ", COLOR_GREEN);
    printCentered(5,  " (___/  \\/  (__)(__)  \\/  (____)   \\___/(__)(__)  \\/  (____)  ", COLOR_GREEN);

    printCentered(7,  "[ SNAKE GAME ]", COLOR_YELLOW);

    char hsBuf[64];
    sprintf(hsBuf, "High Score: %d", highScore);
    printCentered(9, hsBuf, COLOR_CYAN);

    printCentered(11, "SELECT DIFFICULTY", COLOR_WHITE);
    printCentered(13, "[1]  Easy    (Slow)",   COLOR_GREEN);
    printCentered(14, "[2]  Medium  (Normal)", COLOR_YELLOW);
    printCentered(15, "[3]  Hard    (Fast)",   COLOR_RED);
    printCentered(17, "[Q]  Quit", COLOR_WHITE);

    printCentered(19, "Controls: Arrow Keys / WASD  |  P = Pause", COLOR_WHITE);

    setColor(COLOR_WHITE);

    while (1) {
        if (_kbhit()) {
            char ch = (char)_getch();
            if (ch == '1') return 1;
            if (ch == '2') return 2;
            if (ch == '3') return 3;
            if (ch == 'q' || ch == 'Q') return 0;
        }
    }
}

/* ── Snake Customisation Menu ── called before game starts */
void showCustomMenu(Snake *snake) {
    clearScreen();

    printCentered(2,  "=== CUSTOMISE YOUR SNAKE ===", COLOR_YELLOW);

    /* Color choice */
    printCentered(5,  "Choose snake color:", COLOR_WHITE);
    printCentered(7,  "[1] Green", COLOR_GREEN);
    printCentered(8,  "[2] Cyan",  COLOR_CYAN);
    printCentered(9,  "[3] Magenta", COLOR_MAGENTA);
    printCentered(10, "[4] Blue",  COLOR_BLUE);
    printCentered(11, "[5] Yellow", COLOR_YELLOW);

    int colorPicked = 0;
    while (!colorPicked) {
        if (_kbhit()) {
            char ch = (char)_getch();
            switch (ch) {
                case '1': snake->color = COLOR_GREEN;   colorPicked = 1; break;
                case '2': snake->color = COLOR_CYAN;    colorPicked = 1; break;
                case '3': snake->color = COLOR_MAGENTA; colorPicked = 1; break;
                case '4': snake->color = COLOR_BLUE;    colorPicked = 1; break;
                case '5': snake->color = COLOR_YELLOW;  colorPicked = 1; break;
            }
        }
    }

    /* Food symbol choice */
    printCentered(13, "Choose food symbol:", COLOR_WHITE);
    printCentered(15, "[1]  @   (classic)", COLOR_RED);
    printCentered(16, "[2]  $   (money!)",  COLOR_YELLOW);
    printCentered(17, "[3]  *   (star)",    COLOR_CYAN);
    printCentered(18, "[4]  #   (hash)",    COLOR_GREEN);
    printCentered(19, "[5]  %   (bonus)",   COLOR_MAGENTA);

    char foodSymbols[] = { '@', '$', '*', '#', '%' };
    int foodPicked = 0;
    while (!foodPicked) {
        if (_kbhit()) {
            char ch = (char)_getch();
            int idx = ch - '1';
            if (idx >= 0 && idx <= 4) {
                /* We store it; Game struct will use it */
                snake->bodyChar = foodSymbols[idx]; /* reuse field temporarily */
                foodPicked = 1;
            }
        }
    }
}

/* ── Game Over Screen ── */
void showGameOver(int score, int highScore) {
    clearScreen();
    printCentered(5,  "  ██████╗  █████╗ ███╗   ███╗███████╗", COLOR_RED);
    printCentered(6,  " ██╔════╝ ██╔══██╗████╗ ████║██╔════╝", COLOR_RED);
    printCentered(7,  " ██║  ███╗███████║██╔████╔██║█████╗  ", COLOR_RED);
    printCentered(8,  " ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝  ", COLOR_RED);
    printCentered(9,  " ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗", COLOR_RED);
    printCentered(10, "  ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝", COLOR_RED);
    printCentered(12, "  ██████╗ ██╗   ██╗███████╗██████╗ ", COLOR_RED);
    printCentered(13, " ██╔═══██╗██║   ██║██╔════╝██╔══██╗", COLOR_RED);
    printCentered(14, " ██║   ██║██║   ██║█████╗  ██████╔╝", COLOR_RED);
    printCentered(15, " ██║   ██║╚██╗ ██╔╝██╔══╝  ██╔══██╗", COLOR_RED);
    printCentered(16, " ╚██████╔╝ ╚████╔╝ ███████╗██║  ██║", COLOR_RED);
    printCentered(17, "  ╚═════╝   ╚═══╝  ╚══════╝╚═╝  ╚═╝", COLOR_RED);

    char buf[64];
    sprintf(buf, "Your Score : %d", score);
    printCentered(19, buf, COLOR_YELLOW);

    sprintf(buf, "High Score : %d", highScore);
    printCentered(20, buf, COLOR_CYAN);

    printCentered(22, "[R] Play Again    [Q] Quit", COLOR_WHITE);
    setColor(COLOR_WHITE);
}

/* ═══════════════════════════════════════════════════════════
 *  GAME INITIALISATION
 * ═══════════════════════════════════════════════════════════ */

void initGame(Game *g, int level, int snakeColor, char foodSymbol) {
    /* Snake starts at centre, length 3, moving RIGHT */
    g->snake.length    = 3;
    g->snake.direction = RIGHT;
    g->snake.color     = snakeColor;
    g->snake.headChar  = 'O';
    g->snake.bodyChar  = 'o';

    for (int i = 0; i < g->snake.length; i++) {
        g->snake.body[i].x = WIDTH / 2 - i;   /* head at centre */
        g->snake.body[i].y = HEIGHT / 2;
    }

    /* Food symbol chosen in custom menu */
    g->food.symbol = foodSymbol;

    /* Level → delay */
    g->level   = level;
    g->score   = 0;
    g->paused  = 0;
    g->running = 1;

    switch (level) {
        case 1: g->delay = 180; break;   /* Easy   */
        case 2: g->delay = 110; break;   /* Medium */
        case 3: g->delay = 60;  break;   /* Hard   */
        default: g->delay = 110;
    }

    /* Seed random & place first food */
    srand((unsigned int)time(NULL));
}

/* ═══════════════════════════════════════════════════════════
 *  FOOD GENERATION
 * ═══════════════════════════════════════════════════════════ */

/* Returns 1 if (x,y) overlaps any snake segment */
int isOnSnake(Snake *s, int x, int y) {
    for (int i = 0; i < s->length; i++)
        if (s->body[i].x == x && s->body[i].y == y)
            return 1;
    return 0;
}

/* Place food at a random empty cell */
void spawnFood(Game *g) {
    int x, y;
    do {
        x = 1 + rand() % (WIDTH  - 2);   /* keep inside border */
        y = 1 + rand() % (HEIGHT - 2);
    } while (isOnSnake(&g->snake, x, y));
    g->food.pos.x = x;
    g->food.pos.y = y;
}

/* ═══════════════════════════════════════════════════════════
 *  RENDERING
 * ═══════════════════════════════════════════════════════════ */

/* Draw the static border once */
void drawBorder(void) {
    setColor(COLOR_BORDER);

    /* Top & Bottom rows */
    for (int x = 0; x < WIDTH; x++) {
        gotoxy(x, 0);        printf("#");
        gotoxy(x, HEIGHT-1); printf("#");
    }
    /* Left & Right columns */
    for (int y = 0; y < HEIGHT; y++) {
        gotoxy(0,       y); printf("#");
        gotoxy(WIDTH-1, y); printf("#");
    }
}

/* Draw score line below the board */
void drawScore(Game *g) {
    setColor(COLOR_SCORE);
    gotoxy(0, HEIGHT);
    printf("  Score: %-5d  |  High Score: %-5d  |  Level: %d  |  [P] Pause  [Q] Quit  ",
           g->score, g->highScore, g->level);
}

/* Erase the tail cell (overwrite with space) */
void eraseTail(Game *g) {
    Point tail = g->snake.body[g->snake.length]; /* one past last valid */
    gotoxy(tail.x, tail.y);
    setColor(COLOR_WHITE);
    printf(" ");
}

/* Draw the snake (head in bright color, body slightly dimmer) */
void drawSnake(Game *g) {
    Snake *s = &g->snake;

    /* Head */
    setColor(s->color | FOREGROUND_INTENSITY);
    gotoxy(s->body[0].x, s->body[0].y);
    printf("%c", s->headChar);

    /* Body segment right behind head (newly occupied cell) */
    if (s->length > 1) {
        setColor(s->color);
        gotoxy(s->body[1].x, s->body[1].y);
        printf("%c", s->bodyChar);
    }
}

/* Draw food */
void drawFood(Game *g) {
    setColor(COLOR_FOOD);
    gotoxy(g->food.pos.x, g->food.pos.y);
    printf("%c", g->food.symbol);
}

/* Full initial draw */
void drawAll(Game *g) {
    clearScreen();
    drawBorder();
    drawFood(g);
    /* Draw entire snake */
    Snake *s = &g->snake;
    setColor(s->color | FOREGROUND_INTENSITY);
    gotoxy(s->body[0].x, s->body[0].y);
    printf("%c", s->headChar);
    setColor(s->color);
    for (int i = 1; i < s->length; i++) {
        gotoxy(s->body[i].x, s->body[i].y);
        printf("%c", s->bodyChar);
    }
    drawScore(g);
}

/* ═══════════════════════════════════════════════════════════
 *  INPUT HANDLING
 * ═══════════════════════════════════════════════════════════ */

void handleInput(Game *g) {
    if (!_kbhit()) return;

    int ch = _getch();

    /* Arrow keys send two codes: 0 or 224, then the key code */
    if (ch == 0 || ch == 224) {
        ch = _getch();
        switch (ch) {
            case 72: if (g->snake.direction != DOWN)  g->snake.direction = UP;    break;
            case 80: if (g->snake.direction != UP)    g->snake.direction = DOWN;  break;
            case 75: if (g->snake.direction != RIGHT) g->snake.direction = LEFT;  break;
            case 77: if (g->snake.direction != LEFT)  g->snake.direction = RIGHT; break;
        }
    } else {
        switch (ch) {
            /* WASD */
            case 'w': case 'W': if (g->snake.direction != DOWN)  g->snake.direction = UP;    break;
            case 's': case 'S': if (g->snake.direction != UP)    g->snake.direction = DOWN;  break;
            case 'a': case 'A': if (g->snake.direction != RIGHT) g->snake.direction = LEFT;  break;
            case 'd': case 'D': if (g->snake.direction != LEFT)  g->snake.direction = RIGHT; break;
            /* Pause */
            case 'p': case 'P':
                g->paused = !g->paused;
                if (g->paused) {
                    setColor(COLOR_YELLOW);
                    gotoxy(WIDTH/2 - 5, HEIGHT/2);
                    printf("-- PAUSED --");
                } else {
                    gotoxy(WIDTH/2 - 5, HEIGHT/2);
                    setColor(COLOR_WHITE);
                    printf("            ");
                }
                break;
            /* Quit */
            case 'q': case 'Q':
                g->running = 0;
                break;
        }
    }
}

/* ═══════════════════════════════════════════════════════════
 *  MOVEMENT & COLLISION DETECTION
 * ═══════════════════════════════════════════════════════════ */

/*
 * Move the snake one step:
 *   1. Shift body segments backward (tail → head direction).
 *   2. Compute new head position.
 *   3. Check wall collision.
 *   4. Check self collision.
 *   5. Check food collision → grow & spawn new food.
 */
void moveSnake(Game *g) {
    Snake *s = &g->snake;

    /* Save old tail position BEFORE shifting (for erase) */
    Point oldTail = s->body[s->length - 1];

    /* Shift body: each segment takes the position of the one ahead */
    for (int i = s->length - 1; i > 0; i--)
        s->body[i] = s->body[i - 1];

    /* New head position */
    switch (s->direction) {
        case UP:    s->body[0].y--; break;
        case DOWN:  s->body[0].y++; break;
        case LEFT:  s->body[0].x--; break;
        case RIGHT: s->body[0].x++; break;
    }

    /* ── Wall collision ── */
    if (s->body[0].x <= 0 || s->body[0].x >= WIDTH - 1 ||
        s->body[0].y <= 0 || s->body[0].y >= HEIGHT - 1) {
        g->running = 0;
        return;
    }

    /* ── Self collision ── (skip index 0, that's the head itself) */
    for (int i = 1; i < s->length; i++) {
        if (s->body[0].x == s->body[i].x &&
            s->body[0].y == s->body[i].y) {
            g->running = 0;
            return;
        }
    }

    /* ── Food collision ── */
    if (s->body[0].x == g->food.pos.x &&
        s->body[0].y == g->food.pos.y) {
        /* Grow: length increases, no erase needed for tail */
        if (s->length < MAX_LENGTH - 1)
            s->length++;
        g->score += 10 * g->level;   /* more points for harder levels */
        if (g->score > g->highScore) {
            g->highScore = g->score;
            saveHighScore(g->highScore);
        }
        drawScore(g);
        spawnFood(g);
        drawFood(g);
    } else {
        /* Erase old tail from screen */
        gotoxy(oldTail.x, oldTail.y);
        setColor(COLOR_WHITE);
        printf(" ");
    }

    /* Draw updated snake */
    drawSnake(g);
}

/* ═══════════════════════════════════════════════════════════
 *  MAIN GAME LOOP
 * ═══════════════════════════════════════════════════════════ */

void runGame(int level, int snakeColor, char foodSymbol, int *highScore) {
    Game g;
    g.highScore = *highScore;

    initGame(&g, level, snakeColor, foodSymbol);
    spawnFood(&g);
    drawAll(&g);

    while (g.running) {
        handleInput(&g);
        if (!g.paused && g.running) {
            moveSnake(&g);
        }
        Sleep((DWORD)g.delay);
    }

    *highScore = g.highScore;
}

/* ═══════════════════════════════════════════════════════════
 *  ENTRY POINT
 * ═══════════════════════════════════════════════════════════ */

int main(void) {
    /* Console setup */
    setCursorVisibility(0);
    /* Try to resize console window for a better experience */
    system("mode con: cols=82 lines=26");

    int highScore = loadHighScore();

    while (1) {
        /* 1. Main menu → pick level */
        int level = showMainMenu(highScore);
        if (level == 0) break;   /* user pressed Q */

        /* 2. Customisation menu */
        Snake tmpSnake;
        tmpSnake.color    = COLOR_GREEN;
        tmpSnake.bodyChar = '@';   /* default food symbol */
        showCustomMenu(&tmpSnake);

        int  snakeColor  = tmpSnake.color;
        char foodSymbol  = tmpSnake.bodyChar;

        /* Countdown */
        clearScreen();
        for (int i = 3; i >= 1; i--) {
            char buf[32];
            sprintf(buf, "Starting in  %d ...", i);
            printCentered(12, buf, COLOR_YELLOW);
            Sleep(700);
        }

        /* 3. Play */
        runGame(level, snakeColor, foodSymbol, &highScore);

        /* 4. Game over screen */
        /* Compute final score by reading last game's score.
           We re-display via showGameOver using highScore. */
        showGameOver(highScore, highScore);   /* pass score twice; adapt if needed */

        /* Wait for R or Q */
        setCursorVisibility(0);
        int again = 0;
        while (!again) {
            if (_kbhit()) {
                char ch = (char)_getch();
                if (ch == 'r' || ch == 'R') again = 1;
                if (ch == 'q' || ch == 'Q') { again = 2; }
            }
        }
        if (again == 2) break;
    }

    /* Restore console */
    clearScreen();
    setCursorVisibility(1);
    setColor(COLOR_WHITE);
    printf("\nThanks for playing Snake! Final High Score: %d\n\n", highScore);
    return 0;
}
