// DenCoder.EXE

#include <stdlib.h>
#include <ncurses.h>
#include <stdbool.h>

#define INF 999
#define row_base 0
#define col_base 0

typedef enum Difficulty
{
    easy,
    normal,
    hard
} Difficulty;

typedef struct Cell
{
    short mines_around;
    bool mine;
    bool opened;
    bool flagged;
    bool border;
} Cell;

typedef struct Game
{
    Cell **field;
    short flag_count;
    short moves_left;
    short cursor_x;
    short cursor_y;
    bool game_is_active;
    Difficulty difficulty;
    short rows;
    short cols;
    short mines;
    bool defeat;
} Game;

WINDOW* win = NULL;
Game game;

// Rows, Cols, Mines //
short EASY[3] = {4, 8, 6};
short NORMAL[3] = {8, 16, 25}; 
short HARD[3] = {15, 30, 99};

void setCursor(void) {
    move(game.cursor_x + row_base, game.cursor_y + col_base);
    refresh();
}

void resetCursor(void) { // Move the cursor to the starting position
    game.cursor_x = 1;
    game.cursor_y = 1;
    setCursor();
}

short setParameters(Difficulty difficulty) {
    switch (difficulty)
    {
    case easy:
        game.rows = EASY[0];
        game.cols = EASY[1];
        game.mines = EASY[2];
        break;
    case normal:
        game.rows = NORMAL[0];
        game.cols = NORMAL[1];
        game.mines = NORMAL[2];
        break;
    case hard:
        game.rows = HARD[0];
        game.cols = HARD[1];
        game.mines = HARD[2];
        break;
    default:
        break;
    }
}

void printMenu() {
    clear();
    addstr("Minesweeper\n");
    if (game.game_is_active)
    {
        addstr("  Resume \n");
        addstr(" New game \n");
    }
    else
    {
        addstr(" New game \n");
    }
    addstr(" Settings \n");
    addstr("   Quit \n");
    refresh();
}

void highlightOption(short option, short x1, short x2) {
    mvaddch(option + 1, x1, '<');
    mvaddch(option + 1, x2, '>');
}

void highlightActiveOption(short option) {
    clear();
    printMenu();
    if (game.game_is_active)
    {
        switch (option)
        {
        case 0:
            highlightOption(option, 1, 8);
            break;
        case 1:
            highlightOption(option, 0, 9);
            break;
        case 2:
            highlightOption(option, 0, 9);
            break;
        case 3:
            highlightOption(option, 2, 7);
            break;
        default:
            break;
        }
    }
    else
    {
        switch (option)
        {
        case 0:
            highlightOption(option, 0, 9);
            break;
        case 1:
            highlightOption(option, 0, 9);
            break;
        case 2:
            highlightOption(option, 2, 7);
            break;
        default:
            break;
        }
    }
    refresh();
}

void printField() {
    clear();
    for (short i = 0; i < game.rows + 2; i++)
    {        
        for (short j = 0; j < game.cols + 2; j++)
        {
            if (game.field[i][j].mine && (game.defeat || (game.moves_left == 0 && game.flag_count == game.mines)))
            {
                if (game.field[i][j].flagged)
                {
                    attrset(COLOR_PAIR(1));
                    addch('*');
                    attroff(COLOR_PAIR(1));
                }
                else
                {
                    addch('*');
                }
            }
            else if (game.field[i][j].border)
            {
                addch('#');
            }
            else if (game.field[i][j].flagged)
            {
                addch('!');
            }
            else if (game.field[i][j].opened)
            {
                addch((char)game.field[i][j].mines_around + '0');
            }
            else if (!game.field[i][j].opened)
            {
                addch(' ');
            }
        }
        addch('\n');
    }
    setCursor();
    refresh();
}

void clearField() {
    for (short i = 0; i < game.rows; i++)
    {
        free(game.field[i]);
        game.field[i] = NULL;
    }
    free(game.field);
    game.field = NULL;
}

void allocateField() {
    game.field = (Cell**)malloc((game.rows + 2) * sizeof(Cell*));
    for (short i = 0; i < game.rows + 2; i++)
    {
        game.field[i] = (Cell*)malloc((game.cols + 2) * sizeof(Cell));
    }
}

