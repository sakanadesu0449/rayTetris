// Author: sakanadesu0449

#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BG_COLOR (Color){100, 100, 100, 255}

#define WINDOW_X 1000
#define WINDOW_Y 1080
#define BLOCK_COUNT_X 10
#define BLOCK_COUNT_Y 22
#define BLOCK_LENGTH 40
#define BLOCK_MARGIN 4

#define SHAPE_DATA_BUDR(__name, __color, __rotation, ...)                      \
    {                                                                          \
        .zeropoint_x = 4,                                                      \
        .zeropoint_y = 2,                                                      \
        .shape = {{0, 0}, __VA_ARGS__},                                        \
        .color = __color,                                                      \
        .name = __name,                                                        \
        .rotation = __rotation,                                                \
    }

#define DEFAULT_SHAPE_DATA(_n, _c, ...)                                        \
    SHAPE_DATA_BUDR((_n), (_c), ALPHA, ##__VA_ARGS__)

#define CHECK_OPT(n) IsKeyDown(n) && (op_inv == 0)
#define FREE_SHAPE(s)                                                          \
    do {                                                                       \
        free(s);                                                               \
        (s) = NULL;                                                            \
    } while (0)

// WARN: this macro returns ptr
// and u definitely can modify it directly
#define OFFSET_TO_BOARD(i, board, s, x, y)                                     \
    &(board)[(x) + (s)->shape[(i)][0]][(y) + (s)->shape[(i)][1]]

#define SET_EACH_BLOCK(_board, _s, _x, _y, _v, _stat)                          \
    do {                                                                       \
        for (int _i = 0; _i < 4; _i++) {                                       \
            block * _block = OFFSET_TO_BOARD(_i, (_board), (_s), (_x), (_y));  \
            _block->b = (_v);                                                  \
            _block->color = (_s)->color;                                       \
            _block->status = (_stat);                                          \
        }                                                                      \
    } while (0)

typedef unsigned int ingame_time;
typedef unsigned int level;
typedef enum {
    ACTIVE,
    FREEZE,
} block_status;

typedef enum {
    UP,
    DOWN,
    LEFT,
    RIGHT,
} movement;

typedef enum {
    ORANGE_RICKY,
    //      --#-
    //      ###-
    BLUE_RICKY,
    //      #---
    //      ###-
    CLEVELAND_Z,
    //      ##--
    //      -##-
    RHODE_ISLAND_Z,
    //      -##-
    //      ##--
    HERO,
    //      ----
    //      ####
    TEEWEE,
    //      -#--
    //      ###-
    SMASHBOY,
    //      ##--
    //      ##--
} shape_name;

typedef enum { ALPHA, BETA, GAMMA, DELTA } shape_rotation;

//
//  shapes' data format should using OFFSET
//  in clockwise and spiral order
//
//  case:
//      shape = (shape) {
//          {0, 0}, {0, 1}, {-1, 0}, {-1, -1}
//      };
//
//  will be:
//  [-][#][-][-]
//  [-][#][@][-]
//  [-][-][#][-]
//
typedef struct {
    int zeropoint_x;
    int zeropoint_y;
    int shape[4][2];
    shape_name name;
    shape_rotation rotation;
    Color color;
} moving_shape;

// shapes' initial state
static const moving_shape new_shapes[7] = {
    DEFAULT_SHAPE_DATA(ORANGE_RICKY, MAROON, {1, -1}, {1, 0}, {-1, 0}),
    DEFAULT_SHAPE_DATA(BLUE_RICKY, ORANGE, {1, 0}, {-1, 0}, {-1, -1}),
    DEFAULT_SHAPE_DATA(CLEVELAND_Z, DARKBLUE, {1, 1}, {0, 1}, {-1, 0}),
    DEFAULT_SHAPE_DATA(RHODE_ISLAND_Z, DARKBLUE, {1, -1}, {-1, 0}, {0, -1}),
    DEFAULT_SHAPE_DATA(HERO, LIME, {1, 0}, {2, 0}, {-1, 0}),
    DEFAULT_SHAPE_DATA(TEEWEE, DARKPURPLE, {1, 0}, {-1, 0}, {0, -1}),
    DEFAULT_SHAPE_DATA(SMASHBOY, DARKBROWN, {0, -1}, {-1, 0}, {-1, -1})};

static const moving_shape shape_rotations[7][4] = {
    // ORANGE_RICKY
    {new_shapes[0],
     SHAPE_DATA_BUDR(ORANGE_RICKY, MAROON, BETA, {1, 1}, {0, 1}, {0, -1}),
     SHAPE_DATA_BUDR(ORANGE_RICKY, MAROON, GAMMA, {1, 0}, {-1, 1}, {-1, 0}),
     SHAPE_DATA_BUDR(ORANGE_RICKY, MAROON, DELTA, {0, 1}, {-1, -1}, {0, -1})},
    // BLUE_RICKY
    {new_shapes[1],
     SHAPE_DATA_BUDR(BLUE_RICKY, ORANGE, BETA, {0, -1}, {1, -1}, {0, 1}),
     SHAPE_DATA_BUDR(BLUE_RICKY, ORANGE, GAMMA, {1, 0}, {1, 1}, {-1, 0}),
     SHAPE_DATA_BUDR(BLUE_RICKY, ORANGE, DELTA, {0, 1}, {-1, 1}, {0, -1})},
    // CLEVELAND_Z
    {new_shapes[2],
     SHAPE_DATA_BUDR(CLEVELAND_Z, DARKBLUE, BETA, {0, -1}, {-1, 1}, {-1, 0}),
     SHAPE_DATA_BUDR(CLEVELAND_Z, DARKBLUE, GAMMA, {1, 1}, {0, 1}, {-1, 0}),
     SHAPE_DATA_BUDR(CLEVELAND_Z, DARKBLUE, DELTA, {0, -1}, {-1, 1}, {-1, 0})},
    // RHODE_ISLAND_Z
    {new_shapes[3],
     SHAPE_DATA_BUDR(RHODE_ISLAND_Z, DARKBLUE, BETA, {0, 1}, {-1, 0}, {-1, -1}),
     SHAPE_DATA_BUDR(RHODE_ISLAND_Z, DARKBLUE, GAMMA, {1, -1}, {-1, 0}, {0, -1}),
     SHAPE_DATA_BUDR(RHODE_ISLAND_Z, DARKBLUE, DELTA, {0, 1}, {-1, 0}, {-1, -1})},
    // HERO
    {new_shapes[4], SHAPE_DATA_BUDR(HERO, LIME, BETA, {0, -1}, {0, 1}, {0, 2}),
     SHAPE_DATA_BUDR(HERO, LIME, GAMMA, {1, 0}, {2, 0}, {-1, 0}),
     SHAPE_DATA_BUDR(HERO, LIME, DELTA, {0, -1}, {0, 1}, {0, 2})},
    // TEEWEE
    {new_shapes[5],
     SHAPE_DATA_BUDR(TEEWEE, DARKPURPLE, BETA, {0, -1}, {1, 0}, {0, 1}),
     SHAPE_DATA_BUDR(TEEWEE, DARKPURPLE, GAMMA, {1, 0}, {0, 1}, {-1, 0}),
     SHAPE_DATA_BUDR(TEEWEE, DARKPURPLE, DELTA, {0, -1}, {0, 1}, {-1, 0})},
    // SMASHBOY
    {new_shapes[6],
     SHAPE_DATA_BUDR(SMASHBOY, DARKBROWN, BETA, {0, -1}, {-1, 0}, {-1, -1}),
     SHAPE_DATA_BUDR(SMASHBOY, DARKBROWN, GAMMA, {0, -1}, {-1, 0}, {-1, -1}),
     SHAPE_DATA_BUDR(SMASHBOY, DARKBROWN, DELTA, {0, -1}, {-1, 0}, {-1, -1})}};

static const int levels_ft[10] = {39, 36, 33, 30, 27, 24, 21, 18, 15, 12};

//  board data finally given to render
typedef struct {
    char b;
    block_status status;
    Color color;
} block;

typedef struct {
    int line[8];
    int line_count;
} full_line;

// declarations
void draw_gamezone(int width, int height, char * score, char * lines,
                   char * level);
void draw_blocks(block ** board,
                 int blocks_pos[BLOCK_COUNT_X][BLOCK_COUNT_Y][2], int length);
int update_board(block ** board, moving_shape * shape, movement move);
full_line check_full_line(block ** board);
moving_shape * get_next_shape_rotation(moving_shape * shape);
void clear_line(block ** board, full_line line);
void add_new_block(block ** board);
moving_shape * add_new_shape();
moving_shape * free_moving_shape_m(moving_shape * shape);
int is_shape_moveable(block ** board, moving_shape * shape, movement move);
block ** init_board();

//
// definitions
void draw_gamezone(int width, int height, char * score, char * lines,
                   char * level) {
    DrawRectangleLinesEx(
        (Rectangle){WINDOW_X * 0.05, WINDOW_Y * 0.05,
                    BLOCK_COUNT_X * (BLOCK_LENGTH + BLOCK_MARGIN * 2) +
                        BLOCK_MARGIN * 2,
                    (BLOCK_COUNT_Y - 2) * (BLOCK_LENGTH + BLOCK_MARGIN * 2) +
                        BLOCK_MARGIN * 2},
        5, BLACK);
    int diagonal_length = sqrt(width * width + height * height);
    DrawText(score, width * 0.60, height * 0.10, 65, BLACK);
    DrawText(level, width * 0.65, height * 0.25, 50, BLACK);
    DrawText(lines, width * 0.65, height * 0.30, 50, BLACK);
    DrawText("Hold DOWN to\nspeed up", width * 0.55, height * 0.65, 45, BLACK);
    DrawText("LEFT/RIGHT\nto control", width * 0.55, height * 0.75, 45, BLACK);
    DrawText("UP to rotate", width * 0.55, height * 0.85, 45, BLACK);
    DrawText("ESC to quit", width * 0.55, height * 0.90, 45, BLACK);
}

void draw_blocks(block ** board,
                 int blocks_pos[BLOCK_COUNT_X][BLOCK_COUNT_Y][2], int length) {
    for (int x = 0; x < BLOCK_COUNT_X; x++) {
        for (int y = 2; y < BLOCK_COUNT_Y; y++) {
            if (board[x][y].b == '#') {
                DrawRectangle(blocks_pos[x][y][0], blocks_pos[x][y][1], length,
                              length, board[x][y].color);
                DrawRectangleLinesEx((Rectangle){blocks_pos[x][y][0], blocks_pos[x][y][1], length,
                              length}, 3, BLACK);
            } else {
                continue;
            }
        }
    }
}

//
// return 1 when a shape cannot move (frozen)
int update_board(block ** board, moving_shape * shape, movement move) {
    int current_x = shape->zeropoint_x;
    int current_y = shape->zeropoint_y;
    switch (move) {
    case UP: {
        if (is_shape_moveable(board, shape, UP)) {
            SET_EACH_BLOCK(board, shape, current_x, current_y, '-', ACTIVE);
            moving_shape * _shape = get_next_shape_rotation(shape);
            memcpy(shape->shape, _shape->shape, sizeof(_shape->shape));
            shape->rotation = _shape->rotation;
            SET_EACH_BLOCK(board, shape, current_x, current_y, '#', ACTIVE);
            FREE_SHAPE(_shape);
        }
        break;
    }
    case LEFT:
        if (is_shape_moveable(board, shape, LEFT)) {
            SET_EACH_BLOCK(board, shape, current_x, current_y, '-', ACTIVE);
            current_x--;
            SET_EACH_BLOCK(board, shape, current_x, current_y, '#', ACTIVE);
            shape->zeropoint_x--;
        }
        break;
    case RIGHT:
        if (is_shape_moveable(board, shape, RIGHT)) {
            SET_EACH_BLOCK(board, shape, current_x, current_y, '-', ACTIVE);
            current_x++;
            SET_EACH_BLOCK(board, shape, current_x, current_y, '#', ACTIVE);
            shape->zeropoint_x++;
        }
        break;
    default:
        if (is_shape_moveable(board, shape, DOWN)) {

            SET_EACH_BLOCK(board, shape, current_x, current_y, '-', ACTIVE);
            current_y++;
            SET_EACH_BLOCK(board, shape, current_x, current_y, '#', ACTIVE);
            shape->zeropoint_y++;
            break;
        } else {
            SET_EACH_BLOCK(board, shape, current_x, current_y, '#', FREEZE);
            // printf("a shape has been frozen!\n");
            return 1;
        }
        break;
    }
    return 0;
}

moving_shape * get_next_shape_rotation(moving_shape * shape) {
    moving_shape * next_shape = malloc(sizeof(moving_shape));
    shape_rotation rotation = ALPHA;
    if (shape->rotation == DELTA) {
        rotation = ALPHA;
    } else {
        rotation = shape->rotation;
        rotation++;
    }
    memcpy(next_shape, &shape_rotations[shape->name][rotation],
           sizeof(shape_rotations[shape->name][rotation]));
    next_shape->zeropoint_x = shape->zeropoint_x;
    next_shape->zeropoint_y = shape->zeropoint_y;
    next_shape->rotation = rotation;
    return next_shape;
}

// check board if ok then return 1
int is_shape_moveable(block ** board, moving_shape * shape, movement move) {
    switch (move) {
    case DOWN: {
        for (int i = 0; i < 4; i++) {
            if ((shape->zeropoint_y + shape->shape[i][1] + 1) >= BLOCK_COUNT_Y)
                return 0;
            block * block = OFFSET_TO_BOARD(i, board, shape, shape->zeropoint_x,
                                            shape->zeropoint_y + 1);
            if (block->b == '#' && block->status == FREEZE)
                return 0;
        }
        break;
    }
    case LEFT: {
        for (int i = 0; i < 4; i++) {
            if ((shape->zeropoint_x + shape->shape[i][0] - 1) < 0)
                return 0;
            block * block = OFFSET_TO_BOARD(
                i, board, shape, shape->zeropoint_x - 1, shape->zeropoint_y);
            if (block->b == '#' && block->status == FREEZE)
                return 0;
        }
        break;
    }
    case RIGHT: {
        for (int i = 0; i < 4; i++) {
            if ((shape->zeropoint_x + shape->shape[i][0] + 1) >= BLOCK_COUNT_X)
                return 0;
            block * block = OFFSET_TO_BOARD(
                i, board, shape, shape->zeropoint_x + 1, shape->zeropoint_y);
            if (block->b == '#' && block->status == FREEZE)
                return 0;
        }
        break;
    }
    case UP: {
        moving_shape * _next_shape = get_next_shape_rotation(shape);
        for (int i = 0; i < 4; i++) {
            int _x = _next_shape->zeropoint_x + _next_shape->shape[i][0];
            int _y = _next_shape->zeropoint_y + _next_shape->shape[i][1];
            if (_x > BLOCK_COUNT_X || _x < 0 || _y < 0 || _y > BLOCK_COUNT_Y) {
                // printf("rotation out of bound\n");
                FREE_SHAPE(_next_shape);
                return 0;
            }
            block * block =
                OFFSET_TO_BOARD(i, board, _next_shape, _next_shape->zeropoint_x,
                                _next_shape->zeropoint_y);
            if (block->b == '#' && block->status == FREEZE) {
                // printf("cannot rotation\n");
                FREE_SHAPE(_next_shape);
                return 0;
            }
        }
        FREE_SHAPE(_next_shape);
        break;
    }
    }
    return 1;
}

full_line check_full_line(block ** board) {
    int lines = 0;
    full_line line = {.line = {0}, .line_count = 0};

    for (int y = BLOCK_COUNT_Y - 1; y >= 0; y--) {
        int block_count = 0;
        for (int x = 0; x < BLOCK_COUNT_X; x++) {
            if (board[x][y].b == '#' && board[x][y].status == FREEZE)
                block_count++;
        }
        if (block_count == BLOCK_COUNT_X) {
            line.line[lines] = y;
            lines++;
            line.line_count = lines;
        }
    }

    return line;
}

void clear_line(block ** board, full_line line) {
    int l = 0;
    for (int i = 0; i < line.line_count; i++) {
        int y = line.line[i];
        // printf("cleaning line %d\n", y);
        y += l;
        for (int x = 0; x < BLOCK_COUNT_X; x++) {
            board[x][y].b = '-';
            board[x][y].status = ACTIVE;
        }
        for (int _y = y - 1; _y >= 0; _y--) {
            // printf("moving line %d\n", _y);
            for (int x = 0; x < BLOCK_COUNT_X; x++) {
                board[x][_y + 1].b = board[x][_y].b;
                board[x][_y + 1].status = board[x][_y].status;
                board[x][_y + 1].color = board[x][_y].color;
            }
        }

        for (int x = 0; x < BLOCK_COUNT_X; x++) {
            board[x][0].b = '_';
            board[x][0].status = ACTIVE;
        }
        l++;
    }
    printf("INFO: %d lines cleaerd\n", line.line_count);
}

block ** init_board() {
    block ** board = malloc(BLOCK_COUNT_Y * sizeof(block *));
    for (int x = 0; x < BLOCK_COUNT_X; x++) {
        board[x] = malloc(BLOCK_COUNT_Y * sizeof(block));
        for (int y = 0; y < BLOCK_COUNT_Y; y++) {
            board[x][y].b = '-';
        }
        // printf("\n");
    }
    return board;
}

moving_shape * add_new_shape() {
    shape_name shape_name = rand() % 7;
    // shape_name shape_name = 1;
    moving_shape * shape = malloc(sizeof(moving_shape));
    memcpy(shape, &new_shapes[shape_name], sizeof(new_shapes[shape_name]));
    // printf("new shape: %d\n", shape_name);
    return shape;
}

int main() {

    srand(time(NULL));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_X, WINDOW_Y, "rayTetris");
    SetTargetFPS(60);
    char size_info[64];
    char score_info[32];
    char level_info[32];
    char lines_info[32];

    ingame_time igt = 0;        // ingame timer
    level level = 0;
    const float frametime = 0.0167f;
    int cleaMAROON_lines = 0;
    int level_up_ = 10;

    // blocks pos init
    int g_x = WINDOW_X * 0.05;
    int g_y = WINDOW_Y * 0.05;
    int blocks_pos[BLOCK_COUNT_X][BLOCK_COUNT_Y][2];
    for (int x = 0; x < BLOCK_COUNT_X; x++) {
        for (int y = 0; y < BLOCK_COUNT_Y; y++) {
            int pos_x = g_x + BLOCK_MARGIN * 2 * (x + 1) + BLOCK_LENGTH * x;
            int pos_y =
                g_y + BLOCK_MARGIN * 2 * (y - 1) + BLOCK_LENGTH * (y - 2);
            blocks_pos[x][y][0] = pos_x;
            blocks_pos[x][y][1] = pos_y;
        }
    }

    block ** gameboard = init_board();
    moving_shape * shape = NULL;
    ingame_time op_inv = 0;
    int score = 0;

    printf("=============================\n");
    printf("     welcome to Tetris!\n");
    printf("=============================\n");

    //
    //
    // main loop
    while (!WindowShouldClose()) {
        // printf("frame %d\n", igt);
        if (igt > 0)
            igt--;
        if (op_inv > 0)
            op_inv--;

        int window_width = GetRenderWidth();
        int window_height = GetRenderHeight();
        snprintf(size_info, sizeof(size_info), "rayTetris %dx%d", window_width,
                 window_height);
        SetWindowTitle(size_info);

        if (!shape) {
            // printf("generating a new shape!\n");
            shape = malloc(sizeof(moving_shape));
            shape = add_new_shape();
        }
        int result = 0;
        if (op_inv == 0) {
            if (CHECK_OPT(KEY_UP)) {
                result = update_board(gameboard, shape, UP);
                op_inv = 10;
            } else if (CHECK_OPT(KEY_LEFT)) {
                result = update_board(gameboard, shape, LEFT);
                op_inv = 6;
            } else if (CHECK_OPT(KEY_RIGHT)) {
                result = update_board(gameboard, shape, RIGHT);
                op_inv = 6;
            }
        }

        if (igt == 0) {
            result = update_board(gameboard, shape, DOWN);
            igt = levels_ft[level];
        }
        if (CHECK_OPT(KEY_DOWN)) {
            result = update_board(gameboard, shape, DOWN);
            igt = 8;
        }
        if (result == 1) {
            FREE_SHAPE(shape);
            full_line line = check_full_line(gameboard);
            if (line.line_count > 0) {
                clear_line(gameboard, line);
                cleaMAROON_lines += line.line_count;
                score += line.line_count * 100;
                if (level < 9 && level_up_ == 10) {
                    level++;
                    level_up_ = 0;
                }
                level_up_++;
            }
        }

        BeginDrawing();
        ClearBackground(BG_COLOR);
        snprintf(score_info, sizeof(score_info), "SCORE: %d", score);
        snprintf(level_info, sizeof(level_info), "LEVEL: %d", level);
        snprintf(lines_info, sizeof(lines_info), "LINES: %d", cleaMAROON_lines);
        draw_gamezone(window_width, window_height, score_info, lines_info,
                      level_info);
        draw_blocks(gameboard, blocks_pos, BLOCK_LENGTH);
        EndDrawing();
    }
    printf("=============================\n");
    printf("     quit game...\n");
    printf("=============================\n");
    CloseWindow();

    return 0;
}
