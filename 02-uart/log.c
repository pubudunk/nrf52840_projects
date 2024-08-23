#include "log.h"
#include "app_uart.h"
#include <stdarg.h>
#include <stdio.h>

#define LOG_BUFFER_SIZE   128

void UART_LOG(const char *format, ...)
{
    char buffer[LOG_BUFFER_SIZE];  // Buffer to hold the formatted string
    va_list args;
    int len;

    // Start processing variable arguments
    va_start(args, format);

    // Format the string with the given arguments
    len = vsnprintf(buffer, sizeof(buffer), format, args);

    // Ensure the buffer was large enough to hold the formatted string
    if (len >= 0 && len < sizeof(buffer))
    {
        // Send the formatted string over UART
        for (int i = 0; i < len; i++)
        {
            UNUSED_VARIABLE(app_uart_put((uint8_t)buffer[i]));
        }
    }

    // End processing variable arguments
    va_end(args);
}