void resetGame() {
    clearField();
    setParameters(game.difficulty);
    allocateField();
    for (short i = 0; i < game.rows + 2; i++)
    {        
        for (short j = 0; j < game.cols + 2; j++)
        {
            game.field[i][j].mines_around = 0;
            game.field[i][j].mine = false;
            game.field[i][j].opened = false;
            game.field[i][j].flagged = false;
            game.field[i][j].border = (i == 0 || i == game.rows + 1 || j == 0 || j == game.cols + 1);
        }
    }
    game.flag_count = 0;
    game.moves_left = INF;
    game.cursor_x = 1, game.cursor_y = 1;       // Cursor position
    game.game_is_active = true;
    game.defeat = false;
    resetCursor();
}

char end() {
    if (game.defeat)
    {
        return 'd'; // Defeat
    }
    if (game.moves_left == 0 && game.flag_count == game.mines)
    {
        return 'v'; // Victory
    }
    return 'n'; // None
}

bool gameLost() {
    curs_set(0);
    printField();
    attrset(COLOR_PAIR(2));
    mvaddch(game.cursor_x, game.cursor_y, '*');
    attroff(COLOR_PAIR(2));
    mvaddstr(game.rows + 3, 0, "BOOM! You lost! \nPress any key to restart: ");
    refresh();
    short pressed = wgetch(win);
    switch (pressed)
    {
    case 27:    // ESC key
        return false;
    default:
        break;
    }
    return true; // Try again
}

bool gameWon() {
    curs_set(0);
    printField();
    mvaddstr(game.rows + 3, 0, "You won! \nPress any key to restart: ");
    refresh();
    short pressed = wgetch(win);
    switch (pressed)
    {
    case 27:    // ESC key
        return false;
    default:
        break;
    }
    return true; // Try again
}

void updateCellsAround(short i, short j) {
    game.field[i - 1][j - 1].mines_around++;
    game.field[i - 1][j + 1].mines_around++;
    game.field[i - 1][j].mines_around++;
    game.field[i][j - 1].mines_around++;
    game.field[i][j + 1].mines_around++;
    game.field[i + 1][j - 1].mines_around++;
    game.field[i + 1][j + 1].mines_around++;
    game.field[i + 1][j].mines_around++;
}

void placeMines() {
    short tmp_mines = game.mines;
    while (tmp_mines > 0)
    {
        int i = rand() % game.rows + 1;
        int j = rand() % game.cols + 1;
        if (!game.field[i][j].mine && !(i == game.cursor_x && j == game.cursor_y))
        {
            game.field[i][j].mine = true;
            tmp_mines--;
            updateCellsAround(i, j);
        }
    }
    game.moves_left = game.rows * game.cols - game.mines;
}

void openCells(short x, short y) {
    if (game.field[x][y].opened || game.field[x][y].border || game.field[x][y].flagged) 
    {
        return;
    }
    game.field[x][y].opened = true;
    game.moves_left--;
    if (game.field[x][y].mines_around == 0)
    {
        for (short dx = -1; dx <= 1; dx++) {
            for (short dy = -1; dy <= 1; dy++) {
                if (dx != 0 || dy != 0) {
                    openCells(x + dx, y + dy);
                }
            }
        }
    }
}

bool playersMove() {
    short pressed = wgetch(win);
    switch (pressed)
    {
    case 27: // ESC key
        return true; // Exit to menu
        break;
    case 'd':
    case 'D':
    case KEY_RIGHT:
        if (game.cursor_y < game.cols)
        {
            game.cursor_y++;
        }
        else 
        {
            game.cursor_y = 1;
        }
        setCursor(); // Moving the cursor to the next position
        break;
    case 'a':
    case 'A':
    case KEY_LEFT:
        if (game.cursor_y > 1)
        {
            game.cursor_y--;
        }
        else
        {
            game.cursor_y = game.cols;
        }
        setCursor(); // Moving the cursor to the next position
        break;
    case 'w':
    case 'W':
    case KEY_UP:
        if (game.cursor_x > 1)
        {
            game.cursor_x--;
        }
        else
        {
            game.cursor_x = game.rows;
        }
        setCursor(); // Moving the cursor to the next position
        break;
    case 's':
    case 'S':
    case KEY_DOWN:
        if (game.cursor_x < game.rows)
        {
            game.cursor_x++;
        }
        else
        {
            game.cursor_x = 1;
        }
        setCursor(); // Moving the cursor to the next position
        break;
    case ' ': // Space bar, flag the cell
        if (game.field[game.cursor_x][game.cursor_y].opened)
        {
            beep();
            break;
        }
        if (game.field[game.cursor_x][game.cursor_y].flagged)
        {
            game.field[game.cursor_x][game.cursor_y].flagged = false;
            if (game.field[game.cursor_x][game.cursor_y].mine)
            {
                game.flag_count--;
            }
        }
        else
        {
            game.field[game.cursor_x][game.cursor_y].flagged = true;
            if (game.field[game.cursor_x][game.cursor_y].mine)
            {
                game.flag_count++;
            }
        }
        break;
    case 10: // ENTER key, open the cell
        if (!game.field[game.cursor_x][game.cursor_y].opened && !game.field[game.cursor_x][game.cursor_y].flagged)
        {
            if (game.field[game.cursor_x][game.cursor_y].mine)
            {
                game.defeat = true;
                break;
            }
            if (game.moves_left == INF)
            {
                placeMines();
            }
            openCells(game.cursor_x, game.cursor_y);
        }
        else
        {
            beep(); // Illegal move signal
        }
        break;
    default:
        break;
    }
    return false;
}

