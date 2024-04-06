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

#define MAX_SNAKE_SIZE 10
// #define C_GREEN OLEDC_COLOR_PALEGREEN
// #define C_RED OLEDC_COLOR_TOMATO
#define BOARD_Y 96 // logical board height size
#define BOARD_X 48 // logical board width size

typedef struct coordinates_t
{
    uint8_t x;
    uint8_t y;
} coordinates;

enum board_options
{
    WALL = -1,
    EMPTY = 0,
    SNAKE = 1,
    CHARM_GREEN = 2, // increase snake length
    CHARM_RED = 3,   // decrease snake length
    DIDNT_MOVE = 4,
};

enum directions
{
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3,
    NO_MOVE = 4,
};

typedef struct snake_body_t
{
    coordinates body_position; // x, y
    enum directions direction;
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
charm_s charm[8];                 // 8 charms on the board
enum directions direction = DIDNT_MOVE;   // current direction of the snake
int score = 0;
bool game_over = false;
int pot_off_set = 0;

void initialize_game();
void clearBoard();
void direction_diff(int16_t *x, int16_t *y);
void find_direction();
void move_snake();
bool check_wall_collision();
enum board_options check_hit_something();
void snake_game_options();
void random_available_board_position(coordinates *position);
void random_number(int *number, int max);
void view_game();
void errorStop(char *msg);
void potentiometer_initilize();
void accelerometer_error_handler(I2Cerror status);
void accelerometer_initialize();
void initialize_micro_chip();
void set_snake_color();
void erase_snake_color();
void read_potentiometer_offset();
void view_initialize();
void initialize_snake();

void random_number(int *number, int max)
{
    unsigned char seed[6] = {0};
    accelerometer_error_handler(i2cReadSlaveMultRegister(0x3A, 0x32, 6, seed));
    srand((unsigned int)seed);
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
    int x, y;
    unsigned char xyz[6] = {0};
    i2cReadSlaveMultRegister(0x3A, 0x32, 6, xyz);
    // xyz[0] left
    // xyz[1] right
    // xyz[2]
    // xyz[3]
    x = xyz[0] + xyz[1] * 256; // 2xbytes ==> word
    y = xyz[2] + xyz[3] * 256;

    if (xyz[0] > xyz[1])
        x = xyz[0];
    else
        x = xyz[1];

    if (xyz[2] > xyz[3])
        y = xyz[2];
    else
        y = xyz[3];
    if (x < 100 && y < 100)
    {
        direction = DIDNT_MOVE;
        return;
    }
    if (x > y)
    {
        if (x == xyz[0])
            direction = LEFT;
        else
            direction = RIGHT;
    }
    else
    {
        if (y == xyz[2])
            direction = DOWN;
        else
            direction = UP;
    }
}

void initialize_snake(void)
{
    snake.snake_length = 0;
    // for (int i = 0; i < MAX_SNAKE_SIZE; i++)
    // {
    //     snake.snake_body[i].body_position.x = 0;
    //     snake.snake_body[i].body_position.y = 0;
    //     snake.snake_body[i].color = OLEDC_COLOR_BLACK;
    // }
    snake.snake_body[0].body_position.x = BOARD_X / 2; // snake starting position is the middle of the board
    snake.snake_body[0].body_position.y = BOARD_Y / 4;
    snake.snake_body[0].color = OLEDC_COLOR_RED;
    snake.snake_body[0].direction = UP;
    for (int i = 1; i <= 4; i++)
    {
        snake.snake_body[i].body_position.x = snake.snake_body[i - 1].body_position.x;
        snake.snake_body[i].body_position.y = snake.snake_body[i - 1].body_position.y + 1;
        set_snake_color();
        snake.snake_body[i].direction = UP;
        snake.snake_length = i;
    }
}

void initialize_game(void)
{
    clearBoard();
    initialize_snake();
    // move_snake();
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

void direction_diff(int16_t *x, int16_t *y)
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
    case DIDNT_MOVE:
        break;
    default:
        break;
    }
}

// snake move function

void move_snake(void)
{
    int16_t add_x = 0, add_y = 0;
    direction_diff(&add_x, &add_y);

    erase_snake_color();
    // moves the snake body
    for (int i = snake.snake_length+1; i > 1; i--)
    {
        snake.snake_body[i].body_position.x = snake.snake_body[i - 1].body_position.x;
        snake.snake_body[i].body_position.y = snake.snake_body[i - 1].body_position.y;
        snake.snake_body[i].direction = snake.snake_body[i - 1].direction;
    }

    // moves the snake head
    snake.snake_body[0].body_position.x += add_x;
    snake.snake_body[0].body_position.y += add_y;
}

// logical function to check if the snake has hit the wall
//  returns true if it has hit the wall

bool check_wall_collision(void)
{
    int16_t add_x = 0, add_y = 0;
    direction_diff(&add_x, &add_y);
    if (snake.snake_body[0].body_position.x + add_x < 0 ||
        snake.snake_body[0].body_position.x + add_x >= BOARD_X ||
        snake.snake_body[0].body_position.y + add_y < 0 ||
        snake.snake_body[0].body_position.y + add_y >= BOARD_Y)
    {
        return true;
    }
    return false;
}

