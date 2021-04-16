/**
  Generated main.c file from MPLAB Code Configurator

  @Company
    Microchip Technology Inc.

  @File Name
    main.c

  @Summary
    This is the generated main.c using PIC24 / dsPIC33 / PIC32MM MCUs.

  @Description
    This source file provides main entry point for system initialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.170.0
        Device            :  PIC24FJ256GA705
    The generated drivers are tested against the following:
        Compiler          :  XC16 v1.61
        MPLAB 	          :  MPLAB X v5.45
*/

/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/

/**
  Section: Included Files
*/
#include <stdlib.h>
#include <stdio.h>

#include "System/system.h"
#include "System/delay.h"
#include "oledDriver/oledC_example.h"
#include "oledDriver/oledC.h"
#include "oledDriver/oledC_colors.h"
#include "oledDriver/oledC_shapes.h"

/*
                         Main application
 */
int main(void)
{
    int count=0, pot, prev_pot=-1;
    int wasPressed=0;
    int i;
    char countBuf[20];
    char buff[10], prev_buff[10];

    // initialize the system
    SYSTEM_Initialize();

    oledC_setBackground(OLEDC_COLOR_SKYBLUE);
    oledC_clearScreen();
    
    //Initialize
    TRISA |= (1<<11);
    TRISA &= ~(1<<8);

    AD1CON1bits.SSRC = 0x0 ;
    AD1CON1bits.MODE12 = 0 ;
    AD1CON1bits.ADON = 1 ;
    AD1CON1bits.FORM = 0b00 ;
    AD1CON2bits.SMPI = 0x0 ;
    AD1CON2bits.PVCFG = 0x00 ;
    AD1CON2bits.NVCFG0 = 0x0 ;
    AD1CON3bits.ADCS = 0xFF ;
    AD1CON3bits.SAMC = 0b10000 ;

    //Main loop
    while(1)
    {
        if (PORTA & (1<<11)) {
            LATA &= ~(1<<8);        //Off
            wasPressed=0;
        }
        else {
            LATA |= (1<<8);         //On
            if (!wasPressed) {
                wasPressed=1;
                if (count>0)
                  oledC_DrawString(0, 0, 2, 2, countBuf, OLEDC_COLOR_SKYBLUE);
                ++count;
                sprintf(countBuf, "S1 hit:%d", count);
                oledC_DrawString(0, 0, 2, 2, countBuf, OLEDC_COLOR_DARKGREEN);
            }
        }

        AD1CHS = 8;

        AD1CON1bits.SAMP = 1 ;           //Sample
        for (i = 0 ; i < 1000 ; i++) ; //Sample delay, conversion start automatically

        AD1CON1bits.SAMP = 0 ;           //Hold
        for (i = 0 ; i < 100 ; i++) ; //Sample delay, conversion start automatically

        while (!AD1CON1bits.DONE) ;       //Wait for conversion to complete

        pot = ADC1BUF0 ;
        sprintf(buff,"%d", pot);
        if (prev_pot>=0 && abs(pot-prev_pot)>2) {
          sprintf(prev_buff,"%d", prev_pot);
          oledC_DrawString(20, 5*8, 3, 3, prev_buff, OLEDC_COLOR_SKYBLUE);
        }
        // oledC_DrawRectangle(20, 5*8, 20+3*5*4, 5*8+3*8,OLEDC_COLOR_RED);
        if (abs(pot-prev_pot)>2) {
            oledC_DrawString(20, 5*8, 3, 3, buff, OLEDC_COLOR_BLACK);
            prev_pot=pot;
        }
        DELAY_milliseconds(100);
    }    return 1;
}
/**
 End of File
*/

