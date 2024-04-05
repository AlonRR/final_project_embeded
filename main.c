/*
 * File:   LabC5.c
 * Author: Amit
 *
 * Created on May 21, 2022
 * Fixed Jan 2023
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "System/system.h"
#include "System/delay.h"
#include "oledDriver/oledC.h"
#include "oledDriver/oledC_colors.h"
#include "oledDriver/oledC_shapes.h"

#include "i2cDriver/i2c1_driver.h"
#include "Accel_i2c.h"
// #include "snake/snake_globals.h"

#define MAX_SNAKE_SIZE 120

#define RED OLEDC_COLOR_RED;
#define GREEN OLEDC_COLOR_GREEN
#define BLUE OLEDC_COLOR_BLUE
#define YELLOW OLEDC_COLOR_YELLOW
#define C_GREEN OLEDC_COLOR_PALEGREEN
#define C_RED OLEDC_COLOR_TOMATO
#define BOARD_Y 96 // logical board height size
#define BOARD_X 48 // logical board width size

typedef struct coordinates_t
{
    int x;
    int y;
} coordinates;

enum board_options
{
    WALL = -1,
    EMPTY = 0,
    SNAKE = 1,
    CHARM_GREEN = 2, // increase snake length
    CHARM_RED = 3,   // decrease snake length
};

enum directions
{
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3,
};

typedef struct snake_body_t
{
    coordinates body_position; // x, y
    uint16_t color;
} snake_body_s;

typedef struct snake_t
{
    int snake_length;
    snake_body_s snake_body[MAX_SNAKE_SIZE];
} snake_s;

typedef struct charm_t
{
    coordinates charm_position; // x, y
} charm_s;

int game_board[BOARD_X][BOARD_Y]; // 48x96 board *** 2x2 px per cell
snake_s snake;                    // snake is 2px width 4px length per body piece
charm_s charm;
enum directions direction; // current direction of the snake
int score;
bool game_over;
int pot_off_set;

void initialize_game(void);
void clearBoard(void);
void direction_diff(int *x, int *y);
void find_direction(void);
void move_snake(void);
bool check_wall_collision(void);
enum board_options check_hit_something(void);
void snake_game_options(void);
void random_available_board_position(coordinates *position);
void random_number(int *number, int max);

void random_number(int *number, int max)
{
    unsigned char seed[6] = {0};
    i2cReadSlaveMultRegister(0x3A, 0x32, 6, seed);
    srand(seed);
    *number = rand() % max;
}

// random available board position for charms
void random_available_board_position(coordinates *position)
{
    int x, y;
    while (1)
    {
        random_number(&x, BOARD_X);
        random_number(&y, BOARD_Y);
        if (game_board[x][y] == EMPTY)
        {
            break;
        }
    }
    position->x = x;
    position->y = y;
}

void find_direction(void)
{
    int x, y, ux, uy; // x, y, unsigned x, unsigned y
    unsigned char xyz[6] = {0};
    i2cReadSlaveMultRegister(0x3A, 0x32, 6, xyz);

    x = xyz[0] + xyz[1] * 256; // 2xbytes ==> word
    y = xyz[2] + xyz[3] * 256;
    ux = x & ~(1 << 15); // clear sign bit
    uy = y & ~(1 << 15);
    if (ux > uy)
    {
        if (x > 0)
        {
            direction = RIGHT;
        }
        else
        {
            direction = LEFT;
        }
    }
    else
    {
        if (y > 0)
        {
            direction = DOWN;
        }
        else
        {
            direction = UP;
        }
    }
}

void initialize_game(void)
{
    clearBoard();
    snake.snake_length = 4;
    snake.snake_body[0].body_position.x = BOARD_X / 2; // snake starting position is the middle of the board
    snake.snake_body[0].body_position.y = BOARD_Y / 2;
    snake.snake_body[0].color = RED;

    move_snake();
}

void clearBoard(void)
{
    for (int i = 0; i < BOARD_X; i++)
    {
        for (int j = 0; j < BOARD_Y; j++)
        {
            game_board[i][j] = EMPTY;
        }
    }
}

void direction_diff(int *x, int *y)
{
    switch (direction)
    {
    case UP:
        *x = -1;
        break;
    case DOWN:
        *x = 1;
        break;
    case LEFT:
        *y = -1;
        break;
    case RIGHT:
        *y = 1;
        break;
    default:
        break;
    }
}

// snake move function
void move_snake(void)
{
    int add_x = 0;
    int add_y = 0;
    direction_diff(direction, &add_x, &add_y);

    // moves the snake body
    for (int i = 1; i < snake.snake_length; i++)
    {
        snake.snake_body[i].body_position.x = snake.snake_body[i - 1].body_position.x;
        snake.snake_body[i].body_position.y = snake.snake_body[i - 1].body_position.y;
    }

    // moves the snake head
    snake.snake_body[0].body_position.x += add_x;
    snake.snake_body[0].body_position.y += add_y;
}

// logical function to check if the snake has hit the wall
//  returns true if it has hit the wall
bool check_wall_collision(void)
{
    int add_x = 0, add_y = 0;
    direction_diff(direction, &add_x, &add_y);
    if (snake.snake_body[0].body_position.x + add_x < 0 ||
        snake.snake_body[0].body_position.x + add_x >= BOARD_X ||
        snake.snake_body[0].body_position.y + add_y < 0 ||
        snake.snake_body[0].body_position.y + add_y >= BOARD_Y)
    {
        return true;
    }
}

// logical function to check if the snake has hit something
// returns enum board_options
enum board_options check_hit_something(void)
{
    int add_x = 0, add_y = 0;
    enum board_options hit = EMPTY;
    direction_diff(direction, &add_x, &add_y);
    if (check_wall_collision(direction))
    {
        hit = WALL;
    }
    else
    {
        hit = game_board[snake.snake_body[0].body_position.x + add_x][snake.snake_body[0].body_position.y + add_y];
    }
    return hit;
}

void snake_game_options()
{
    enum board_options hit = check_hit_something();
    switch (hit)
    {
    case WALL:
    case SNAKE:
        // game_over = true; /// game over???
        break;
    case EMPTY:
        move_snake();
        break;
    case CHARM_GREEN:
        snake.snake_length++;
        move_snake();
        break;
    case CHARM_RED:
        snake.snake_length--;
        move_snake();
        break;
    default:
        break;
    }
}

void view_game(void)
{
    for (int i = 0; i < snake.snake_length; i++)
    {
        oledC_DrawRectangle(snake.snake_body[i].body_position.x * 2 + pot_off_set,
                            snake.snake_body[i].body_position.y * 2,
                            snake.snake_body[i].body_position.x * 2 + 2 + pot_off_set,
                            snake.snake_body[i].body_position.y * 2 + 2,
                            snake.snake_body[i].color);
    }
}

// void placeSnake(void)
// {
//     for (int i = 0; i < snake.snake_length; i++)
//     {
//         game_board[snake.snake_head_position[0] + i][snake.snake_head_position[1]] = SNAKE;
//     }
//     snake.snake_length = 1;
//     snake.snake_head_position[0] = 96 / 2;
//     snake.snake_head_position[1] = 192 / 2;

//     game_board[snake.snake_head_position[0]][snake.snake_head_position[1]] = SNAKE;
// }

void errorStop(char *msg)
{
    oledC_DrawString(0, 20, 2, 2, msg, OLEDC_COLOR_DARKRED);

    for (;;)
        ;
}

void initialize_micro_chip(void)
{

    SYSTEM_Initialize();
}

/*
                         Main application
 */