// logical function to check if the snake has hit something
// returns enum board_options

enum board_options check_hit_something(void)
{
    enum board_options hit = EMPTY;
    int16_t add_x = 0, add_y = 0;
    direction_diff(&add_x, &add_y);
    if(add_x == 0 && add_y == 0)
    {
        return DIDNT_MOVE;
    }
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

void set_snake_color()
{
    switch ((snake.snake_length - 1) % 3) // snake length - 1 to avoid the head
    {
    case 0:
        snake.snake_body[snake.snake_length].color = OLEDC_COLOR_GREEN;
        break;
    case 1:
        snake.snake_body[snake.snake_length].color = OLEDC_COLOR_BLUE;
        break;
    case 2:
        snake.snake_body[snake.snake_length].color = OLEDC_COLOR_YELLOW;
        break;
    default:
        break;
    }
}

// erase past last body piece color
void erase_snake_color(void)
{
    snake.snake_body[snake.snake_length+1].color = oledC_getBackground();
}

void snake_game_options()
{
    enum board_options hit = check_hit_something();
    switch (hit)
    {
    case WALL:
    case SNAKE:
    case DIDNT_MOVE:
        // do nothing
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
        erase_snake_color();
        move_snake();
        break;
    default:
        break;
    }
}

void view_game(void)
{
    uint8_t current_y = 0;
    uint8_t current_x = 0;
    int16_t add_x = 0, add_y = 0;
    for (int i = 0; i <= snake.snake_length; i++)
    {
        current_y = snake.snake_body[i].body_position.y * 2 + pot_off_set;
        current_x = snake.snake_body[i].body_position.x * 2;
        direction_diff(&add_x, &add_y);
        add_x = add_x * 2;
        add_y = add_y * 2;
        if (add_x < 0)
        {
            current_x = current_x + add_x;
            add_x = 2;
        }
        if (add_y < 0)
        {
            current_y = current_y + add_y;
            add_y = 2;
        }
        if (current_y < 96 && current_x < 48 && current_y >= 0 && current_x >= 0)
            continue;

        oledC_DrawRectangle(current_x,
                            current_y,
                            (current_x + add_x + 2),
                            (current_y + add_y + 2),
                            snake.snake_body[i].color);
    }
}

void errorStop(char *msg)
{
    oledC_DrawString(0, 20, 2, 2, msg, OLEDC_COLOR_DARKRED);

    for (;;)
        ;
}

// NEEDS CHECKING !!! ???

void read_potentiometer_offset(void)
{
    int pot = 0;
    char pot_str[4] = {0};
    AD1CHSbits.CH0SA3 = 1; // select AN8 for A/D conversion ptoentiometer is connected to AN8
    AD1CON1bits.SAMP = 1;  // start sampling
    for (int i = 1; i < 1000; i++)
        ;
    AD1CON1bits.SAMP = 0;
    // for (int i = 1; i < 100; i++)
    //     ;
    while (AD1CON1bits.DONE == 0)
        ;
    pot = ADC1BUF0;
    pot_off_set = (int)(((float)pot / 1023.0) * 96.0);
    sprintf(pot_str, "%d", pot_off_set);
}

// ???

void potentiometer_initilize(void)
{
    AD1CON1bits.SSRC = 0;
    AD1CON1bits.MODE12 = 0;
    AD1CON1bits.ADON = 1;
    AD1CON1bits.FORM = 0;
    AD1CON2bits.SMPI = 0;
    AD1CON2bits.PVCFG = 0;
    AD1CON2bits.NVCFG0 = 0;
    AD1CON3bits.ADCS = 0xFF;
    AD1CON3bits.SAMC = 0B10000;
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
    //     initialize the accelerometer
    accelerometer_initialize();
}

// void timer_1_init()
// {
//     // Timer1 control register
//     T1CON = 0;
//     T1CONbits.TON = 1;
//     T1CONbits.TSIDL = 1;
//     T1CONbits.TGATE = 0;
//     T1CONbits.TCKPS = 0b1; // 1:1 prescaler
//     T1CONbits.TCS = 0;

//     // Timer1 interrupt settings and period
//     TMR1 = 0;
//     PR1 = 0xF424;      // timer period value
//     IFS0bits.T1IF = 0; // Clear Timer1 interrupt flag
//     IEC0bits.T1IE = 1; // Enable Timer1 interrupt
// }

// void __attribute__((__interrupt__)) _T1Interrupt(void)
// {
//     read_potentiometer_offset();
//     find_direction();
//     snake_game_options();
//     view_game();
//     IFS0bits.T1IF = 0;
// }

void view_initialize(void)
{
    oledC_setBackground(OLEDC_COLOR_BLACK);
    oledC_clearScreen();
    // timer_1_init();
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
    {
        read_potentiometer_offset();
        find_direction();
        snake_game_options();
        view_game();
        DELAY_milliseconds(300);
    };
}
/**
 End of File
 */