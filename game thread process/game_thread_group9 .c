

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MAX_PLAYERS 10 // Define a constant for the maximum number of players.

// Structure to hold x and y coordinates
typedef struct {
    int x;
    int y;
} Coordinates;

// Structure to hold game shared data
typedef struct {
    int min_distance[MAX_PLAYERS];
    int *threads_player_ids[MAX_PLAYERS];
    int size_of_area;
    int num_players;
    Coordinates player_positions[MAX_PLAYERS];
    Coordinates player_guesses[MAX_PLAYERS][MAX_PLAYERS];;
    Coordinates previous_guesses[MAX_PLAYERS][MAX_PLAYERS];;
    int distances[MAX_PLAYERS][MAX_PLAYERS];
    int round;
    int total_distance[MAX_PLAYERS]; 
    pthread_t threads[MAX_PLAYERS];
    pthread_mutex_t lock;
} GameSharedData;

// Function to print the game board
void print_board(int size, Coordinates players[], int num_players) {
    printf("%dx%d map is created\n", size, size);
    printf("%d threads are created\n", num_players);
    printf("Coordinates of the players are chosen randomly\n");
// Print statements to display the game board with players and their positions 
    for (int i = 0; i < num_players; ++i) {
        printf("Player%d: [%d,%d]", i + 1, players[i].x, players[i].y);
        if(i<num_players-1){
        	printf(" , ");
        }
    }
    printf("\n");
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
            else {
                int printed = 0;
                for (int p = 0; p < num_players; ++p) {
                    if (i == players[p].x && j == players[p].y) {
                        printf("%d", p + 1);
                        printed = 1;
                        break;
                    }
                }
                if (!printed) {
                    printf(" ");
                }
            }
        }
        printf("\n");
    }
}

// Function that each player thread will run to perform their guessing logic
void* player_thread(void* arg) {
   // Convert argument to GameSharedData pointer and perform game logic for guessing positions

    GameSharedData *game_data = (GameSharedData *)arg;
    int thread_index = -1;
    // Find the correct thread index, make guesses, and update the game state
    for (int i = 0; i < game_data->num_players; ++i) {
        if (pthread_self() == game_data->threads[i]) {
            thread_index = i;
            break;
        }
    }
    if (thread_index == -1) {
        // ERROR: Thread index not found
        return NULL;
    }
    int player_id = (game_data->num_players-1) - *game_data->threads_player_ids[thread_index];
    for(int i = 0; i< game_data->num_players; i++){
        if (game_data->round == 1){
	    	game_data->player_guesses[player_id][i].x = rand() % game_data->size_of_area;
		game_data->player_guesses[player_id][i].y = rand() % game_data->size_of_area;						
		game_data->previous_guesses[player_id][i] = game_data->player_guesses[player_id][i];						
		game_data->distances[player_id][i] = abs(game_data->player_guesses[player_id][i].x - game_data->player_positions[(player_id + 1 + i) % game_data->num_players].x) + abs(game_data->player_guesses[player_id][i].y - game_data->player_positions[(player_id + 1 + i) % game_data->num_players].y);    
    } else{ 
    		//algorithm to reduce the total offset distance in the next step;
		int dx = game_data->player_positions[(player_id + 1 + i) % game_data->num_players].x - game_data->previous_guesses[player_id][i].x;
	    	int dy = game_data->player_positions[(player_id + 1 + i) % game_data->num_players].y - game_data->previous_guesses[player_id][i].y;	    					
	    	int step_x = dx ? (abs(dx) / dx) * (1 + rand() % 2) : 0; // Step size in x coordinate
	    	int step_y = dy ? (abs(dy) / dy) * (1 + rand() % 2) : 0; // Step size in y coordinate    		    		
	    	// Perform steps in X and Y coordinates
		game_data->player_guesses[player_id][i].x += step_x;
		game_data->player_guesses[player_id][i].y += step_y;
	    	// Check the borders
		game_data->player_guesses[player_id][i].x = (game_data->player_guesses[player_id][i].x < 0) ? 0 : (game_data->player_guesses[player_id][i].x >= game_data->size_of_area) ? game_data->size_of_area - 1 : game_data->player_guesses[player_id][i].x;
		game_data->player_guesses[player_id][i].y = (game_data->player_guesses[player_id][i].y < 0) ? 0 : (game_data->player_guesses[player_id][i].y >= game_data->size_of_area) ? game_data->size_of_area - 1 : game_data->player_guesses[player_id][i].y;
	    	//save previous
		game_data->previous_guesses[player_id][i] = game_data->player_guesses[player_id][i];				
		game_data->distances[player_id][i] = abs(game_data->player_guesses[player_id][i].x - game_data->player_positions[(player_id + 1 + i) % game_data->num_players].x) + abs(game_data->player_guesses[player_id][i].y - game_data->player_positions[(player_id + 1 + i) % game_data->num_players].y);   
    	}
 
    }
    for(int i=0; i<game_data->num_players; i++){
    	game_data->player_guesses[player_id][game_data->num_players].x += game_data->player_guesses[player_id][i].x;
    	game_data->player_guesses[player_id][game_data->num_players].y += game_data->player_guesses[player_id][i].y;
    }
    game_data->player_guesses[player_id][game_data->num_players].x = game_data->player_guesses[player_id][game_data->num_players].x / (game_data->num_players) -1;
    game_data->player_guesses[player_id][game_data->num_players].y = game_data->player_guesses[player_id][game_data->num_players].y / (game_data->num_players) -1;  
    printf("%d. guess of player%d: [%d,%d]\n", game_data->round, ((player_id)%game_data->num_players+1), game_data->player_guesses[player_id][game_data->num_players].x, game_data->player_guesses[player_id][game_data->num_players].y);
       
    	for(int i=0; i<game_data->num_players; i++){
    		if (i != player_id) {
    			game_data->distances[player_id][i] = abs(game_data->player_guesses[player_id][game_data->num_players].x - game_data->player_positions[i].x ) + abs(game_data->player_guesses[player_id][game_data->num_players].y - game_data->player_positions[i].y);
			printf("the distance with player%d: %d\n", i+1, game_data->distances[player_id][i]);
                }
    	}
    	game_data->min_distance[player_id] = INT_MAX;
    	if(game_data->round == 3){
    		for(int i =0; i<game_data->num_players; i++){
    			if(i != player_id && game_data->distances[player_id][i] < game_data->min_distance[player_id]){
    				game_data->min_distance[player_id] = game_data->distances[player_id][i];	
    			}			
    		}	
    	}
    return NULL;
}

