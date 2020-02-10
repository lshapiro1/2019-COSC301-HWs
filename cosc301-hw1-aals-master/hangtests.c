#include <stdio.h>
#include <string.h>
#include <stdbool.h>

void initialize_game_state(const char[], char[], bool[26]);
bool update_game_state(char, const char[], char[]);
bool won(const char[], char[]);
bool lost(int);
bool previous_guess(char, bool[26]);


void test_initialize_game_state() {
    printf("test_initialize_game_state()\n");
    char word[] = "TESTTHIS";
    char gamestate[strlen(word)];
    bool guessed[26];

    initialize_game_state(word, gamestate, guessed);
    bool ok = true;
    printf("\tGame state: (should be initialized to _ for every letter to guess)\n\t\t");
    for (int i = 0; i < strlen(word); i++) {
        printf("%c ", gamestate[i]);
        if (gamestate[i] != '_') {
            ok = false;
        }
    }
    if (ok) {
        printf("  ... ok\n");
    } else {
        printf("  ... not ok\n");
    }

    printf("\tAlready guessed (everything should be initialized to false)\n\t\t");
    ok = true;
    for (int i = 0; i < 26; i++) {
        if (guessed[i] != false) {
            printf("%d/%d ", i, guessed[i]);
            ok = false;
        }
    }
    if (ok) {
        printf("... looks fine\n");
    } else {
        printf("... not ok\n");
    }
}

void test_update_game_state() {
    printf("test_update_game_state()\n");
    bool ok = true;
    char word[] = "TRABANT";
    char state[strlen(word)];
    memset(state, '_', strlen(word));

    bool rv = true;
    printf("\tUpdating game state, guessing 'T' for word 'TRABANT';\n");
    printf("\texpecting T to be at indexes 0 and 6 in updated state:  ");
    rv = update_game_state('T', word, state);
    if (!rv) {
        printf("return val should be true but was not; ");
        ok = false;
    }
    if (state[0] != 'T') {
        printf("T should be at index 0 but was not; ");
        ok = false;
    }
    if (state[6] != 'T') {
        printf("T should be at index 6 but was not; ");
        ok = false;
    }
    printf("\n\tUpdating game state, guessing 'A' for word 'TRABANT';\n");
    printf("\texpecting T to be at indexes 2 and 4 in updated state:  ");
    rv = update_game_state('A', word, state);
    if (!rv) {
        printf("return val should be true but was not; ");
        ok = false;
    }
    if (state[2] != 'A') {
        printf("A should be at index 2 but was not; ");
        ok = false;
    }
    if (state[4] != 'A') {
        printf("A should be at index 4 but was not; ");
        ok = false;
    }
    char statecopy[strlen(word)];
    memcpy(statecopy, state, strlen(word));
    printf("\n\tUpdating game state, guessing 'Z' for word 'TRABANT';\n");
    printf("\texpecting game state to be unchanged: ");
    rv = update_game_state('Z', word, state);
    if (rv) {
        printf("return val should be false but was not; ");
        ok = false;
    }
    for (int i = 0; i < strlen(word); i++) {
        if (state[i] != statecopy[i]) {
            printf("badstatemod, index %d; ", i);
            ok = false;
        }
    }

    if (ok) {
        printf("ok\n");
    } else {
        printf("not ok\n");
    }
}

void test_won() {
    printf("test_won()\n\t");
    bool ok = true;
    if (won("TEST", "TEST") != true) {
        printf("won(\"TEST\", \"TEST\") != true; ");
        ok = false;
    }
    if (won("TEST", "_ES_") != false) {
        printf("won(\"TEST\", \"_ES_\") != false; ");
        ok = false;
    }
    if (won("TEST", "____") != false) {
        printf("won(\"TEST\", \"____\") != false; ");
        ok = false;
    }
    if (ok) {
        printf(" ... ok\n");
    } else {
        printf(" ... not ok\n");
    }
}

void test_lost() {
    printf("test_lost()\n\t");
    bool ok = true;
    if (lost(7) != true) {
        printf("lost(7) != true; ");
        ok = false;
    }
    for (int i = 0; i < 7; i++) {
        if (lost(i) != false) {
            printf("%d wrong; ", i);
            ok = false;
        }
    }
    if (ok) {
        printf(" ... ok\n");
    } else {
        printf(" ... not ok\n");
    }
}

void test_previous_guess() {
    printf("test_previous_guess()\n");
    bool ok = true;
    bool arr[26];
    memset(arr, 0, 26*sizeof(bool));
    for (char c = 'A'; c <= 'Z'; c++) {
        if (previous_guess(c, arr) != false) {
            printf("\t%c was not set in previous_guesses array but previous_guess returned true\n", c);
            ok = false;
        }
        if (previous_guess(c, arr) != true) {
            printf("\t%c was set in previous_guesses array but previous_guess returned false\n", c);
            ok = false;
        }
    }
    if (ok) {
        printf("\t ... ok\n");
    } else {
        printf("\t ... not ok\n");
    }
}

int main() {
    test_initialize_game_state();
    test_update_game_state();
    test_won();
    test_lost();
    test_previous_guess();

    return 0;
}
