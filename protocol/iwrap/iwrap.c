/*
Copyright 2011 Jun Wako <wakojun@gmail.com>

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

/* host driver for Bulegiga iWRAP */
/* Bluegiga BT12
 * Connections
 *    Hardware UART       Software UART            BlueTooth
 * PC=====UART=======AVR=====SUART====iWRAP(BT12)-----------PC
 *
 * - Hardware UART for Debug Console to communicate iWRAP
 * - Software UART for iWRAP control to send keyboard/mouse data
 */

#include <stdint.h>
#include <string.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "keycode.h"
#include "suart.h"
#include "uart.h"
#include "report.h"
#include "host_driver.h"
#include "iwrap.h"
#include "print.h"
#include <avr/eeprom.h>


/* iWRAP MUX mode utils. 3.10 HID raw mode(iWRAP_HID_Application_Note.pdf) */
#define MUX_HEADER(LINK, LENGTH) do { \
    xmit(0xbf);     /* SOF    */ \
    xmit(LINK);     /* Link   */ \
    xmit(0x00);     /* Flags  */ \
    xmit(LENGTH);   /* Length */ \
} while (0)
#define MUX_FOOTER(LINK) xmit(LINK^0xff)


static uint8_t connected = 0;
//static uint8_t channel = 1;

/* iWRAP buffer */
#ifndef NO_SUART_PORT
#define MUX_BUF_SIZE 64
static char buf[MUX_BUF_SIZE];
static uint8_t snd_pos = 0;

#define MUX_RCV_BUF_SIZE 256
static char rcv_buf[MUX_RCV_BUF_SIZE];
static uint8_t rcv_head = 0;
static uint8_t rcv_tail = 0;


/* receive buffer */
static void rcv_enq(char c)
{
    uint8_t next = (rcv_head + 1) % MUX_RCV_BUF_SIZE;
    if (next != rcv_tail) {
        rcv_buf[rcv_head] = c;
        rcv_head = next;
    }
}

static char rcv_deq(void)
{
    char c = 0;
    if (rcv_head != rcv_tail) {
        c = rcv_buf[rcv_tail++];
        rcv_tail %= MUX_RCV_BUF_SIZE;
    }
    return c;
}

/*
static char rcv_peek(void)
{
    if (rcv_head == rcv_tail)
        return 0;
    return rcv_buf[rcv_tail];
}
*/

static void rcv_clear(void)
{
    rcv_tail = rcv_head = 0;
}

/* iWRAP response */
ISR(PCINT1_vect, ISR_BLOCK) // recv() runs away in case of ISR_NOBLOCK
{
    if ((SUART_IN_PIN & (1<<SUART_IN_BIT)))
        return;

    static volatile uint8_t mux_state = 0xff;
    static volatile uint8_t mux_link = 0xff;
    uint8_t c = recv();
    switch (mux_state) {
        case 0xff: // SOF
            if (c == 0xbf)
                mux_state--;
            break;
        case 0xfe: // Link
            mux_state--;
            mux_link = c;
            break;
        case 0xfd: // Flags
            mux_state--;
            break;
        case 0xfc: // Length
            mux_state = c;
            break;
        case 0x00:
            mux_state = 0xff;
            mux_link = 0xff;
            break;
        default:
            if (mux_state--) {
                uart_putchar(c);
                rcv_enq(c);
            }
    }
}
#endif

/*------------------------------------------------------------------*
 * iWRAP communication
 *------------------------------------------------------------------*/
void iwrap_init(void)
{
    // reset iWRAP if in already MUX mode after AVR software-reset
    iwrap_send("RESET");
    iwrap_mux_send("RESET");
    _delay_ms(3000);
    iwrap_send("\r\nSET CONTROL MUX 1\r\n");
    _delay_ms(500);
    iwrap_check_connection();
}

void iwrap_mux_send(const char *s)
{
    rcv_clear();
    MUX_HEADER(0xff, strlen((char *)s));
    iwrap_send(s);
    MUX_FOOTER(0xff);
}