int main(void)
{
    // initialize game board with 0
    clearBoard();

    // ignore
    unsigned char id = 0;
    I2Cerror rc;
    int x, y, z;
    char xx[] = "     ";
    char yy[] = "     ";
    char zz[] = "     ";
    unsigned char xyz[6] = {0};

    oledC_setBackground(OLEDC_COLOR_SKYBLUE);
    oledC_clearScreen();

    i2c1_driver_driver_close();
    i2c1_open();

    rc = i2cReadSlaveRegister(0x3A, 0, &id);

    if (rc == OK)
        if (id == 0xE5)
            oledC_DrawString(10, 10, 2, 2, "ADXL345", OLEDC_COLOR_BLACK);
        else
            errorStop("Acc!Found");
    else
        errorStop("I2C Error");

    rc = i2cWriteSlave(0x3A, 0x2D, 8);

    oledC_DrawString(2, 30, 2, 2, "X:", OLEDC_COLOR_BLACK);
    oledC_DrawString(2, 50, 2, 2, "Y:", OLEDC_COLOR_BLACK);
    oledC_DrawString(2, 70, 2, 2, "Z:", OLEDC_COLOR_BLACK);

    for (;;)
    {
        int i;

        i2cReadSlaveMultRegister(0x3A, 0x32, 6, xyz);

        x = xyz[0] + xyz[1] * 256; // 2xbytes ==> word
        y = xyz[2] + xyz[3] * 256;
        z = xyz[4] + xyz[5] * 256;

        sprintf(xx, "%d", x); // Make it a string
        sprintf(yy, "%d", y);
        sprintf(zz, "%d", z);

        //  === Display Axes Acceleration   ====================
        oledC_DrawString(26, 30, 2, 2, xx, OLEDC_COLOR_BLACK);
        oledC_DrawString(26, 50, 2, 2, yy, OLEDC_COLOR_BLACK);
        oledC_DrawString(26, 70, 2, 2, zz, OLEDC_COLOR_BLACK);
        DELAY_milliseconds(500);

        //  === Erase Axes Acceleration   ====================
        oledC_DrawString(26, 30, 2, 2, xx, OLEDC_COLOR_SKYBLUE);
        oledC_DrawString(26, 50, 2, 2, yy, OLEDC_COLOR_SKYBLUE);
        oledC_DrawString(26, 70, 2, 2, zz, OLEDC_COLOR_SKYBLUE);
    }
}
/**
 End of File
*/