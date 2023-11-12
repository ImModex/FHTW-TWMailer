#include "hide_pw.h"

int getch(){
    int ch;
    // https://man7.org/linux/man-pages/man3/termios.3.html
    struct termios t_old, t_new;

    // https://man7.org/linux/man-pages/man3/termios.3.html
    // tcgetattr() gets the parameters associated with the object referred
    //   by fd and stores them in the termios structure referenced by
    //   termios_p
    tcgetattr(STDIN_FILENO, &t_old);
    
    // copy old to new to have a base for setting c_lflags
    t_new = t_old;

    // https://man7.org/linux/man-pages/man3/termios.3.html
    //
    // ICANON Enable canonical mode (described below).
    //   * Input is made available line by line (max 4096 chars).
    //   * In noncanonical mode input is available immediately.
    //
    // ECHO   Echo input characters.
    t_new.c_lflag &= ~(ICANON | ECHO);
    
    // sets the attributes
    // TCSANOW: the change occurs immediately.
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    // reset stored attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);

    return ch;
}

char* getpw(){
    //printf("IM IN GETPW\n");
    int show_asterisk = 1;

    char *password = calloc(128, sizeof(char));

    const char BACKSPACE = 127;
    const char RETURN = 10;

    unsigned char ch = 0;

    while ((ch = getch()) != RETURN)
    {
        if (ch == BACKSPACE)
        {
            if (strlen(password) != 0)
            {
                if (show_asterisk)
                {
                    printf("\b \b"); // backslash: \b
                }
                password[strlen(password)-1] = '\0';
            }
        }
        else
        {
            password[strlen(password)] = ch;
            if (show_asterisk)
            {
                printf("*");
            }
            else{
                printf("%c",ch);
            }
        }
    }
    return password;
}