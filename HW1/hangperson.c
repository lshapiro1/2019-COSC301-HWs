#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>


static const int MAX_MISSES = 7;


/*
 * Be happy you didn't have to write this function.
 * Prints a low-tech ASCII gallows.  Max number of misses
 * is MAX_MISSES (i.e., if there are MAX_MISSES misses, the 
 * player loses and the poor sap gets hung).  
 */
void print_gallows(int num_missed) {
    // make sure that we got called in a reasonable way
    assert(num_missed >= 0 && num_missed <= MAX_MISSES);

    printf("\n\n       |||========|||\n");
    if (num_missed > 0) {
        printf("       |||         |\n");
    } else {
        printf("       |||          \n");
    }

    if (num_missed > 1) {
        printf("       |||         O\n");
    } else {
        printf("       |||          \n");
    }

    if (num_missed > 2) {
        if (num_missed > 4) {
            printf("       |||        /|\\\n");
        } else if (num_missed > 3) {
            printf("       |||        /| \n");
        } else {
            printf("       |||        /  \n");
        }
    } else {
        printf("       |||           \n");
    }

    if (num_missed > 5) {
        if (num_missed > 6) {
            printf("       |||        / \\\n");
        } else {
            printf("       |||        /  \n");
        }
    } else {
        printf("       |||           \n");
    }

    printf("       |||\n");
    printf("       |||\n");
    printf("     =================\n\n");
}


/* *
 * Initialize the game state at the beginning of a new game.
 * Params: 1) the secret word, 
 *         2) the array holding information on what letters have been correctly 
 *            guessed.
 *         3) an array holding information on what letters have previously been
 *            guessed.
 * Returns: Nothing.
 * */
void initialize_game_state(const char word[], char game_state[], bool already_guessed[26]) {
    int size = strlen(word);
    memset(game_state, '_', size);     // fill game_state with '_'

    memset(already_guessed, false, 26);     // fill already_guessed with bool 'false'
    // your code here

}


/* *
 * Update the game state after a new guess.
 * Params: 1) the guess that has been made.
 *         2) the secret word.
 *         3) the array holding information on what letters have been
 *            correctly guessed.
 * Returns: bool indicating whether the guess was correct or not.
 * */
bool update_game_state(char guess, const char word[], char game_state[]) {
     bool found = false;
    for (int i = 0; i < strlen(word); i++){     // loops through the word to see if the guess matches any of the characters in the given word
        if (word[i] == guess){
            game_state[i] = guess;      // if found, update game_state in correct loc
            found = true;
        }
    }
    if (found == true){
        return true;
    }
    return false;
    // your code here

}


/* *
 * Print the current game state.
 * Params: 1) the secret word.
 *         2) the array holding information on what letters have been
 *            correctly guessed.
 *         3) the array holding what letters have been previously guessed.
 * Returns: Nothing.
 * */
void print_game_state(const char word[], char game_state[], bool already_guessed[26]) {
    printf("%s\n", game_state);
    printf("What is your guess? ");
    for (int i = 0; i < strlen(word); i++){ // needs to be strlen(word), not strlen(already_guessed)
        if (already_guessed[i] == true){
            printf("%c", (i+65));        // print out letter value if boolean = true
        }
    }
    // your code here

}


/* *
 * Get the next guess from the keyboard.
 * Params: Nothing.
 * Returns: A single letter in upper-case (i.e., the new guess).
 * 
 * Note: this function should not return until the player has given
 * a guess that is just one letter.  The function doesn't care whether
 * the guess has been previously made or not.
 * */
char get_guess(void) {
    bool issuff = false;
    char guess;
    while (issuff == false){
        printf("What is your guess? ");
        guess = getchar();
        // guess isn't a c string (not nul-char terminated) so
        // you cannot call strlen on it.
        //if (strlen(&guess) == 1){        // if guess is only one char long, break loop
            // if (isalpha(guess) == 0){     // checks to see if the guess is a letter
            // wrong check
            if (isalpha(guess)) {     // checks to see if the guess is a letter
                issuff = true;
            }
            else {
                printf("Please enter a letter. ");
            }
        // }
    }
    guess = toupper(guess);

    /*
     * use toupper() instead.
    if (guess >= 97 &&  guess <= 122){
        guess = guess - 32;
    }
    */

    return guess;
    // your code here

}


/* *
 * Check whether the player has won.
 * Params: 1) the secret word.
 *         2) the array holding information on what letters have been
 *            correctly guessed.
 * Returns: bool indicating whether the player has won or not.
 * */
bool won(const char word[], char game_state[]) {
    // game_state is not a valid C string.  it only 
    // holds enough characters for each letter in the word,
    // but not for the null-termination character.  so
    // you need to do this in a for loop; you can't just call 
    // strcmp directly.

    if (strcmp(word, game_state) == 0){     // compares the arrays to see if they are the same
        return true;
    }
    return false;
    // your code here

}


/* *
 * Check whether the player has lost.
 * Params: 1) the number of incorrect guesses that the player has made.
 * Returns: bool indicating whether the player has lost or not.
 * */
bool lost(int incorrect_guesses) {
    if (incorrect_guesses == MAX_MISSES){
        return true;
    }
    return false;
    // your code here 

}


/* *
 * Check whether the current guess has previously been made or not.
 *
 * Params: 1) the current guess
 *         2) an array holding information on what letters have previously been
 *            guessed.
 * Returns: bool indicating whether the current guess has previously been made.
 * */
bool previous_guess(char guess, bool already_guessed[26]) {
    char newGuess = toupper(guess);
    // instead of a magic number 65, just use 'A'
    // int loc = newGuess - 65;           // gets the index of where the guess would reside within the array

    int loc = newGuess - 'A';           // gets the index of where the guess would reside within the array
    if (already_guessed[loc] == true){      // checks if the guess has been inputted previously
        return true;
    }
    already_guessed[loc] = true;
    return false;
    // your code here 

}


/*
 * Play one game of Hangperson.  The secret word is passed as a
 * parameter.  The function should return true if the player won,
 * and false otherwise.
 *
 * Params: 1) the secret word.
 * Returns: bool indicating whether the player won or not.
 */
bool one_game(const char word[]) {
    char game_state[strlen(word)];
    bool already_guessed[26];
    int incorrect_guesses = 0;

    initialize_game_state(word, game_state, already_guessed);

    while (!won(word, game_state) && !lost(incorrect_guesses)) {
        print_gallows(incorrect_guesses);
        print_game_state(word, game_state, already_guessed);
        printf("\n");
        char next_guess = get_guess();
        if (previous_guess(next_guess, already_guessed)) {
            printf("You already guessed that.\n");
        } else {
            if (!update_game_state(next_guess, word, game_state)) {
                printf("Bad guess, fool.\n");
                incorrect_guesses += 1;
            } else {
                printf("Good guess.\n");
            }
            printf("Missed: %d\n", incorrect_guesses);
        }
    }

    if (won(word, game_state)) { 
        return true;
    } 
    return false;
}

