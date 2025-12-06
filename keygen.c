#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


int output_file();
char* create_string();

int main(int argc, char *argv[]){
	int length = atoi(argv[1]);

	char* return_value = create_string(length);

	printf("%s\n", return_value);
}

char* create_string(int length){
	char possible_characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	char* string = calloc(length + 1, sizeof(char));
	srand(time(NULL));
    for (int i = 0; i < length; i++){

		char random_letter = possible_characters[rand() % 27];
		string[i] = random_letter;
    }
	return string;
}