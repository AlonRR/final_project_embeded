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

// make it a struct? maybe???
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
void view_game(void);
void errorStop(char *msg);
void potentiometer_initilize(void);
void accelerometer_error_handler(I2Cerror status);
void accelerometer_initialize(void);
void initialize_micro_chip(void);

void random_number(int *number, int max)
{
    unsigned char seed[6] = {0};
    accelerometer_error_handler(i2cReadSlaveMultRegister(0x3A, 0x32, 6, seed));
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
    for (int i = 0; i <= 4; i++){
        set_snake_color();
        snake.snake_length = i;
    }
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
    direction_diff(&add_x, &add_y);

    // moves the snake body
    for (int i = 1; i < snake.snake_length; i++)
    {
        snake.snake_body[i].body_position.x = snake.snake_body[i - 1].body_position.x;
        snake.snake_body[i].body_position.y = snake.snake_body[i - 1].body_position.y;
        snake.snake_body[i].color = snake.snake_body[i - 1].color;
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
    direction_diff(&add_x, &add_y);
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
    direction_diff(&add_x, &add_y);
    if (check_wall_collision())
    {
        hit = WALL;
    }
    else
    {
        hit = game_board[snake.snake_body[0].body_position.x + add_x][snake.snake_body[0].body_position.y + add_y];
    }
    return hit;
}

void set_snake_color(void)
{
    switch ((snake.snake_length - 1) % 3) // snake length - 1 to avoid the head
    {
    case 0:
        snake.snake_body[0].color = GREEN;
        break;
    case 1:
        snake.snake_body[0].color = BLUE;
        break;
    case 2:
        snake.snake_body[0].color = YELLOW;
        break;
    default:
        break;
    }
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
        set_snake_color();
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
    int current_y = 0;
    for (int i = 0; i < snake.snake_length; i++)
    {
        current_y = snake.snake_body[i].body_position.y * 2 + pot_off_set;
        if (current_y <= 96)
            oledC_DrawRectangle(snake.snake_body[i].body_position.x * 2,
                                current_y,
                                snake.snake_body[i].body_position.x * 2 + 2,
                                current_y + 2,
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

// NEEDS CHECKING !!! ???
void read_potentiometer_offset(void)
{
    // Select AN8 for A/D conversion
    AD1CHS = (1 << 4);
    // Perform A/D Conversion
    AD1CON1 |= (1 << 15);          // A/D Converter turned on
    AD1CON1 |= (1 << 1);           // sample
    for (int i = 0; i < 1000; i++) // delay
        ;
    AD1CON1 &= ~(1 << 1);      // stop sampling
    while ((AD1CON1 & 1) == 0) // wait for conversion to complete
        ;
    pot_off_set = (int)(((float)ADC1BUF0 / 1023.0) * 96.0);
}

// ???
void potentiometer_initilize(void)
{
    AD1CON1 = 0;
    AD1CON1 |= (1 << 15); // A/D Converter turned on
    AD1CON2 = 0;
    AD1CON3bits.ADCS = 0xFF;
    AD1CON3bits.SAMC = 0x10;
    AD1CON3bits.ADRC = 0;

    TRISBbits.TRISB12 = 1; // set RB12 as input
    ANSBbits.ANSB12 = 1;   // set RB12 as analog input
    AD1CHS = 8;
}

void accelerometer_error_handler(I2Cerror status)
{
    // potential error handling
    // switch (status)
    // {
    // case OK:
    //     break;
    // case NACK:
    // case ACK:
    // case BAD_ADDR:
    // case BAD_REG:
    //     errorStop("I2C Error");
    // default:
    //     break;
    // }

    if (status != OK)
        errorStop("I2C Error");
}

void accelerometer_initialize(void)
{
    unsigned char id = 0;
    i2c1_driver_driver_close();
    i2c1_open();
    accelerometer_error_handler(i2cReadSlaveRegister(0x3A, 0, &id));
    if (id != 0xE5)
        errorStop("Acc!Found");
    accelerometer_error_handler(i2cWriteSlave(0x3A, 0x2D, 8));
}

void initialize_micro_chip(void)
{
    // OLED & clock initializer
    SYSTEM_Initialize();
    // initialize potentiometer
    potentiometer_initilize();
    // initialize the accelerometer
    accelerometer_initialize();
}

void timer_1_init()
{
    // Timer1 control register
    T1CON = 0;
    T1CONbits.TON = 1;
    T1CONbits.TSIDL = 1;
    T1CONbits.TGATE = 0;
    T1CONbits.TCKPS = 0b10; // 1:64 prescaler
    T1CONbits.TCS = 0;

    // Timer1 interrupt settings and period
    TMR1 = 0;
    PR1 = 0xF424;      // timer period value
    IFS0bits.T1IF = 0; // Clear Timer1 interrupt flag
    IEC0bits.T1IE = 1; // Enable Timer1 interrupt
}

void __attribute__((__interrupt__)) _T1Interrupt(void)
{
    read_potentiometer_offset();
    find_direction();
    snake_game_options();
    view_game();
    IFS0bits.T1IF = 0;
}

void view_initialize(void)
{
    oledC_setBackground(OLEDC_COLOR_BLACK);
    oledC_clearScreen();
    timer_1_init();
}
/*
                         Main application
*/
int main(void)
{
    // initialize the microchip
    initialize_micro_chip();
    // initialize the view screen
    view_initialize();
    // initialize game board with 0
    initialize_game();

    for (;;)
        ;
}
/**
 End of File
*/