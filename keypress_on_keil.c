/*
 * ===================================================================
 * Final Code for 4x4 Keypad - Debugged in Keil
 *
 * MCU: STM32G071xx
 *
 * --- KEYPAD ---
 * Rows: PA8, PA9, PA10, PA11 (Input, Pull-down)
 * Cols: PC4, PC5, PC6, PC7 (Output, Push-pull)
 *
 * This code detects a key press and stores the result in a
 * global variable (g_pressed_key) so it can be "watched"
 * in the Keil uVision debugger.
 * ===================================================================
 */

#include "stm32g071xx.h"
#include <stdint.h>     // For standard types (uint32_t, uint8_t)

// --- Global Variables to Watch in Keil ---
// "volatile" tells the compiler not to optimize this away.
// The debugger will be able to read this variable.
volatile uint8_t g_last_keycode = 0xFF; // Will store 0-15
volatile char    g_pressed_key  = '\0'; // Will store '1', 'A', etc.


// --- Function Prototypes ---
void msdelay(volatile uint32_t ms);

// --- Keypad mapping (4x4 matrix) ---
const char keypad_map[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

int main(void)
{
    uint32_t rowval;
    uint8_t row_index = 0;
    uint8_t col_index = 0;

    // --- 1. GPIO and Clock Initialization ---

    // Enable Clocks for GPIOA and GPIOC
    RCC->IOPENR |= 0x00000005; // Bit 0=A, Bit 2=C

    // --- Configure GPIOC (Columns PC4-7) as Output ---
    GPIOC->MODER &= 0xFFFF00FF; // Clear bits
    GPIOC->MODER |= 0x00005500; // Set PC4-PC7 as output (01)
    GPIOC->OTYPER &= 0xFFFFFF0F; // Set PC4-PC7 as push-pull (0)
    GPIOC->PUPDR &= 0xFFFF00FF;  // Set PC4-PC7 no pull-up/down (00)

    // --- Configure GPIOA (Rows PA8-11) as Input ---
    GPIOA->MODER &= 0xFF00FFFF; // Set PA8-PA11 (Rows) as Input (00)

    // Set PA8-PA11 as PULL-DOWN (10)
    GPIOA->PUPDR &= 0xFF00FFFF; // Clear bits
    GPIOA->PUPDR |= 0x00AA0000; // (10) for PA8, PA9, PA10, PA11


    // --- 3. Main Scanning Loop ---
jump:
    // --- 3a. Wait for Key Release ---
    GPIOC->BSRR = 0x000000F0; // Set all columns (PC4-7) HIGH
    while((GPIOA->IDR & 0x00000F00) != 0) {
        // Wait here until all keys are released
    }
    msdelay(10); // Debounce the release

    // --- 3b. Wait for Key Press ---
    while(1)
    {
        // Check for a press (with all columns high)
        rowval = (GPIOA->IDR & 0x00000F00); //Read PA8-PA11
        if(rowval != 0)
        {
            // --- 3c. Debounce Press ---
            msdelay(10); //debounce delay
            rowval = (GPIOA->IDR & 0x00000F00); //Read PA8-PA11 again
            if(rowval != 0)
            {
                // Key press is confirmed, break out of this loop
                break;
            }
        }
    } // End of wait-for-press loop

    // --- 3d. Scan Columns to Find Pressed Key ---
    for(uint8_t j = 0; j < 4; j++)
    {
        if(j==0)      { GPIOC->BSRR = 0x00E00010; } //Set PC4 high, reset 5,6,7
        else if(j==1) { GPIOC->BSRR = 0x00D00020; } //Set PC5 high, reset 4,6,7
        else if(j==2) { GPIOC->BSRR = 0x00B00040; } //Set PC6 high, reset 4,5,7
        else if(j==3) { GPIOC->BSRR = 0x00700080; } //Set PC7 high, reset 4,5,6

        rowval = (GPIOA->IDR & 0x00000F00); //Read PA8-PA11
        if(rowval != 0)
        {
            col_index = j; // Column found
            break;         // Exit for loop
        }
    }

    // --- 3e. Encode Key ---
    if (rowval == 0x0100) row_index = 0;      // PA8
    else if (rowval == 0x0200) row_index = 1; // PA9
    else if (rowval == 0x0400) row_index = 2; // PA10
    else if (rowval == 0x0800) row_index = 3; // PA11

    // --- 3f. Store Key in Global Variables ---
    // Instead of displaying on LCD, we store the value
    // for the Keil debugger to see.
    g_last_keycode = (row_index * 4) + col_index;
    g_pressed_key = keypad_map[row_index][col_index];


    // Go back to the beginning to wait for the key to be released
    goto jump;
}


/**
 * @brief A simple, blocking millisecond delay function.
 * (Adjust inner loop value based on your clock speed, 3180 is approx for 48MHz)
 */
void msdelay(volatile uint32_t ms) {
    volatile uint32_t i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 3180; j++) {
            __NOP(); // No operation
        }
    }
}