void sweep() {
    while (true)
    {
        switch (end())
        {
        case 'n': // Check game for end | v = victory | d = defeat | n = none
            if(playersMove())
            {
                return;
            }
            printField();
            break;
        case 'v':
            if (gameWon()) // Try again
            {
                clear();
                curs_set(2);
                resetGame();
                printField();
                continue;
            }
            else            // Exit to menu
            {
                return;
            }
            break;
        case 'd':
            if (gameLost()) // Try again
            {
                clear();
                curs_set(2);
                resetGame();
                printField();
                continue;
            }
            else            // Exit to menu
            {
                return;
            }
            break;
        default:
            break;
        }
    }
}

void newGame() {
    resetGame();
    printField();
    sweep();
}

void resumeGame() {
    if (!game.game_is_active)
    {
        return;
    }
    clear();
    printField();
    setCursor();
    curs_set(2);
    sweep();
}

void highlightActiveSettingsOption(short option) {
    curs_set(0);

    mvaddch(1, 16, ' ');
    if (game.difficulty != normal)
    {
        mvaddch(1, 23, ' '); // Difficulty
    }
    mvaddch(1, 25, ' ');

    mvaddch(2, 7, ' ');
    mvaddch(2, 12, ' '); // Back

    switch (option)
    {
    case 0:
        mvaddch(option + 1, 16, '<');
        if (game.difficulty == normal)
        {
            mvaddch(option + 1, 25, '>');
        }
        else
        {
            mvaddch(option + 1, 23, '>');
        }
        break;
    default:
        mvaddch(option + 1, 7, '<');
        mvaddch(option + 1, 12, '>');
        break;
    }
    refresh();
}

void printSettingsMenu() {
    clear();
    addstr("      Settings:\n");
    addstr("Difficulty\t [");
    if (game.difficulty == easy)
    {
        addstr("Easy");
    }
    else if (game.difficulty == normal)
    {
        addstr("Normal");
    }
    else if (game.difficulty == hard)
    {
        addstr("Hard");
    }
    addstr("] \n");
    addstr("\tBack");
    highlightActiveSettingsOption(0);
}

void settingsChange(short option, char direction) {
    curs_set(0);
    switch (option)
    {
    case 0:
        if (direction == 'L')
        {
            if (game.difficulty == 0) // Easy
            {
                game.difficulty = 2; // Hard
            }
            else
            {
                game.difficulty--; // Decrease
            }
        }
        else
        {
            if (game.difficulty == 2) // Hard
            {
                game.difficulty = 0; // Easy
            }
            else
            {
                game.difficulty++; // Increase
            }
        }        
        if (game.difficulty == normal)
        {
            mvaddstr(option + 1, 16, "<[Normal]>");
        }
        else
        {
            mvaddstr(option + 1, 16, "          ");
            mvaddstr(option + 1, 16, "<[    ]>");
            if (game.difficulty == easy)
            {
                mvaddstr(option + 1, 18, "Easy");
            }
            else
            {
                mvaddstr(option + 1, 18, "Hard");
            }
        }
        break;
    default:
        break;
    }
}

