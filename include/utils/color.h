#ifndef COLOR_H
#define COLOR_H

/* * ANSI Escape Codes for terminal colors.
 * Usage: printf(COLOR_RED "Error!" COLOR_END "\n");
 */

#define COLOR_END "\033[0m"  // Reset to default
#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_BROWN "\033[0;33m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_PURPLE "\033[1;35m"
#define COLOR_CYAN "\033[1;36m"

#endif