void iwrap_send(const char *s)
{
    while (*s)
        xmit(*s++);
}
#ifndef NO_SUART_PORT
/* send buffer */
void iwrap_buf_add(uint8_t c)
{
    // need space for '\0'
    if (snd_pos < MUX_BUF_SIZE-1)
        buf[snd_pos++] = c;
}

void iwrap_buf_del(void)
{
    if (snd_pos)
        snd_pos--;
}

void iwrap_buf_send(void)
{
    buf[snd_pos] = '\0';
    snd_pos = 0;
    iwrap_mux_send(buf);
}

void iwrap_call(void)
{
    char *p;

    iwrap_mux_send("SET BT PAIR");
    _delay_ms(500);

    p = rcv_buf + rcv_tail;
    while (!strncmp(p, "SET BT PAIR", 11)) {
        p += 7;
        strncpy(p, "CALL", 4);
        strncpy(p+22, " 11 HID\n\0", 9);
        print_S(p);
        iwrap_mux_send(p);
        // TODO: skip to next line
        p += 57;

        DEBUG_LED_CONFIG;
        DEBUG_LED_ON;
        _delay_ms(500);
        DEBUG_LED_OFF;
        _delay_ms(500);
        DEBUG_LED_ON;
        _delay_ms(500);
        DEBUG_LED_OFF;
        _delay_ms(500);
        DEBUG_LED_ON;
        _delay_ms(500);
        DEBUG_LED_OFF;
        _delay_ms(500);
        DEBUG_LED_ON;
        _delay_ms(500);
        DEBUG_LED_OFF;
        _delay_ms(500);
        DEBUG_LED_ON;
        _delay_ms(500);
        DEBUG_LED_OFF;
        _delay_ms(500);
    }
    iwrap_check_connection();
}

void iwrap_kill(void)
{
    char c;
    iwrap_mux_send("LIST");
    _delay_ms(500);

    while ((c = rcv_deq()) && c != '\n') ;
    if (strncmp(rcv_buf + rcv_tail, "LIST ", 5)) {
        print("no connection to kill.\n");
        return;
    }
    // skip 10 'space' chars
    for (uint8_t i = 10; i; i--)
        while ((c = rcv_deq()) && c != ' ') ;

    char *p = rcv_buf + rcv_tail - 5;
    strncpy(p, "KILL ", 5);
    strncpy(p + 22, "\n\0", 2);
    print_S(p);
    iwrap_mux_send(p);
    _delay_ms(500);

    iwrap_check_connection();
}

void iwrap_unpair(void)
{
    iwrap_mux_send("SET BT PAIR");
    _delay_ms(500);

    char *p = rcv_buf + rcv_tail;
    if (!strncmp(p, "SET BT PAIR", 11)) {
        strncpy(p+29, "\n\0", 2);
        print_S(p);
        iwrap_mux_send(p);
    }
}
#else
void iwrap_call(void)
{
    //char *p;
	//char c;
	uint8_t i = 0;
    char *callCmd = "CALL REPLACE_ME_BY_MAC 11 HID";
    //pair known remote device
    //iwrap_mux_send("SET BT PAIR");
    //_delay_ms(500);
	paired_device_info_t paired_device_info;
    while(!eeprom_is_ready());
	eeprom_read_block(&paired_device_info, PAIRED_DEVICE_INFO_ADDR, sizeof(paired_device_info_t));
		
    //p = (char *)get_rx_buf();
	for(i = 0;i < 3;i++)
	{
	  strncpy(callCmd+5, paired_device_info.macAddr[i], 17);
	  iwrap_mux_send(callCmd);
      _delay_ms(5000);
	  if(iwrap_check_connection())
		return;
	  _delay_ms(500);
	#if 0
	  if(0 == strncmp(p, "SET BT PAIR", 11))
	  {
	    p += 7;
        strncpy(p, "CALL", 4);
        strncpy(p+22, " 11 HID", 8);
        iwrap_mux_send(p);
		_delay_ms(500);
		if(iwrap_check_connection())
			break;
	  }
	  #endif
	}

	if(3 == i)
	{
	  //didn't connect to known remote device
	  //clear receive buffer, prepare for receive RING
	  rcv_clear();
	  memset(get_rx_buf(), 0x00, RX_BUFFER_SIZE);
	  while(1)
	  {
		if(('R' == rcv_deq()) 
			&& ('I' == rcv_deq())
			&& ('N' == rcv_deq())
			&& ('G' == rcv_deq()))
		{
		  
		  char macAddr[17];
		  //pop up space
		  rcv_deq();
		  //pop up link id
		  rcv_deq();
		  //pop up space
		  rcv_deq();
		  //pop up mac address
		  for(int k = 0;k < 17;k++)
		  {
		    macAddr[k] = rcv_deq();
		  }
		  strncpy(callCmd + 5, macAddr, 17);
		  iwrap_mux_send(callCmd);
		  _delay_ms(500);

		  if(iwrap_check_connection())
		  {
		    uint8_t currentPairedIndex = (paired_device_info.lastPairedIndex + 1) % 3;
			strncpy(paired_device_info.macAddr[currentPairedIndex], macAddr, 17);
			paired_device_info.lastPairedIndex = currentPairedIndex;
			while(!eeprom_is_ready());
			eeprom_write_block(&paired_device_info, PAIRED_DEVICE_INFO_ADDR, sizeof(paired_device_info_t));
			return;
		  }
		  _delay_ms(500);
		}
		else
		  _delay_ms(100);
	  }	
	}
}

