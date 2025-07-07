

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MAX_SIZE 10 //Define maximum size of the game area

// Structure to store x and y coordinates
typedef struct {
    int x;
    int y;
} Coordinates;

// Structure for shared game data between processes
typedef struct {
    Coordinates player_positions[2]; // Coordinates for 2 players
    Coordinates player_guesses[2];   // Players' guesses
    Coordinates previous_guesses[2];   // Players' previous guesses
    int distances[2];                // Distances of players' guesses from each other
    int round;                       // Current game round
    int game_over;		      // Flag to check if the game is over
    int total_distance[2];           // Total distance after 3 rounds for both players        
} GameSharedData;

// Function to print the game board
void print_board(int size, int board[size][size], int player1_x, int player1_y, int player2_x, int player2_y) {
    // Implementation to print the game board with player positions
    printf("%dx%d map is created\n", size, size);
    printf("A child process created\n");
    printf("Coordinates of the players are chosen randomly\n");
    printf("Player1: [%d,%d] , Player2: [%d,%d]\n", player1_x, player1_y, player2_x, player2_y);
    for (int i = -1; i <= size; i++) {
        for (int j = -1; j <= size; j++) {
            // for corners
            if ((i == -1 || i == size) && (j == -1 || j == size)) {
                printf("+");
            }
            // For upper and lower limits
            else if (i == -1 || i == size) {
                printf("-");
            }
            // For right and left borders
            else if (j == -1 || j == size) {
                printf("|");
            }
            // Players' positions
            else if (i == player1_x && j == player1_y) {
                printf("1");
            } else if (i == player2_x && j == player2_y) {
                printf("2");
            }
            // empty spaces
            else {
                printf(" ");
            }
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]){

	char *name = "CENG 305 PROJECT1_PROCESS";
	int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    	if (shm_fd == -1) {
      	  	perror("Error opening shared memory");
        	return 1;
    	}
    	// Set the size to the size of the GameSharedData structure
    	if (ftruncate(shm_fd, sizeof(GameSharedData)) == -1) {
    	    perror("Error setting size of shared memory");
    	    return 1;
   	 }
    	// Map shared memory to the address space of the process
    	void *ptr = mmap(0, sizeof(GameSharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    	if (ptr == MAP_FAILED) {
      	  perror("Error mapping shared memory");
      	  return 1;
   	 }	
	GameSharedData *game_data = (GameSharedData *)ptr;
	game_data->distances[0]=0;
	game_data->distances[1]=0;
	game_data->round = 1;
	game_data->game_over = 0;
	game_data->total_distance[0]=0;
	game_data->total_distance[1]=0;
	if (argc < 2) {
        fprintf(stderr, "Usage: %s size\n", argv[0]);
        return 1;
   	}
	int size_of_area = atoi(argv[1]);
	if (size_of_area <= 0 || size_of_area > MAX_SIZE) {
        fprintf(stderr, "Invalid size. Size must be between 1 and %d.\n", MAX_SIZE);
        return 1;
    	}
	int board[size_of_area][size_of_area];	
        srand(time(NULL)); // Initialize random number generator
        // 2 coordinates are randomly selected (x,y).
        game_data->player_positions[0].x = rand() % size_of_area;
        game_data->player_positions[0].y = rand() % size_of_area;
        game_data->player_positions[1].x = rand() % size_of_area;
        game_data->player_positions[1].y = rand() % size_of_area;
	print_board(size_of_area, board, game_data->player_positions[0].x, game_data->player_positions[0].y, game_data->player_positions[1].x, game_data->player_positions[1].y);
	printf("Game Launches -->\n");
	//........................................................WE CAN NOW START CREATING CHILDREN WITH FORK............................................................
	
	// We start a loop for a maximum of 3 rounds.
	while(game_data->game_over != 1 && game_data->round < 4){
		printf("\n------ Round-%d ------\n", game_data->round);
		// Create both players via fork
	    	pid_t player1_pid, player2_pid;
	    	// player1 created.
	    	player1_pid = fork();
	    	if (player1_pid == 0) {
	    			// It is necessary to reset rand() time for each player.
	    			srand(time(NULL) ^ (getpid()<<16));
	    			if(game_data->round == 1){
					game_data->player_guesses[0].x = rand() % size_of_area;
					game_data->player_guesses[0].y = rand() % size_of_area;
					
					game_data->previous_guesses[0] = game_data->player_guesses[0];
					game_data->distances[0] = abs(game_data->player_guesses[0].x - game_data->player_positions[1].x) + abs(game_data->player_guesses[0].y - game_data->player_positions[1].y);
				} else{
					//algorithm to reduce the total offset distance in the next step
					int dx = game_data->player_positions[1].x - game_data->previous_guesses[0].x;
    					int dy = game_data->player_positions[1].y - game_data->previous_guesses[0].y;
    					
					int step_x = dx ? (abs(dx) / dx) * (1 + rand() % 2) : 0; // Step size in x coordinate
    					int step_y = dy ? (abs(dy) / dy) * (1 + rand() % 2) : 0; // Step size in y coordinate
					
					// Apply steps in X and Y coordinates
					game_data->player_guesses[0].x += step_x;
					game_data->player_guesses[0].y += step_y;
					
					// Check the borders
					game_data->player_guesses[0].x = (game_data->player_guesses[0].x < 0) ? 0 : (game_data->player_guesses[0].x >= size_of_area) ? size_of_area - 1 : game_data->player_guesses[0].x;
					game_data->player_guesses[0].y = (game_data->player_guesses[0].y < 0) ? 0 : (game_data->player_guesses[0].y >= size_of_area) ? size_of_area - 1 : game_data->player_guesses[0].y;
					
					game_data->previous_guesses[0] = game_data->player_guesses[0];
					game_data->distances[0] = abs(game_data->player_guesses[0].x - game_data->player_positions[1].x) + abs(game_data->player_guesses[0].y - game_data->player_positions[1].y);
				}
	    		printf("%d. guess of player1: [%d,%d]\n",game_data->round, game_data->player_guesses[0].x, game_data->player_guesses[0].y );
	    		printf("the distance with player2: %d\n", game_data->distances[0]);
	    		game_data->total_distance[0] += game_data->distances[0];
	    		
	    		if (game_data->player_guesses[0].x == game_data->player_positions[1].x && game_data->player_guesses[0].y == game_data->player_positions[1].y){
		   		printf("***********************************\n"); 
		   		printf("player1 won the game!!!\n");
		   		printf("***********************************\n");
		   		game_data->game_over = 1;
		   	}
	    		exit(EXIT_SUCCESS);
	    	} else{
			// The main process continues here and creates the second player
			waitpid(player1_pid, NULL, 0);
			//player2 created 
			player2_pid = fork();
			if (player2_pid == 0) {
				// If a player has already won the game, we do not continue.
				if(game_data->game_over == 1){
					exit(EXIT_SUCCESS);
				}
				// It is necessary to reset rand() time for each player.
				srand(time(NULL) ^ (getpid()<<16));

				if(game_data->round == 1){
					game_data->player_guesses[1].x = rand() % size_of_area;
					game_data->player_guesses[1].y = rand() % size_of_area;
					
					game_data->previous_guesses[1] = game_data->player_guesses[1];
					game_data->distances[1] = abs(game_data->player_guesses[1].x - game_data->player_positions[0].x) + abs(game_data->player_guesses[1].y - game_data->player_positions[0].y);

				} else{
					//algorithm to reduce the total offset distance in the next step;
					int dx = game_data->player_positions[0].x - game_data->previous_guesses[1].x;
    					int dy = game_data->player_positions[0].y - game_data->previous_guesses[1].y;
    					
    					int step_x = dx ? (abs(dx) / dx) * (1 + rand() % 2) : 0; // X koordinatında adım büyüklüğü
    					int step_y = dy ? (abs(dy) / dy) * (1 + rand() % 2) : 0; // Y koordinatında adım büyüklüğü
					
					// Perform steps in X and Y coordinates
					game_data->player_guesses[1].x += step_x;
					game_data->player_guesses[1].y += step_y;
					// check borders
					game_data->player_guesses[1].x = (game_data->player_guesses[1].x < 0) ? 0 : (game_data->player_guesses[1].x >= size_of_area) ? size_of_area - 1 : game_data->player_guesses[1].x;
					game_data->player_guesses[1].y = (game_data->player_guesses[1].y < 0) ? 0 : (game_data->player_guesses[1].y >= size_of_area) ? size_of_area - 1 : game_data->player_guesses[1].y;
				
					game_data->previous_guesses[1] = game_data->player_guesses[1];
					game_data->distances[1] = abs(game_data->player_guesses[1].x - game_data->player_positions[0].x) + abs(game_data->player_guesses[1].y - game_data->player_positions[0].y);
				}
				 printf("%d. guess of player2: [%d,%d]\n",game_data->round, game_data->player_guesses[1].x, game_data->player_guesses[1].y );
	    			 printf("the distance with player1: %d\n\n", game_data->distances[1]);
				game_data->total_distance[1] += game_data->distances[1];
				
				if (game_data->player_guesses[1].x == game_data->player_positions[0].x && game_data->player_guesses[1].y == game_data->player_positions[0].y){
					printf("***********************************\n"); 
		   		 	printf("player2 won the game!!!\n");
		   		 	printf("***********************************\n");  
		   		 	game_data->game_over = 1;
		   		 }
				exit(EXIT_SUCCESS); 
	       	 } else {
		   		 // The main process monitors the status of the game and evaluates the results
		   		 int status;
		    		 waitpid(player1_pid, &status, 0);
		   		 waitpid(player2_pid, &status, 0);
			}
		}
		game_data->round++;
	}
	
	if(game_data->game_over != 1 && game_data->total_distance[0] < game_data->total_distance[1]){
	        printf("***********************************\n"); 
		printf("player1 won the game!!!\n");
		printf("***********************************\n");
		game_data->game_over = 1;
	} else if(game_data->game_over != 1 && game_data->total_distance[0] > game_data->total_distance[1]){
	        printf("***********************************\n"); 
		printf("player2 won the game!!!\n");
		printf("***********************************\n");
		game_data->game_over = 1;
	}
	//////////////////////////////////////// Finally, we close all process ////////////////////////////////////////////////////////////////////////////
        // Clean up IPC resources (for example, delete the message queue)
	if (munmap(ptr, sizeof(GameSharedData)) == -1) {
		perror("munmap");
		return 1;
	}
	// Close shared memory object
	close(shm_fd);
	// Delete shared memory object
	shm_unlink(name);  

return 0;
}	