int main(int argc, char *argv[]){

    char *name = "CENG 305 PROJECT1_THREADS";	
	int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
	if (shm_fd == -1) {
	   perror("Error opening shared memory");
	   return 1;
	}
	//  Set the size to the size of the GameSharedData structure
	if (ftruncate(shm_fd, sizeof(GameSharedData)) == -1) {
	   perror("Error setting size of shared memory");
	   return 1;
	}
	//  Map shared memory to the address space of the process
	void *ptr = mmap(0, sizeof(GameSharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (ptr == MAP_FAILED) {
	   perror("Error mapping shared memory");
	   return 1;
	}
		
    GameSharedData *game_data = (GameSharedData *)ptr;    
    game_data->round = 1;    		
    game_data->size_of_area = atoi(argv[1]);
    game_data->num_players = atoi(argv[2]);
	
    if (game_data->num_players < 3) {
        fprintf(stderr, "Usage: %s size num_players\n", argv[0]);
        return 1;
    }
    if (game_data->size_of_area <= 0 || game_data->num_players <= 0 || game_data->num_players > MAX_PLAYERS) {
        fprintf(stderr, "Invalid size or number of players.\n");
        return 1;
    }
    srand(time(NULL)); // Initialize random number generator
    // Generating random locations for players
    for (int i = 0; i < game_data->num_players; ++i) {
	game_data->player_positions[i].x = rand() % game_data->size_of_area;
	game_data->player_positions[i].y = rand() % game_data->size_of_area;    	
    }
    //Print the board and player positions
    print_board(game_data->size_of_area, game_data->player_positions, game_data->num_players);
    printf("Game Launches -->\n");
    //.....................................NOW WE CAN START CREATING Threads WITH creat......................................................................

    while(game_data->round < 4){
    	    printf("\n------ Round-%d ------\n", game_data->round);
    	    // MUTEX open
	    pthread_mutex_init(&game_data->lock, NULL);
	    // Start player threads
	    for (int i = 0; i < game_data->num_players; ++i) {
	        game_data->threads_player_ids[i] = malloc(sizeof(int));
		*game_data->threads_player_ids[i] = i;
		if (pthread_create(&game_data->threads[i], NULL, player_thread, game_data) != 0) {
		    perror("Failed to create thread");
		}
	    }		
	    // Wait for player threads to finish
	    for (int i = 0; i < game_data->num_players; ++i) {
		pthread_join(game_data->threads[i], NULL);	
		}
	    // MUTEX close
	    pthread_mutex_destroy(&game_data->lock);    
    	game_data->round++;
    }
	// Find the lowest total distance
	int min_distance = INT_MAX;
	for (int i = 0; i < game_data->num_players; ++i) {	
		if (game_data->min_distance[i] < min_distance) {
			min_distance = game_data->min_distance[i];
	    	}		    
	}
	// Print winners
	printf("\nThe game ends! \nThe winner with the closest guess of %d-distance:\n", min_distance);
	for (int i = 0; i < game_data->num_players; ++i) {
	    if (game_data->min_distance[i] == min_distance) {
		printf("player%d ", i + 1);
	    }
	}
	printf("\n");
	
//////////////////////////////////////// We close all processes ///////////////////////////////////////////////////////////////////////////

	for (int i = 0; i < game_data->num_players; ++i) {
        free(game_data->threads_player_ids[i]);
        }
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





