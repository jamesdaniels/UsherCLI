#include <stdio.h>

int main(int argc, char *argv[]) {

	int distance = 100; 
	float power = 2.345f;
	double super_power = 56789.4532; 
	char initial = 'A';
	char first_name[] = "Zed"; 
	char last_name[] = "Shaw";
	int areas[] = {10, 12, 13, 14, 20};

	printf("You are %d miles away.\n", distance); 
	printf("You have %f levels of power.\n", power); 
	printf("You have %f awesome super powers.\n", super_power); 
	printf("I have an initial %c.\n", initial); 
	printf("I have a first name %s.\n", first_name); 
	printf("I have a last name %s.\n", last_name); 
	printf("My whole name is %s %c. %s.\n", first_name, initial, last_name);

	printf("The size of an int: %ld\n", sizeof(int)); 
	printf("The size of areas (int[]): %ld\n", sizeof(areas)); 
	printf("The number of ints in areas: %ld\n", sizeof(areas) / sizeof(int)); 
	printf("The first area is %d, then 2nd %d.\n", areas[0], areas[1]);

	return 0;
}