void iwrap_kill(void)
{
    char c;
    iwrap_mux_send("LIST");
    _delay_ms(500);

    while ((c = rcv_deq()) && c != '\n') ;
    if (strncmp((const char *)get_rx_buf_tail(), "LIST ", 5)) {
        print("no connection to kill.\n");
        return;
    }
    // skip 10 'space' chars
    for (uint8_t i = 10; i; i--)
        while ((c = rcv_deq()) && c != ' ') ;

    char *p = (char *)(get_rx_buf_tail() - 5);
    strncpy(p, "KILL ", 5);
    strncpy(p + 22, "\n\0", 2);
    print_S(p);
    iwrap_mux_send(p);
    _delay_ms(500);

    iwrap_check_connection();
}

void iwrap_unpair(void)
{
    iwrap_mux_send("SET BT PAIR");
    _delay_ms(500);

    char *p = (char *)get_rx_buf_tail();
    if (!strncmp(p, "SET BT PAIR", 11)) {
        strncpy(p+29, "\n\0", 2);
        print_S(p);
        iwrap_mux_send(p);
    }
}

#endif


void iwrap_sleep(void)
{
    iwrap_mux_send("SLEEP");
}

void iwrap_sniff(void)
{
}

void iwrap_subrate(void)
{
}
#ifndef NO_SUART_PORT
bool iwrap_failed(void)
{
    if (strncmp(rcv_buf, "SYNTAX ERROR", 12))
        return true;
    else
        return false;
}
#else
bool iwrap_failed(void)
{
    if (strncmp((const char *)get_rx_buf(), "SYNTAX ERROR", 12))
        return true;
    else
        return false;
}
#endif

uint8_t iwrap_connected(void)
{
    return connected;
}
#ifndef NO_SUART_PORT
uint8_t iwrap_check_connection(void)
{
    iwrap_mux_send("LIST");
    _delay_ms(100);

    if (strncmp(rcv_buf, "LIST ", 5) || !strncmp((const char *)rcv_buf, "LIST 0", 6))
        connected = 0;
    else
        connected = 1;
    return connected;
}
#else
uint8_t iwrap_check_connection(void)
{
    iwrap_mux_send("LIST");
    _delay_ms(100);

    if (strncmp((const char *)get_rx_buf(), "LIST ", 5) || !strncmp((const char *)get_rx_buf(), "LIST 0", 6))
        connected = 0;
    else
        connected = 1;
    return connected;
}
#endif


/*------------------------------------------------------------------*
 * Host driver
 *------------------------------------------------------------------*/
static uint8_t keyboard_leds(void);
static void send_keyboard(report_keyboard_t *report);
static void send_mouse(report_mouse_t *report);
static void send_system(uint16_t data);
static void send_consumer(uint16_t data);

static host_driver_t driver = {
        keyboard_leds,
        send_keyboard,
        send_mouse,
        send_system,
        send_consumer
};

