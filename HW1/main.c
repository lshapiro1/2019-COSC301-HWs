#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/*
 *  forward declaration of one_game function to make
 *  compiler happy for this file.
 */
bool one_game(const char []);


/*
 * The main function is where everything starts.  Choose a random
 * word and call one_game.  
 */
int main() {
    /* You can modify the words used in the game if you want;
     * just be sure to also update the num_words constant */
    const char *words[] = {"WINTER", "SNOW", "ICE", "FROZEN"};
    const int num_words = 4;

    srand(time(NULL));
    printf("Number of words %d\n", num_words);
    const char *secret_word = words[rand() % num_words];
    printf("Secret word is %s\n", secret_word);

    if (one_game(secret_word)) {
        printf("Congratulations!  You saved stick-person!\n");
    } else {
        printf("You lost and made stick-person sad...\n");
    }
    return EXIT_SUCCESS;
}
