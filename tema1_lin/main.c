#include <stdio.h>
#include "libso_stdio.h"

int main(int argc, const char *argv[])
{
    // printf("hello");

    printf("TEST 1: ----------\n"); // citire simpla din fisier
    SO_FILE *file = so_fopen(argv[1], "r");
    char text1[30];
    int t = so_fread(text1, 1, 4, file);
    printf("S-au citit primii %d octeti din fisier, continut: %s\n\n", t, text1);

    printf("TEST 2: ----------\n"); // ftell
    printf("Pozitia actuala a cursorului este %d\n\n", (int)so_ftell(file));

    printf("TEST 3: ----------\n"); // seek
    so_fseek(file, 2, SEEK_SET);    // muta cursorul la pozitia 2 de la inceput
    printf("Noua pozitie a cursorului este %d\n\n", (int)so_ftell(file));

    printf("TEST 4: ----------\n"); // fgetc
    char c = so_fgetc(file);
    printf("Caracterul citit la pozitia %d este %c\n\n", (int)so_ftell(file), c);

    printf("TEST 4: ----------\n");
    if (so_fclose(file) == 0)
        printf("Fisier inchis.\n");
    else
        printf("Eroare la inchiderea fisierului.\n");

    printf("TEST 5: ----------\n"); // fputc in r+
    SO_FILE *f2 = so_fopen(argv[1], "r+");
    so_fseek(f2, 0, SEEK_SET);
    t = so_fread(text1, 1, 5, f2);
    printf("Text inainte de fputc: %s\n", text1);
    so_fseek(f2, 2, SEEK_SET);
    so_fputc('A', f2);
    so_fseek(f2, 0, SEEK_SET);
    t = so_fread(text1, 1, 6, f2);
    printf("Text inainte de fputc: %s\n", text1);
    so_fclose(f2);

    printf("TEST 6: ----------\n"); // fwrite
    SO_FILE *f3 = so_fopen(argv[1], "w+");
    char text2[] = "abcdefg";
    so_fwrite(text2, 1, 5, f3);
    so_fseek(f3, 0, SEEK_SET);
    t = so_fread(text1, 1, 5, f3);
    printf("Text introdus: %s\n", text1);

    printf("TEST 7: ----------\n"); // fputc
    so_fseek(f3, 2, SEEK_SET);
    so_fputc('A', f3);
    so_fseek(f3, 0, SEEK_SET);
    t = so_fread(text1, 1, 6, f3);
    printf("Text dupa fputc la pozitia 2: %s\n\n", text1);
    so_fclose(f3);

    printf("TEST 8: ----------\n");
    SO_FILE *f4 = so_fopen(argv[1], "a+");
    t = so_fread(text1, 1, 5, f4);
    printf("Text inainte de append: %s\n", text1);
    char text[] = "AAA";
    so_fwrite(text, 1, 3, f4);
    so_fseek(f4, 0, SEEK_SET);
    t = so_fread(text1, 1, 10, f4);
    printf("Text dupa append: %s\n", text1);
    so_fclose(f4);
    return 0;
}