host_driver_t *iwrap_driver(void)
{
    return &driver;
}

static uint8_t keyboard_leds(void) {
    return 0;
}

static void send_keyboard(report_keyboard_t *report)
{
    if (!iwrap_connected() && !iwrap_check_connection()) return;
    MUX_HEADER(0x01, 0x0c);
    // HID raw mode header
    xmit(0x9f);
    xmit(0x0a); // Length
    xmit(0xa1); // DATA(Input)
    xmit(0x01); // Report ID
    xmit(report->mods);
    xmit(0x00); // reserved byte(always 0)
    xmit(report->keys[0]);
    xmit(report->keys[1]);
    xmit(report->keys[2]);
    xmit(report->keys[3]);
    xmit(report->keys[4]);
    xmit(report->keys[5]);
    MUX_FOOTER(0x01);
}

static void send_mouse(report_mouse_t *report)
{
#if defined(MOUSEKEY_ENABLE) || defined(PS2_MOUSE_ENABLE)
    if (!iwrap_connected() && !iwrap_check_connection()) return;
    MUX_HEADER(0x01, 0x09);
    // HID raw mode header
    xmit(0x9f);
    xmit(0x07); // Length
    xmit(0xa1); // DATA(Input)
    xmit(0x02); // Report ID
    xmit(report->buttons);
    xmit(report->x);
    xmit(report->y);
    xmit(report->v);
    xmit(report->h);
    MUX_FOOTER(0x01);
#endif
}

static void send_system(uint16_t data)
{
    /* not supported */
}

static void send_consumer(uint16_t data)
{
#ifdef EXTRAKEY_ENABLE
    static uint16_t last_data = 0;
    uint8_t bits1 = 0;
    uint8_t bits2 = 0;
    uint8_t bits3 = 0;

    if (!iwrap_connected() && !iwrap_check_connection()) return;
    if (data == last_data) return;
    last_data = data;

    // 3.10 HID raw mode(iWRAP_HID_Application_Note.pdf)
    switch (data) {
        case AUDIO_VOL_UP:
            bits1 = 0x01;
            break;
        case AUDIO_VOL_DOWN:
            bits1 = 0x02;
            break;
        case AUDIO_MUTE:
            bits1 = 0x04;
            break;
        case TRANSPORT_PLAY_PAUSE:
            bits1 = 0x08;
            break;
        case TRANSPORT_NEXT_TRACK:
            bits1 = 0x10;
            break;
        case TRANSPORT_PREV_TRACK:
            bits1 = 0x20;
            break;
        case TRANSPORT_STOP:
            bits1 = 0x40;
            break;
        case TRANSPORT_EJECT:
            bits1 = 0x80;
            break;
        case AL_EMAIL:
            bits2 = 0x01;
            break;
        case AC_SEARCH:
            bits2 = 0x02;
            break;
        case AC_BOOKMARKS:
            bits2 = 0x04;
            break;
        case AC_HOME:
            bits2 = 0x08;
            break;
        case AC_BACK:
            bits2 = 0x10;
            break;
        case AC_FORWARD:
            bits2 = 0x20;
            break;
        case AC_STOP:
            bits2 = 0x40;
            break;
        case AC_REFRESH:
            bits2 = 0x80;
            break;
        case AL_CC_CONFIG:
            bits3 = 0x01;
            break;
        case AL_CALCULATOR:
            bits3 = 0x04;
            break;
        case AL_LOCK:
            bits3 = 0x08;
            break;
        case AL_LOCAL_BROWSER:
            bits3 = 0x10;
            break;
        case AC_MINIMIZE:
            bits3 = 0x20;
            break;
        case TRANSPORT_RECORD:
            bits3 = 0x40;
            break;
        case TRANSPORT_REWIND:
            bits3 = 0x80;
            break;
    }

    MUX_HEADER(0x01, 0x07);
    xmit(0x9f);
    xmit(0x05); // Length
    xmit(0xa1); // DATA(Input)
    xmit(0x03); // Report ID
    xmit(bits1);
    xmit(bits2);
    xmit(bits3);
    MUX_FOOTER(0x01);
#endif
}
