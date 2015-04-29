#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

const int SCR_WIDTH  = 640;
const int SCR_HEIGHT = 480;
int rows             = 4;
int cols             = 4;
bool shuffled        = false;

SDL_Window  *window  = NULL;
SDL_Surface *surface = NULL;
SDL_Surface *image   = NULL;
SDL_Rect sliding_puzzle[32][32];

void update_window(int[], int);

bool init (void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return false;
    } else {
        window = SDL_CreateWindow("Sliding Puzzle", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, SCR_WIDTH, SCR_HEIGHT,
                              SDL_WINDOW_SHOWN);

        if (window == NULL) {
            printf("Could not create window: %s\n", SDL_GetError());
            return false;
        } else {
            int imgFlags = IMG_INIT_PNG;
            if (!(IMG_Init(imgFlags) & imgFlags)) {
                printf("Could not initialize SDL_image: %s\n", IMG_GetError() );
                return false;
            } else {
                surface = SDL_GetWindowSurface(window);
            }
        }
    }
    return true;
}

SDL_Surface *load_surface (char *path) {
    SDL_Surface *optimized_surface = NULL;
    SDL_Surface *loaded_surface = IMG_Load(path);
    if (loaded_surface == NULL) {
        printf("Unable to load image %s: %s\n", path, IMG_GetError());
    } else {
        optimized_surface = SDL_ConvertSurface(loaded_surface, surface->format, 0);
        if (optimized_surface == NULL) {
            printf("Unable to optimize image %s: %s\n", path, SDL_GetError());
        }
        SDL_FreeSurface(loaded_surface);
    }
    return optimized_surface;
}

bool load_media (char *argv) {
    image = load_surface(argv);
    if (image == NULL) {
        printf("Failed to load PNG image!\n");
        return false;
    }
    int cnk_w = image->w / cols;
    int cnk_h = image->h / rows;
    
    SDL_SetWindowSize(window, image->w, image->h);
    for (int i=0; i<rows; i++) {
        for (int j=0; j<cols; j++) {
            sliding_puzzle[i][j].w = cnk_w;
            sliding_puzzle[i][j].h = cnk_h;
            sliding_puzzle[i][j].x = j * cnk_w;
            sliding_puzzle[i][j].y = i * cnk_h;
        }
    }
    update_window(NULL,0);
    return true;
}

void apply_surface (int x, int y, SDL_Surface* source, SDL_Surface* destination, SDL_Rect* sliding_puzzle) {
    SDL_Rect offset;
    offset.x = x;
    offset.y = y;
    SDL_BlitSurface(source, sliding_puzzle, destination, &offset);
}

void clean_up (void) {
    SDL_FreeSurface(image);
    SDL_DestroyWindow(window);
    image  = NULL;
    window = NULL;
    IMG_Quit();
    SDL_Quit();
}

void update_window (int invis[2], int init) {
    SDL_FillRect(surface, NULL, 0x000000);
    for (int i=0; i<rows; i++) {
        for (int j=0; j<cols; j++) {
            if (init && invis[0] == i && invis[1] == j) continue;
                apply_surface(j * sliding_puzzle[i][j].w + j, i * sliding_puzzle[i][j].h + i,
                              image, surface, &sliding_puzzle[i][j]);
        }
    }
    surface = SDL_GetWindowSurface(window);
    SDL_UpdateWindowSurface(window);
}

void set_clicked_index (int clicked_x, int clicked_y, int *idx_i, int *idx_j, int invis[2]) {
    for (int i=0; i<rows; i++) {
        for (int j=0; j<cols; j++) {
            if (clicked_x >= j * sliding_puzzle[i][j].w + j
             && clicked_x <= j * sliding_puzzle[i][j].w + j + sliding_puzzle[i][j].w
             && clicked_y >= i * sliding_puzzle[i][j].h + i
             && clicked_y <= i * sliding_puzzle[i][j].h + i + sliding_puzzle[i][j].h) {
                if (i == invis[0] && j == invis[1]) continue;
                *idx_i = i;
                *idx_j = j;
            }
        }
    }
}

