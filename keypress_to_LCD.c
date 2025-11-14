/*
 * ===================================================================
 * Final Combined Code for 4x4 Keypad and 16x2 LCD
 *
 * MCU: STM32G071xx
 *
 * --- KEYPAD ---
 * Rows: PA8, PA9, PA10, PA11 (Input, Pull-down)
 * Cols: PC4, PC5, PC6, PC7 (Output, Push-pull)
 *
 * --- LCD (8-bit mode) ---
 * Ctrl: PA5(EN), PA6(R/W), PA7(RS) (Output, Push-pull)
 * Data: PB0 - PB7 (Output, Push-pull)
 *
 * This code takes the user's preferred 'goto' structure and
 * fixes all logic, GPIO, and encoding errors, while integrating
 * the LCD display functions.
 * ===================================================================
 */

#include "stm32g071xx.h"
#include <stdint.h>     // For standard types (uint32_t, uint8_t)

// --- Function Prototypes ---
void LCD_cmd(uint8_t comd);
void LCD_disp(unsigned char char1);
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
    uint8_t j;
    uint32_t rowval;
    uint8_t row_index = 0;
    uint8_t col_index = 0;
    char key_char;

    // --- 1. GPIO and Clock Initialization ---

    // Enable Clocks for GPIOA, GPIOB, and GPIOC
    RCC->IOPENR |= 0x00000007; // Bit 0=A, Bit 1=B, Bit 2=C

    // --- Configure GPIOC (Columns PC4-7) as Output ---
    GPIOC->MODER &= 0xFFFF00FF; // Clear bits
    GPIOC->MODER |= 0x00005500; // Set PC4-PC7 as output (01)
    GPIOC->OTYPER &= 0xFFFFFF0F; // Set PC4-PC7 as push-pull (0)
    GPIOC->PUPDR &= 0xFFFF00FF;  // Set PC4-PC7 no pull-up/down (00)

    // --- Configure GPIOA (Rows PA8-11 & LCD Ctrl PA5-7) ---
    // Clear all settings for PA5-PA11 first
    GPIOA->MODER &= 0xFF0003FF;
    GPIOA->OTYPER &= 0xFFFFF01F;
    GPIOA->PUPDR &= 0xFF0003FF;

    // Set PA8-PA11 (Rows) as Input (00)
    // (Already 00 from clearing)

    // *FIX 1: Set PA8-PA11 as PULL-DOWN (10)*
    GPIOA->PUPDR |= 0x00AA0000; // This was wrong in your code

    // Set PA5-PA7 (LCD Ctrl) as Output (01)
    GPIOA->MODER |= 0x00005400;
    // (OTYPER and PUPDR are already 0)

    // --- Configure GPIOB (LCD Data PB0-7) as Output ---
    GPIOB->MODER &= 0xFFFF0000; // Clear bits
    GPIOB->MODER |= 0x00005555; // Set PB0-PB7 as output (01)
    GPIOB->OTYPER &= 0xFFFFFF00; // Set PB0-PB7 as push-pull (0)
    GPIOB->PUPDR &= 0xFFFF0000;  // Set PB0-PB7 no pull-up/down (00)

    // --- 2. LCD Initialization ---
    GPIOA->ODR &= 0xFFBF; // Set PA6 (R/W) LOW (Write mode)
    msdelay(50);          // Wait for LCD to power on
    LCD_cmd(0x38);        // 8-bit mode, 2 line, 5x7 dots
    LCD_cmd(0x01);        // clear display screen
    LCD_cmd(0x0E);        // display on, cursor on
    LCD_cmd(0x06);        // auto increment cursor
    LCD_cmd(0x80);        // Go to 1st line

    // Display a startup message
    char msg[] = "Enter Key:";
    for(j=0; msg[j] != '\0'; j++) {
        LCD_disp(msg[j]);
    }


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
                // *FIX 2: Logic Corrected*
                // Key press is confirmed, break out of this wait loop
                // to proceed with the scan.
                break;
            }
        }
    } // End of wait-for-press loop

    // --- 3d. Scan Columns to Find Pressed Key ---
    // (This code now runs after a press is confirmed)
    for(j = 0; j < 4; j++)
    {
        // *FIX 3: Correct BSRR values*
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
    // *FIX 4: Correctly find row_index from rowval*
    // (Your code was missing this)
    if (rowval == 0x0100) row_index = 0;      // PA8
    else if (rowval == 0x0200) row_index = 1; // PA9
    else if (rowval == 0x0400) row_index = 2; // PA10
    else if (rowval == 0x0800) row_index = 3; // PA11

    // Get the character from the map
    key_char = keypad_map[row_index][col_index];

    // --- 3f. Display Key on LCD ---
    LCD_cmd(0xC0); // Go to 2nd line
    LCD_disp(key_char);
    LCD_disp(' '); // Add a space to clear the next char

    // Go back to the beginning to wait for the key to be released
    goto jump;
}


// --- LCD & Delay Function Implementations ---

/**
 * @brief Sends a command byte to the LCD
 */
void LCD_cmd(uint8_t comd) {
    GPIOB->ODR = (GPIOB->ODR & 0xFF00) | comd; // Send command on PB0-PB7
    GPIOA->ODR &= 0xFF7F; // RS=0 (Command mode)
    GPIOA->ODR |= 0x0020; // EN=1
    msdelay(1);
    GPIOA->ODR &= 0xFFDF; // EN=0
    msdelay(50);
}

/**
 * @brief Sends a data (character) byte to the LCD
 */
void LCD_disp(unsigned char char1) {
    GPIOB->ODR = (GPIOB->ODR & 0xFF00) | char1; // Send data on PB0-PB7
    GPIOA->ODR |= 0x0080; // RS=1 (Data mode)
    GPIOA->ODR |= 0x0020; // EN=1
    msdelay(1);
    GPIOA->ODR &= 0xFFDF; // EN=0
    msdelay(50);
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
