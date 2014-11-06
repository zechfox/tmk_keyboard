/*
Copyright 2012 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * scan matrix
 */
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include "print.h"
#include "debug.h"
#include "util.h"
#include "matrix.h"
#include "uart.h"


#ifndef DEBOUNCE
#   define DEBOUNCE	5
#endif
static uint8_t debouncing = DEBOUNCE;

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];

static matrix_row_t read_cols(void);
static void init_cols(void);
static void unselect_rows(void);
static void select_row(uint8_t row);


inline
uint8_t matrix_rows(void)
{
    return MATRIX_ROWS;
}

inline
uint8_t matrix_cols(void)
{
    return MATRIX_COLS;
}

void matrix_init(void)
{
    // initialize row and col
    unselect_rows();
    init_cols();

    // initialize matrix state: all keys off
    for (uint8_t i=0; i < MATRIX_ROWS; i++) {
        matrix[i] = 0;
        matrix_debouncing[i] = 0;
    }
}

uint8_t matrix_scan(void)
{
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        select_row(i);
        _delay_us(30);  // without this wait read unstable value.
        matrix_row_t cols = read_cols();
        if (matrix_debouncing[i] != cols) {
            matrix_debouncing[i] = cols;
            if (debouncing) {
               // debug("bounce!: "); debug_hex(debouncing); debug("\n");
            }
            debouncing = DEBOUNCE;
        }
        unselect_rows();
    }

    if (debouncing) {
        if (--debouncing) {
            _delay_ms(1);
        } else {
            for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
                matrix[i] = matrix_debouncing[i];
            }
        }
    }

    return 1;
}

bool matrix_is_modified(void)
{
    if (debouncing) return false;
    return true;
}

inline
bool matrix_is_on(uint8_t row, uint8_t col)
{
    return (matrix[row] & ((matrix_row_t)1<<col));
}

inline
matrix_row_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}

void matrix_print(void)
{
    //print("\nr/c 0123456789ABCDEF\n");
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        phex(row); print(": ");
        pbin_reverse16(matrix_get_row(row));
        print("\n");
    }
}

uint8_t matrix_key_count(void)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        count += bitpop16(matrix[i]);
    }
    return count;
}
/*
DDxn        PORTxn            PUD           I/O    Pull-Up          Description
   0          0                X             In       No               Hi-Z
   0          1                0             In       Yes        output current when pull low by outside
   0          1                1             In       No               Hi-Z
   1          0                X             Out      No            output low level
   1          1                X             Out      No            output high level
*/

/* Column pin/GPIO configuration. Pin17(PB0)&Pin18(PB1) not connected
 * col:   0  1   2   3   4   5   6   7   8   9   10  11  12  13
 * pin:  19  20  21  22  26  27  16  15  14  13  12  11  10  9 
 * GPIO: PB2 PB3 PB4 PB5 PB6 PB7 PA0 PA1 PA2 PA3 PA4 PA5 PA6 PA7  
 */
static void  init_cols(void)
{
    // Input with pull-up(DDR:0, PORT:1)
    DDRA  = 0x00;
    PORTA = 0xFF;
    DDRB  &= 0x03;
    PORTB |=  0xFC;
	SFIOR &= ~(1<<PUD); //enable pull-up
}

static matrix_row_t read_cols(void)
{
    return (((matrix_row_t)(~PINA) << 6) & 0x3FC0) | (((~PINB) >> 2) & 0x3F);
}

/* Row pin configuration
 * row:   0   1   2   3   4   5   6   7
 * pin:   1   2   3   4   5   6   7   8
 * GPIO: PC0 PC1 PC2 PC3 PC4 PC5 PC6 PC7
 */
static void unselect_rows(void)
{
    // Hi-Z(DDR:0, PORT:0) to unselect
    DDRC  = 0x00;
    PORTC = 0x00;
}

static void select_row(uint8_t row)
{
    // Output low(DDR:1, PORT:0) to select
    //only 1 row select
    //DDRC  = (1 << row);
    //PORTC = ~(1 << row); 
    //continue select rows
    DDRC  |= (1 << row);
    PORTC &= ~(1 << row); 
}