bool settings() {
    Difficulty tmp_difficulty = game.difficulty;
    curs_set(0);
    printSettingsMenu();
    short option = 0;
    while (true)
    {
        short pressed = wgetch(win);
        switch (pressed)
        {
        case 's':
        case 'S':
        case KEY_DOWN:
            if (option == 1)
            {
                option = 0;
            }
            else
            {
                option++;
            }
            highlightActiveSettingsOption(option);
            break;
        case 'w':
        case 'W':
        case KEY_UP:
            if (option == 0)
            {
                option = 1;
            }
            else
            {
                option--;
            }
            highlightActiveSettingsOption(option);
            break;
        case 'a':
        case 'A':
        case KEY_LEFT:
            settingsChange(option, 'L');
            break;
        case 'd':
        case 'D':
        case KEY_RIGHT:
            settingsChange(option, 'R');
            break;
        case ' ': // Space bar
        case 10: // Enter
            if (option == 1)
            {
                goto exit_settings;
            }
            break;
        case 27: // ESC
            goto exit_settings;
            break;
        default:
            break;
        }
    }
exit_settings:
    if (tmp_difficulty != game.difficulty)
    {
        game.game_is_active = false;
        return true;
    }
    else
    {
        return false;
    }
}

bool mainMenu() {
    curs_set(0);
    printMenu();
    highlightActiveOption(0);
    short option = 0;
    while (true)
    {
        short pressed = wgetch(win);
        switch (pressed)
        {
            case 's':
            case 'S':
            case KEY_DOWN:
                if (game.game_is_active)
                {
                    if (option == 3)
                    {
                        option = 0;
                    }
                    else
                    {
                        option++;
                    }
                }
                else
                {
                    if (option == 2)
                    {
                        option = 0;
                    }
                    else
                    {
                        option++;
                    }
                }
                highlightActiveOption(option);
                break;
            case 'w':
            case 'W':
            case KEY_UP:
                if (game.game_is_active)
                {
                    if (option == 0)
                    {
                        option = 3;
                    }
                    else
                    {
                        option--;
                    }
                }
                else
                {
                    if (option == 0)
                    {
                        option = 2;
                    }
                    else
                    {
                        option--;
                    }
                }
                highlightActiveOption(option);
                break;
            case 27: // ESC
                if (game.game_is_active)
                {
                    resumeGame();
                    printMenu();
                    highlightActiveOption(0);
                    option = 0;
                    curs_set(0);
                }
                break;
            case ' ':               // Space bar
            case 10:                // Enter
                if (game.game_is_active)
                {
                    switch (option)
                    {
                    case 0: // Resume
                        resumeGame();
                        printMenu();
                        highlightActiveOption(0);
                        option = 0;
                        curs_set(0);
                        break;
                    case 1: // New game
                        curs_set(2);
                        newGame();
                        printMenu();
                        highlightActiveOption(0);
                        option = 0;
                        curs_set(0);
                        break;
                    case 2: // Settings
                        settings();
                        printMenu();
                        highlightActiveOption(0);
                        option = 0;
                        break;
                    case 3: // Exit
                        return true;
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    switch (option)
                    {
                    case 0: // New game
                        curs_set(2);
                        newGame();
                        printMenu();
                        highlightActiveOption(0);
                        option = 0;
                        curs_set(0);
                        break;
                    case 1: // Settings
                        settings();
                        printMenu();
                        highlightActiveOption(0);
                        option = 0;
                        break;
                    case 2: // Exit
                        return true;
                        break;
                    default:
                        break;
                    }
                    break;
                }
                break;
            default:
                break;
        }
    }
}

void initWindow() {
    win = initscr();                            // Screen initialization
    noecho();                                   // Switch off echo
    cbreak();                                   // Disable inline buffering. Control characters are interpreted as any other character by the terminal driver
    curs_set(2);                                // Set blinking cursor
    keypad(win, true);                          // Enable reading of function keys
    // Color initialization // 
    if (can_change_color())                     // Check whether the terminal has the capability of changing color.
    {
        start_color();                          // Needed to be called before using colors
        init_pair(1, COLOR_WHITE, COLOR_GREEN); // White foreground, green background
        init_pair(2, COLOR_WHITE, COLOR_RED);   // White foreground, red background
    }
}

void main(void) {
    initWindow();
    game.difficulty = normal;
    if (mainMenu())
    {
        endwin();                               // Ends the terminal cursor mode
        exit(0);                                // Exit
    }
}