void swap_tile (int i, int j, int *invis, int order[][cols]) {
    int arr_tmp = order[i][j];
    
    order[i][j] = order[*(invis)][*(invis+1)];
    order[*(invis)][*(invis+1)] = arr_tmp;
    SDL_Rect temp = sliding_puzzle[i][j];
    sliding_puzzle[i][j] = sliding_puzzle[*(invis)][*(invis+1)];
    sliding_puzzle[*(invis)][*(invis+1)] = temp;
    *(invis) = i;
    *(invis + 1) = j;
}

void shuffle_tiles (int order[][cols], int invis[]) {
    for (int i=0; i<1000; i++) {
        int swap_x1 = rand()%rows;
        int swap_y1 = rand()%rows;
        int swap_x2 = rand()%rows;
        int swap_y2 = rand()%rows;
        int arr_tmp = order[swap_x1][swap_y1];
        
        order[swap_x1][swap_y1] = order[swap_x2][swap_y2];
        order[swap_x2][swap_y2] = arr_tmp;
        SDL_Rect temp = sliding_puzzle[swap_x1][swap_y1];
        sliding_puzzle[swap_x1][swap_y1] = sliding_puzzle[swap_x2][swap_y2];
        sliding_puzzle[swap_x2][swap_y2] = temp;
    }
    shuffled = true;
}

int check_won (int tiles[][cols]) {
    for (int i = 0; i < rows * cols; i++) {
        if (tiles[i/rows][i%cols] > tiles[i/rows][i%cols+1]) return 1;
    }
    return 0;
}

int main (int argc, char **argv) {
    srand(time(NULL));
    int idxi, idxj;
    int invis[2] = {rand() % rows, rand() % cols};
    int count = 0;
    int order[rows][cols];
    

    for (int i=0; i<rows; i++) {
        for (int j=0; j<cols; j++) {
            order[i][j] = count++;
        }
    }

    if (argc < 2) {
        printf("You must specify a .png file\nex: %s dog.png\n", argv[0]);
        printf("You may also specify a difficulty: %s dog.png easy\n", argv[0]);
        printf("Valid difficulties: easy, medium, hard, extreme, NOPE\n");
        return 1;
    }

    if (argc == 3) {
        if (strcmp(argv[2], "easy") == 0) {
            rows = 4;
            cols = 4;
        } else if (strcmp(argv[2], "medium") == 0) {
            rows = 6;
            cols = 6;
        } else if (strcmp(argv[2], "hard") == 0) {
            rows = 8;
            cols = 8;
        } else if (strcmp(argv[2], "extreme") == 0) {
            rows = 16;
            rows = 16;
        } else if (strcmp(argv[2], "NOPE") == 0) { 
            rows = 32;
            cols = 32;
        } else { 
            printf("Invalid difficulty\n");
            return 1;
        }
    }
    
    if (!init()) {
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    } else {
        if (!load_media(argv[1])) {
            printf("Failed to load media\n");
            return 1;
        } else {
            bool quit = false;
            SDL_Event e;
            update_window(NULL,0);
            SDL_Delay(3000);
            if (!shuffled) shuffle_tiles(order, invis);
            update_window(NULL,0);
            while (!quit) {
                while (SDL_PollEvent(&e) != 0) {
                    switch (e.type) {
                       case SDL_QUIT:
                           quit = true;
                           break;
                       case SDL_MOUSEBUTTONDOWN:
                           set_clicked_index(e.button.x, e.button.y, &idxi, &idxj, invis);
                           if      (idxi-1 == invis[0] && idxj == invis[1]) swap_tile(idxi, idxj, invis, order);
                           else if (idxi+1 == invis[0] && idxj == invis[1]) swap_tile(idxi, idxj, invis, order);
                           else if (idxi == invis[0] && idxj-1 == invis[1]) swap_tile(idxi, idxj, invis, order);
                           else if (idxi == invis[0] && idxj+1 == invis[1]) swap_tile(idxi, idxj, invis, order);
                           update_window(invis, 1);
                           if (!check_won(order)) {
                                printf("You've solved the puzzle!\n");
                                SDL_Delay(3000);
                                clean_up();
                                return 0;
                           }
                           break;
                    }
                }
                SDL_Delay(100);
            }
        }
    }
    clean_up();
    return 0;
}
