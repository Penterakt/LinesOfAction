// LinesOfActionMC.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <time.h>
#include <bitset>
#include <cassert>
#include <string>
#include <queue>
#include <thread>
#include <chrono>
#include <algorithm>

using namespace std;

const int board_size = 8;
const int init_piece_count = 12;
const int black = 1;
const int white = 2;
//directions on the board
const int dir_xs[8] = { 0, 1, 1, 1, 0,-1,-1,-1 };
const int dir_ys[8] = { 1, 1, 0,-1,-1,-1, 0, 1 };

const int dir[16] = {0,1,1,1,0,-1,-1,-1,1,1,0,-1,-1,-1,0,1}; //to get i-th direction read x=dir[i], y = dir[8+i]
//piece counts:,
int horizontal[board_size] = { 0 }; // at i-th row
int vertical[board_size] = { 0 }; // columns
int diagonal[2 * board_size - 1] = { 0 }; //diagonals such that x-y == const, indexed by i = const+board_size
int anti_diagonal[2 * board_size - 1] = { 0 }; //diagonals such that x+y == const indexed by i = const

int possible_moves[96] = {0};

int encode_move(int x, int y, int x1, int y1, int piece_idx) {
    //10000* piece_code + 1000 * x + 100 * y + 10 * x1 + y1;
    return 10000 * piece_idx + 1000 * x + 100 * y + 10 * x1 + y1;
}



void get_locations_of_pieces(int board[][board_size], int player, int pieces_xs[], int pieces_ys[], int &num_of_pieces) {
    num_of_pieces = 0;
    for (int r = 0; r < board_size; r++) {
        for (int c = 0; c < board_size; c++) {
            if (board[r][c] == player) {
                pieces_xs[num_of_pieces] = r;
                pieces_ys[num_of_pieces] = c;
                num_of_pieces += 1;
            }
        }
    }
}

class Game_state {
public:
    int board[board_size][board_size] = {};
    int player_to_move = 1;

    //piece counts
    int horizontal[board_size] = { 0 };
    int vertical[board_size] = { 0 };
    int diagonal[2 * board_size - 1] = { 0 }; //x+y=const
    int anti_diagonal[2 * board_size - 1] = { 0 }; //x-y = const
    //white pieces
    int white_piece_xs[init_piece_count] = {};
    int white_piece_ys[init_piece_count] = {};
    int white_piece_count = 0;
    //black pieces 
    int black_piece_xs[init_piece_count] = {};
    int black_piece_ys[init_piece_count] = {};
    int black_piece_count = 0;
    //location to index in piece array
    int white_location_to_index[board_size][board_size] = { -1 };
    int black_location_to_index[board_size][board_size] = { -1 };
    //int oponent_location_to_index[board_size][board_size] = { 0 };

    //player moving
    int* player_piece_xs;
    int* player_piece_ys;
    int player_piece_count = 0;
    int legal_moves[96] = { 0 };
    int legal_moves_count = 0;
    //oponent
    int * oponent_piece_xs;
    int * oponent_piece_ys;
    int (*player_location_to_index)[board_size];
    int (* oponent_location_to_index)[board_size];
    int * oponent_piece_count_ptr = 0;

    /*

    //quick heuristic to rull out game end, halves are not exactly halves
    // 0=<x<3, we ommit x=3 for purpose, 
    int upper_half_white_count = 0;
    // 4=<x< 8
    int lower_half_white_count = 0;
    // 0=<x<3, we ommit x=3 for purpose, 
    int upper_half_black_count = 0;
    // 4=<x< 8
    int lower_half_black_count = 0;

    // 0=<y<3, we ommit x=3 for purpose, 
    int left_half_white_count = 0;
    // 4=<y< 8
    int right_half_white_count = 0;
    // 0=<y<3, we ommit x=3 for purpose, 
    int left_half_black_count = 0;
    // 4=<y< 8
    int right_half_black_count = 0;

    //int result = 0;
    //these check are for quickly checking that game is not over
    int updown_check_white; //= (lower_half_white_count + upper_half_white_count == white_piece_count) && (lower_half_white_count > 0 && upper_half_white_count > 0);
    int updown_check_black; //= (lower_half_black_count + upper_half_black_count == black_piece_count) && (lower_half_black_count > 0) && (upper_half_black_count > 0);
    int leftright_check_white;
    int leftright_check_black;

    */
    Game_state(int board_[][board_size], int player) {

        for (int r = 0; r < board_size; r++) {
            for (int c = 0; c < board_size; c++) {
                board[r][c] = board_[r][c];
            }
        }

        player_to_move = player;

        //set up piece counts
        for (int r = 0; r < board_size; r++) {
            for (int c = 0; c < board_size; c++) {
                if (board[r][c] != 0) {
                    horizontal[r] += 1;
                    vertical[c] += 1;
                    anti_diagonal[r + c] += 1;
                    diagonal[board_size + r - c] += 1;
                }
            }
        }

        //get locations and count of white pieces
        for (int r = 0; r < board_size; r++) {
            for (int c = 0; c < board_size; c++) {
                if (board[r][c] == 2) {
                    white_piece_xs[white_piece_count] = r;
                    white_piece_ys[white_piece_count] = c;
                    white_location_to_index[r][c] = white_piece_count;
                    white_piece_count += 1;
                    /*
                    if (r < 3) {
                        upper_half_white_count += 1;
                    }
                    if (r >= 4) {
                        lower_half_white_count += 1;
                    }
                    if (c < 3) {
                        left_half_white_count += 1;
                    }
                    if (c >= 4) {
                        left_half_white_count += 1;
                    }
                    */
                }
            }
        }
        //get locations and count of black pieces
        for (int r = 0; r < board_size; r++) {
            for (int c = 0; c < board_size; c++) {
                if (board[r][c] == 1) {
                    black_piece_xs[black_piece_count] = r;
                    black_piece_ys[black_piece_count] = c;
                    black_location_to_index[r][c] = black_piece_count;
                    black_piece_count += 1;
                    /*
                    if (r < 3) {
                        upper_half_black_count += 1;
                    }
                    if (r >= 4) {
                        lower_half_black_count += 1;
                    }
                    if (c < 3) {
                        left_half_black_count += 1;
                    }
                    if (c >= 4) {
                        left_half_black_count += 1;
                    }
                    */
                }
            }
        }

        if (player == black) {
            player_piece_xs = black_piece_xs;
            player_piece_ys = black_piece_ys;
            player_location_to_index = black_location_to_index;
            player_piece_count = black_piece_count;
        }
        else {
            player_piece_xs = white_piece_xs;
            player_piece_ys = white_piece_ys;
            player_location_to_index = white_location_to_index;
            player_piece_count = white_piece_count;
        }

        /*
        //result
        //if check true game is not over if false full calculation has to be performed
          
        updown_check_white = (lower_half_white_count + upper_half_white_count == white_piece_count) && (lower_half_white_count > 0 && upper_half_white_count > 0);
        updown_check_black = (lower_half_black_count + upper_half_black_count == black_piece_count) && (lower_half_black_count > 0) && (upper_half_black_count > 0);
        leftright_check_white = (left_half_white_count + left_half_white_count == white_piece_count) && (left_half_white_count > 0) && (right_half_white_count > 0);
        leftright_check_black = (right_half_black_count + right_half_black_count == black_piece_count) && (left_half_black_count > 0) && (right_half_black_count > 0);
        
        if (player_to_move == 1) {
            if (updown_check_black || leftright_check_black) {
                //std::cout << "worked" << endl;
                result = 0;
            }
            else {
                if (component_length(board, player_to_move, player_piece_xs[0], player_piece_ys[0]) == player_piece_count) {
                    //std::cout << g.player_piece_count << endl;
                    result = player_to_move;
                }
            }
        }else if (player_to_move == 2) {
            if (updown_check_white || leftright_check_white) {
                //std::cout << "worked" << endl;
                result = 0;
            }
            else {
                if (component_length(board, player_to_move, player_piece_xs[0], player_piece_ys[0]) == player_piece_count) {
                    //std::cout << g.player_piece_count << endl;
                    result = player_to_move;
                }
            }
        }
        */
        //cout << "player piece count" << player_piece_count << endl;
        //cout << "component len" <<component_length(board, player_to_move, player_piece_xs[0], player_piece_ys[0]) << endl;
        /*
        if (component_length(board, player_to_move, player_piece_xs[0], player_piece_ys[0]) == player_piece_count) {
            //std::cout << g.player_piece_count << endl;
            result = player_to_move;
        }
        */
        //list_all_moves();
    } //Game_state(position, player)

    Game_state make_move(int move) {
        int piece_index = (move / 10000);
        int x = (move % 10000) / 1000; //first digit
        int y = (move % 1000) / 100; //second digit
        int x1 = (move % 100) / 10; //third
        int y1 = (move % 10); //4th

        int temp = board[x][y];
        board[x1][y1] = temp;
        board[x][y] = 0;
        //cout << "move" << move << endl;
        //cout << "x x1" << x << " " << x1 << endl;
        //update counts
        int oponent = (player_to_move == 1) * 2 + (player_to_move == 2) * 1;
        
        return Game_state(board, oponent);
    }

    
    void make_move_faster(int move) {
        int piece_index = (move / 10000);
        int x = (move % 10000) / 1000; //first digit
        int y = (move % 1000) / 100; //second digit
        int x1 = (move % 100) / 10; //third
        int y1 = (move % 10); //4th

        int oponent = (player_to_move == 1) * 2 + (player_to_move == 2) * 1;
        int one_if_not_capture = (board[x1][y1] != oponent);
        //cout << move << endl;
        //cout << "piece index "<<piece_index << endl;
        int temp = board[x][y];
        board[x1][y1] = temp;
        board[x][y] = 0;

 

        if (player_to_move == black) {
            player_piece_xs = black_piece_xs;
            player_piece_ys = black_piece_ys;
            player_piece_count = black_piece_count;
            player_location_to_index = black_location_to_index;
        }
        else {
            player_piece_xs = white_piece_xs;
            player_piece_ys = white_piece_ys;
            player_piece_count = white_piece_count;
            player_location_to_index = white_location_to_index;
        }
        player_piece_xs[piece_index] = x1;
        player_piece_ys[piece_index] = y1;

        player_location_to_index[x1][y1] = piece_index;
        player_location_to_index[x][y] = -1;

        //update counts
       
        //if capture
        if (!one_if_not_capture) {
            //std::cout << "capture" << endl;
            if (player_to_move == white) {
                oponent_piece_xs = black_piece_xs;
                oponent_piece_ys = black_piece_ys;
                oponent_piece_count_ptr = &black_piece_count;
                oponent_location_to_index = black_location_to_index;
            }
            else {
                oponent_piece_xs = white_piece_xs;
                oponent_piece_ys = white_piece_ys;
                oponent_piece_count_ptr = &white_piece_count;
                oponent_location_to_index = white_location_to_index;
            }
            //what about player?
           
            int index_of_captured_piece = oponent_location_to_index[x1][y1];
            //cout << "index_of_captured_piece" <<index_of_captured_piece << endl;
            oponent_location_to_index[x1][y1] = 0;
            //remove captured piece from piece list, swap with the last piece on the list
            
            oponent_piece_xs[index_of_captured_piece] = oponent_piece_xs[*oponent_piece_count_ptr - 1];
            oponent_piece_ys[index_of_captured_piece] = oponent_piece_ys[*oponent_piece_count_ptr - 1];
            oponent_location_to_index[oponent_piece_xs[index_of_captured_piece]][oponent_piece_ys[index_of_captured_piece]] = index_of_captured_piece;
            
            *oponent_piece_count_ptr -= 1;
            
            horizontal[x1] -= 1;
            vertical[y1] -= 1;
            diagonal[board_size + x1 - y1] -= 1;
            anti_diagonal[x1 + y1] -= 1;
        }
        else {

        }

        //row
        if (x != x1) {
            horizontal[x] -= 1;
            horizontal[x1] += 1;
        }
        //column       
        if (y != y1) {
            vertical[y] -= 1;
            vertical[y1] += 1;
        }

        //diagonal
        if (x - y != x1 - y1) {
            diagonal[board_size + x - y] -= 1;
            diagonal[board_size + x1 - y1] += 1;
        }
        //anti diagonal
        if (x + y != x1 + y1) {
            anti_diagonal[x + y] -= 1;
            anti_diagonal[x1 + y1] += 1;
        }


  

        //switch player
        if (player_to_move == white) {
            player_piece_xs = black_piece_xs;
            player_piece_ys = black_piece_ys;
            player_piece_count = black_piece_count;
        }
        else {
            player_piece_xs = white_piece_xs;
            player_piece_ys = white_piece_ys;
            player_piece_count = white_piece_count;
        }

        player_to_move = (player_to_move == 1) * 2 + (player_to_move == 2) * 1;
    } //make_move_faster

    void list_all_moves(){
        legal_moves_count = 0;
        for (int piece_idx = 0; piece_idx < player_piece_count; piece_idx++) {
            int x = player_piece_xs[piece_idx];
            int y = player_piece_ys[piece_idx];
            //for each direction
            int oponent = (player_to_move == 1) * 2 + (player_to_move == 2) * 1;
            bool still_alive = board[x][y] == player_to_move;
            //cout << "piece" << "(" << x << "," << y << ")" << endl;
            if (still_alive) {
                //std::cout << "alive? " << x << y << endl;

                for (int i = 0; i < 8; i++) {
                    //get move length
                    int move_length = 8;

                    if (dir_xs[i] == 0) {
                        move_length = horizontal[x];
                    }
                    else if (dir_ys[i] == 0) {
                        move_length = vertical[y];
                    }
                    else if (dir_xs[i] + dir_ys[i] == 0) {
                        move_length = anti_diagonal[x + y];
                    }
                    else if (dir_xs[i] - dir_ys[i] == 0) {
                        move_length = diagonal[board_size + x - y];
                    }
                    //std::cout << "x y   " << x << y << "len" << move_length << endl;
                    //std::cout << "dir:  " << dir_xs[i] << dir_ys[i] << "len" << move_length << endl;
                    //std::cout << "move length " << move_length << endl;
                    int x2 = x + move_length * dir_xs[i];
                    int y2 = y + move_length * dir_ys[i];
                    //cout << "x y" << x << y << endl;
                    //cout << "directions" << dir_xs[i] << dir_ys[i] << endl;
                    //cout << "move length" << move_length << endl;
                    //cout << "x2 y2 "<<x2 << y2 << endl;


                    bool is_on_the_board = (x2 >= 0) && (x2 < board_size) && (y2 >= 0) && (y2 < board_size);


                    if (is_on_the_board) {

                        bool not_jumping_over = true;
                        bool does_not_capture_own_piece = (board[x2][y2] != player_to_move);
                        for (int j = 1; j < move_length; j++) {
                            int x1 = x + j * dir_xs[i];
                            int y1 = y + j * dir_ys[i];
                            //checks if (x1,y1) is on the board and does not contain enemy piece
                            if ((x1 >= 0) && (x1 < board_size) && (y1 >= 0) && (y1 < board_size) && (board[x1][y1] == oponent)) {
                                not_jumping_over = false;
                            };
                        }
                        
                        if (not_jumping_over && does_not_capture_own_piece) {
                            //add to possible moves
                            legal_moves[legal_moves_count] = encode_move(x, y, x2, y2, piece_idx); //1000 * x + 100 * y + 10 * x2 + y2;
                            legal_moves_count += 1;
                            //cout << 1000 * x + 100 * y + 10 * x2 + y2 << endl;
                        }
                    }
                }
            }
        }
    }
    

};

int largest_component(Game_state g) {

    int longest_component = 0;
    int already_checked[board_size][board_size] = { 0 };

    for (int i = 0; i < g.player_piece_count; i++) {

        int x = g.player_piece_xs[i];
        int y = g.player_piece_ys[i];

        int q[20];
        int q_start = 0;
        int q_end = 0;

        int already_checked[board_size][board_size] = { 0 };
        int connected_component_len = 0;


        q[q_end] = board_size * x + y; //push
        q_end += 1;

        already_checked[x][y] = 1;
        connected_component_len += 1;

        while (q_end - q_start > 0) {


            x = q[q_start] / board_size;
            y = q[q_start] % board_size;
            q_start += 1; //pop

            //check neighbourhood of piece
            for (int i = 0; i < 8; i++) {
                int x1 = x + dir_xs[i];
                int y1 = y + dir_ys[i];
                if ((x1 >= 0) && (x1 < board_size) && (y1 >= 0) && (y1 < board_size)) {
                    if (g.board[x1][y1] == g.player_to_move) {
                        if (already_checked[x1][y1] == 0) {

                            q[q_start] = board_size * x1 + y1;
                            q_end += 1;

                            already_checked[x1][y1] = 1;
                            connected_component_len += 1;
                        }
                    }

                }
            }

        }

        int result = 0;
        if (connected_component_len > longest_component) {
            longest_component = connected_component_len;
        }
    }

    return longest_component;
}

float largest_compo_heuristic(Game_state g, int move) {
    //int l1 = g.make_move(move);
    return g.legal_moves_count + largest_component(g);//largest_component(g);
}
//returns move
int heuristic_based_random_draw(Game_state g, int collection[], int collection_length, float positive_function(Game_state, int)) {
    float probabilities[50] = { 0 };
    float cumulative[50] = { 0 };
    float sum_of_probs = 0;

    probabilities[0] = positive_function(g, collection[0]);

    cumulative[0] = positive_function(g, cumulative[0]);

    for (int i = 1; i < collection_length; i++) {
        probabilities[i] = positive_function(g, collection[i]);
        cumulative[i] = cumulative[i - 1] + positive_function(g, collection[i]);
    }
    
    for (int i = 0; i < collection_length; i++) {
        cumulative[i] = float(cumulative[i]) / cumulative[collection_length - 1];
    }
    

    float random = ((float)rand()) / RAND_MAX;
    //std:cout << random << endl;
    //find first larger
    int index_of_first_larger = 0;

    while (random > cumulative[index_of_first_larger] && index_of_first_larger < collection_length) {
        index_of_first_larger += 1;
    }
    std::cout << index_of_first_larger << endl;
    std::cout << collection[index_of_first_larger] << endl;

    return collection[index_of_first_larger];
}



int guided_random_move(Game_state g, float heuristic(Game_state,int)) {
    g.list_all_moves();
    cout << g.legal_moves_count << endl;

    int move = heuristic_based_random_draw(g,g.legal_moves, g.legal_moves_count, largest_compo_heuristic);
    return move;
}

void set_up_pieces(int board[][board_size]) {
    for (int i = 1; i < board_size - 1; i++) {
        board[0][i] = 1;
        board[board_size - 1][i] = 1;
        board[i][0] = 2;
        board[i][board_size - 1] = 2;
    }
}



void set_initial_piece_counts(int board[][board_size]) {
    for (int r = 0; r < board_size; r++) {
        for (int c = 0; c < board_size; c++) {
            horizontal[c] = 0;
            vertical[r] = 0;
            anti_diagonal[r + c] = 0;
            diagonal[board_size + r - c] = 0;
        }
    }


    for (int r = 0; r < board_size; r++) {
        for (int c = 0; c < board_size; c++) {
            if (board[r][c] != 0) {
                horizontal[c] += 1;
                vertical[r] += 1;
                anti_diagonal[r + c] += 1;
                diagonal[board_size + r - c] += 1;
            }
        }
    }
}

void diagram(int position[][board_size]) {
    for (int i = 0; i < board_size; i++) {
        for (int j = 0; j < board_size; j++) {
            if (position[i][j] != 0) {
                std::cout << to_string(position[i][j]);
            }
            else {
                std::cout << ".";
            }
        }
        std::cout << endl;
    }
    std::cout << "--------" << endl;
}

void clear(int position[][board_size]) {
    for (int i = 0; i < board_size; i++) {
        for (int j = 0; j < board_size; j++) {
            position[i][j] = 0;
        }
    }
}

void cout_pieces(int pieces_xs[], int pieces_ys[], int count) {
    for (int i = 0; i < count; i++) {
        std::cout << "piece " << i << " (" << pieces_xs[i]<<","<< pieces_ys[i] <<") "<< endl;
    }
}

//moving_player = 1 or 2, piece_counts required! 
bool is_jumping_over(int x, int y, int board[][board_size], int move_length) {
    //for each direction
    int oponent = 2;
    if (board[x][y] == 2) {
        oponent = 1;
    }
  

    for (int i = 0; i < 8; i++) {
        bool result = true;
        //for each square on the line
        for (int j = -move_length + 1; j < move_length; j++) {
            int x1 = x + dir_xs[i];
            int y1 = y + dir_ys[i];

            bool is_jumping_over = (x1 >= 0) && (x1 < board_size) && (y1 >= 0) && (y1 < board_size) && (board[x1][y1] == oponent);
            return is_jumping_over;
        }
    }
}


//move (x,y) -> (x2,y2) coded as num = x y x2 y2 (0 <= num < 7777)

int random_move(Game_state g) {
    //for each direction
    int oponent = 2;
    if (g.player_to_move == 2) {
        oponent = 1;
    }
    //list all moves
    
    
    int possible_moves_last = 0;
    for (int piece_idx = 0; piece_idx < g.player_piece_count; piece_idx++) {
        int x = g.player_piece_xs[piece_idx];
        int y = g.player_piece_ys[piece_idx];
        //for each direction
        //bool still_alive = g.board[x][y] == g.player_to_move;
        //cout << "piece" << "(" << x << "," << y << ")" << endl;
        if (true) {
            //std::cout << "alive? " << x << y << endl;

            for (int i = 0; i < 8; i++) {
                //get move length
                int move_length = 8;

                if (dir_xs[i] == 0) {
                    move_length = g.horizontal[x];
                }
                else if (dir_ys[i] == 0) {
                    move_length = g.vertical[y];
                }
                else if (dir_xs[i] + dir_ys[i] == 0) {
                    move_length = g.anti_diagonal[x + y];
                }
                else if (dir_xs[i] - dir_ys[i] == 0) {
                    move_length = g.diagonal[board_size + x - y];
                }
                //std::cout << "x y   " << x << y << "len" << move_length << endl;
                //std::cout << "dir:  " << dir_xs[i] << dir_ys[i] << "len" << move_length << endl;
                //std::cout << "move length " << move_length << endl;
                int x2 = x + move_length * dir_xs[i];
                int y2 = y + move_length * dir_ys[i];
                //cout << "x y" << x << y << endl;
                //cout << "directions" << dir_xs[i] << dir_ys[i] << endl;
                //cout << "move length" << move_length << endl;
                //cout << "x2 y2 "<<x2 << y2 << endl;


                bool is_on_the_board = (x2 >= 0) && (x2 < board_size) && (y2 >= 0) && (y2 < board_size);

                if (is_on_the_board) {

                    bool not_jumping_over = true;
                    bool does_not_capture_own_piece = (g.board[x2][y2] != g.player_to_move);
                    for (int j = 1; j < move_length; j++) {
                        int x1 = x + j * dir_xs[i];
                        int y1 = y + j * dir_ys[i];
                        //checks if (x1,y1) is on the board and does not contain enemy piece
                        if ((x1 >= 0) && (x1 < board_size) && (y1 >= 0) && (y1 < board_size) && (g.board[x1][y1] == oponent)) {
                            not_jumping_over = false;
                        };
                    }

                    if (not_jumping_over && does_not_capture_own_piece) {
                        //add to possible moves
                        //possible_moves[possible_moves_last] = encode_move(x, y, x2, y2, piece_idx); //10000*piece_idx+1000 * x + 100 * y + 10 * x2 + y2;
                        //possible_moves[possible_moves_last] = 10000*piece_idx + 1000 * x + 100 * y + 10 * x2 + y2;
                        g.legal_moves[possible_moves_last] = 10000 * piece_idx + 1000 * x + 100 * y + 10 * x2 + y2;
                        possible_moves_last += 1;
                        //cout << 1000 * x + 100 * y + 10 * x2 + y2 << endl;
                    }
                }
            }
        }
    }
    


    //cout << possible_moves_last << endl;
    int result = -1; //-1 signals zero possible moves
    if (possible_moves_last > 0) {
        int random_move_idx = rand() % possible_moves_last;
        //result = possible_moves[random_move_idx];
        result = g.legal_moves[random_move_idx];
    }
    else {
        diagram(g.board);
        std::cout << "zero possible moves" << "for player " << g.player_to_move << "\n" << endl;
        cout_pieces(g.player_piece_xs, g.player_piece_ys, 12);
        std::cout << g.player_piece_count << endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    }
    
    //cout << result << "\n" << endl;
    //assert(result < 9999);

   
    return result;
} //random move

int random_move_v3(Game_state g) {
    //for each direction
    int oponent = 2;
    if (g.player_to_move == 2) {
        oponent = 1;
    }
    //list all moves

    
    int possible_moves_last = 0;
    for (int piece_idx = 0; piece_idx < g.player_piece_count; piece_idx++) {
        
        int x = g.player_piece_xs[piece_idx];
        int y = g.player_piece_ys[piece_idx];
        
      
            for (int i = 0; i < 8; i++) {
                //get move length
                int move_length = 8;
                
                if (dir_xs[i] == 0) {
                    move_length = g.horizontal[x];
                }
                else if (dir_ys[i] == 0) {
                    move_length = g.vertical[y];
                }
                else if (dir_xs[i] + dir_ys[i] == 0) {
                    move_length = g.anti_diagonal[x + y];
                }
                else if (dir_xs[i] - dir_ys[i] == 0) {
                    move_length = g.diagonal[board_size + x - y];
                }
                
                int x2 = x + move_length * dir_xs[i];
                int y2 = y + move_length * dir_ys[i];


                bool is_on_the_board = (x2 >= 0) && (x2 < board_size) && (y2 >= 0) && (y2 < board_size);

                if (is_on_the_board) {
                    bool not_jumping_over = true;
                    for (int j = 1; j < move_length; j++) {
                        int x1 = x + j * dir_xs[i];
                        int y1 = y + j * dir_ys[i];
                        //checks if (x1,y1) is on the board and does not contain enemy piece
                        if (g.board[x1][y1] == oponent) {
                            not_jumping_over = false;
                            break;
                        };
                    }

                    if (not_jumping_over) {
                        if (g.board[x2][y2] != g.player_to_move) { //not capturing own piece
                            g.legal_moves[possible_moves_last] = 10000 * piece_idx + 1000 * x + 100 * y + 10 * x2 + y2;
                            possible_moves_last += 1;
                        }
                    }
                }
            }
        
    }
    

  
    //cout << possible_moves_last << endl;
    int result = -1; //-1 signals zero possible moves
    if (possible_moves_last > 0) {
        int random_move_idx = rand() % possible_moves_last;
        //result = possible_moves[random_move_idx];
        result = g.legal_moves[random_move_idx];
    }
    else {
        diagram(g.board);
        std::cout << "zero possible moves" << "for player " << g.player_to_move << "\n" << endl;
        cout_pieces(g.player_piece_xs, g.player_piece_ys, 12);
        std::cout << g.player_piece_count << endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    }

    //cout << result << "\n" << endl;
    //assert(result < 9999);


    return result;
} //random move

//directional counts
void update_directional_piece_counts(int board[][board_size], int move) {

    //decode move
    int x = move / 1000; //first digit
    int y = (move % 1000) / 100; //second digit
    int x1 = (move % 100) / 10; //third
    int y1 = (move % 10); //4th

    int one_if_not_capture = (board[x1][y1] != 0);
    //row
    if (x != x1) {
        horizontal[x] -= 1;
        horizontal[x1] += one_if_not_capture;
    }
    //column       
    if (y != y1) {
        vertical[y] -= 1;
        vertical[y1] += one_if_not_capture;
    }
    
    //diagonal
    if(x-y!=x1-y1){
        diagonal[board_size+x-x1] -= 1;
        diagonal[board_size+x1-y1] += one_if_not_capture;
    }
    //anti diagonal
    if (x + y != x1 + y1){
        anti_diagonal[x + x1] -= 1;
        anti_diagonal[x1 + y1] += one_if_not_capture;
    }
}

//let first two digits of move code piece number assigned at the start
void update_piece_locations(int board[][board_size], int piece_xs[], int piece_ys[], int move) {
    //decode move
    int piece_index = (move / 10000);
    int x = (move % 10000)/1000; //first digit
    int y = (move % 1000) / 100; //second digit
    int x1 = (move % 100) / 10; //third
    int y1 = (move % 10); //4th
    assert(x < board_size);
    assert(y < board_size);
    assert(x1 < board_size);
    assert(y1 < board_size);


    piece_xs[piece_index] = x1;
    piece_ys[piece_index] = y1;
};


//void play_the_move(int board[][board_size], int move) {
   // int pos = read_move(move,"idx");
   // int x = read_move(move, "x");
   // int y = read_move(move, "x");
   // int x1 = read_move(move, "x1");
   // int y1 = read_move(move, "y1");
   // int value = board[x][y];
    //update the board
    //board[x][y] = value;
    //board[x][y] = 0;
    //board[x1][y1] = value;
//}


//three options: 0 - game is still going,1 - black won, 2- white won
int game_result(int board[][board_size], int player_pieces_xs[], 
    int player_pieces_ys[], int player_piece_cnt, int player) 
{    
    int result = 0;
    //int oponent = (player == 1) * 2 + (player == 2) * 1;
    //get any piece that's still alive
    int idx = 0;
    while (board[player_pieces_xs[idx]][player_pieces_ys[idx]]!=player && idx < player_piece_cnt) {
        idx += 1;
    }
    
    int x = player_pieces_xs[idx];
    int y = player_pieces_ys[idx];

    queue<int> q_xs;
    queue<int> q_ys;

    int component_xs[12];
    int component_ys[12];
    int compo_len = 0;

    q_xs.push(x);
    q_ys.push(y);
    component_xs[compo_len] = x;
    component_ys[compo_len] = y;
    compo_len += 1;
    
    //to be edited
    int brute_piece_count = 0;
    for (int i = 0; i < board_size; i++) {
        for (int j = 0; j < board_size; j++) {
            if (board[i][j] == player) {
                brute_piece_count += 1;
            }
        }
    }

    while (!q_xs.empty()) {
        
        //check neighbourhood of piece
        x = q_xs.front(); q_xs.pop();
        y = q_ys.front(); q_ys.pop();

        for (int i = 0; i < 8; i++) {
            int x1 = x + dir_xs[i];
            int y1 = y + dir_ys[i];
            if ((x1 >= 0) && (x1 < board_size) && (y1 >= 0) && (y1 < board_size)) {
                //std::cout << "x1 y1 " << x1 << " " << y1 << endl;
                if (board[x1][y1] == player) {
                    bool not_in_compo = true;
                    for (int j = 0; j < compo_len; j++) {
                        not_in_compo *= !(component_xs[j] == x1 && component_ys[j] == y1);
                    }

                    if (not_in_compo) {
                        q_xs.push(x1);
                        q_ys.push(y1);

                        component_xs[compo_len] = x1;
                        component_ys[compo_len] = y1;
                        compo_len += 1;
                    }
                }
            }
        }
        //std::cout << "compo len"<< compo_len << endl;
        //std::cout << brute_piece_count << endl;
        //std::cout << player_piece_cnt << endl;
        if (compo_len == player_piece_cnt) {
            //std::cout << player_piece_cnt << endl;
            result = player;
        }
    }
    
    return result;
}

int game_result(Game_state g) {

    int x = g.player_piece_xs[0];
    int y = g.player_piece_ys[0];


    int q[20];
    int q_start=0; 
    int q_end=0; //first uninitialised
    

    int already_checked[board_size][board_size] = {0};// = { 0 };
    int connected_component_len = 0;


    q[q_end] = board_size * x + y; //push
    q_end += 1;

    already_checked[x][y] = 1;
    connected_component_len += 1;

    
    while (q_end-q_start>0) {
    
        
        x = q[q_start]/board_size;
        y = q[q_start]%board_size;
        q_start +=1; //pop
        
        //check neighbourhood of piece
        for (int i = 0; i < 8; i++) {
            int x1 = x + dir_xs[i];
            int y1 = y + dir_ys[i];
            if ((x1 >= 0) && (x1 < board_size) && (y1 >= 0) && (y1 < board_size)) {
                if (g.board[x1][y1] == g.player_to_move) {
                    if (already_checked[x1][y1] == 0) {

                        q[q_start] = board_size * x1 + y1;
                        q_end += 1;

                        already_checked[x1][y1] = 1;
                        connected_component_len += 1;
                    }
                }
        
            }
        }
    }
    
    int result = 0;
    if (connected_component_len == g.player_piece_count) {
        result = g.player_to_move;
    }
   
    return result;
}


int perform_rollout(Game_state g, int maxT=800) {
    int move = 0;
    int result = game_result(g);

    int simLen = 0;
    //
    while (result == 0 && simLen < maxT) {
        /*
        move = random_move(g);
        
        int piece_index = (move / 10000);
        int x = (move % 10000) / 1000; //first digit
        int y = (move % 1000) / 100; //second digit
        int x1 = (move % 100) / 10; //third
        int y1 = (move % 10); //4th

        g.board[x1][y1] = g.board[x][y];
        g.board[x][y] = 0;
        */
        
        //std::cout << "player to move" << g.player_to_move << endl;
        //diagram(g.board);
        //scout<<
        g = g.make_move(random_move(g));
        result = game_result(g);
    }
    //std::cout << result << endl;
    //diagram(g.board);
    return result;
}

int perform_rollout_v2(Game_state g, int maxT, float heuristic(Game_state, int)) {
    int move = 0;
    int result = game_result(g);

    int simLen = 0;
    //
    while (result == 0 && simLen < maxT) {
        /*
        move = random_move(g);

        int piece_index = (move / 10000);
        int x = (move % 10000) / 1000; //first digit
        int y = (move % 1000) / 100; //second digit
        int x1 = (move % 100) / 10; //third
        int y1 = (move % 10); //4th

        g.board[x1][y1] = g.board[x][y];
        g.board[x][y] = 0;
        */

        //std::cout << "player to move" << g.player_to_move << endl;
        //diagram(g.board);
        //scout<<
        g = g.make_move(guided_random_move(g,heuristic));
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //Game_state g1(g.board,player);
        result = game_result(g);
    }
    //std::cout << result << endl;
    //diagram(g.board);
    return result;
}

int perform_rollout_v3(Game_state g, int maxT=800) {
    int move = 0;
    int result = game_result(g);

    int simLen = 0;
    
    //Game_state g1(g.board, g.player_to_move);
    /*
    bool check_h = true;
    bool check_v = true;
    bool check_d = true;
    bool check_a = true;
    bool check_count = true;
    */
    while (result == 0 && simLen < maxT) {

        //std::cout << "player to move" << g.player_to_move << endl;
        //diagram(g.board);
        //std::cout << "black pc"<<g.black_piece_count << " "<< g1.black_piece_count << endl;
        //std::cout <<"white pc" << g.white_piece_count << " " << g1.white_piece_count << endl;
        //if (g.black_piece_count != g1.black_piece_count) { check_count = false; }
        //if (g.white_piece_count != g1.white_piece_count) { check_count = false; }
        /*
        for (int j = 0; j < board_size; j++) {
            for (int i = 0; i < board_size; i++) {
                if (g.horizontal[j] != g1.horizontal[j]) { check_h = false; }
                if (g.vertical[j] != g1.vertical[j]) { check_v = false; }
                if (g.diagonal[board_size + i-j] != g1.diagonal[board_size + i-j]) { check_d = false; }
                if (g.anti_diagonal[i + j] != g1.anti_diagonal[i + j]) { check_a = false; }
            }
        }

        for (int i = 0; i < 2*board_size-1; i++) {
            cout << "adiag: " << i << "  " << g.anti_diagonal[i] << "  " << g1.anti_diagonal[i] << endl;
        }
        */
        /*
        bool foundall = true;
        for (int i = 0; i < g.black_piece_count;i++ ) {
            bool not_found = true;
            int x = g.black_piece_xs[i];
            int y = g.black_piece_ys[i];
            for (int j = 0; j < g1.black_piece_count; j++) {
                int x1 = g1.black_piece_xs[j];
                int y1 = g1.black_piece_ys[j];
                if ((x == x1) && (y == y1)) { not_found = not_found*false; }
            }
            foundall = foundall * (!not_found);
        }
        */
        /*
        for (int j = 0; j < g.black_piece_count; j++) {
            cout << "("<< g.black_piece_xs[j] <<" "<<g.black_piece_ys[j]<<") ";
        }
        cout << endl;

        for (int j = 0; j < g1.black_piece_count; j++) {
            cout << "(" << g1.black_piece_xs[j] << " " << g1.black_piece_ys[j] << ") ";
        }
        cout << endl;

        for (int j = 0; j < g.white_piece_count; j++) {
            cout << "(" << g.white_piece_xs[j] << " " << g.white_piece_ys[j] << ") ";
        }
        cout << endl;

        for (int j = 0; j < g1.white_piece_count; j++) {
            cout << "(" << g1.white_piece_xs[j] << " " << g1.white_piece_ys[j] << ") ";
        }
        cout << endl;
        diagram(g.white_location_to_index);
        diagram(g.black_location_to_index);
        std::cout << "check" << check_h <<check_v <<check_d << check_a<<check_count<<foundall <<endl;
        if(!(check_h&&check_v&&check_d&&check_a&check_count&& foundall)){ std::this_thread::sleep_for(std::chrono::milliseconds(3000)); }
        */
        int r = random_move_v3(g);
        g.make_move_faster(r);
        //g1 = g1.make_move(r);


        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //Game_state g1(g.board,player);
        result = game_result(g);
    }
    //std::cout << result << endl;
    //diagram(g.board);
    return result;
}

bool test1() {
    //game result function
    int player_pieces_xs[12];
    int player_pieces_ys[12];
    int player_piece_count = 0;
    int player = 1;

    //Case 1: only one white piece
    int board[board_size][board_size] = { 0 };
    board[1][1] = 1;
    Game_state g1(board, 1);
    diagram(board);

    int r1 = game_result(g1);
    bool case1 = (r1 == 1);
    std::cout<<"case 1: "<<case1<<endl;
    
    //2nd case: one black piece
    clear(board);
    board[1][1] = 2;
    player = 2;
    Game_state g2(board, player);
    diagram(board);
    
    
    int r2 = game_result(g2);
    bool case2 = (r2 == 2);
    std::cout << "case 2: " << case2 << endl;
    
    //3rd case: three black pieces
    clear(board);
    board[1][1] = 2;
    board[1][2] = 2;
    board[2][3] = 2;
    player = 2;
    diagram(board);
    
    Game_state g3(board, player);
    int r3 = game_result(g3);
    
    bool case3 = (r3 == 2);
    std::cout << "case 3 "<<case3 << endl;
    
    //case 4: three black pieces and 2 white
    clear(board);
    board[1][1] = 2;
    board[1][2] = 2;
    board[2][5] = 2;
    board[3][3] = 1;
    board[3][5] = 1;
    player = 1;
    diagram(board);
    get_locations_of_pieces(board, player, player_pieces_xs, player_pieces_ys, player_piece_count);

    Game_state g4(board, player);
    int r4 = game_result(g4);

    bool case4 = (r4 == 0);
    std::cout << "case4 " << case4 << endl;
    
    //case 5 random moves

    clear(board);
    board[1][1] = 1;
    board[1][2] = 2;
    std::cout<<"case"<<5<<endl;
    diagram(board);
    player = 2;
    Game_state g5(board, player);
    int r5 = game_result(g5);

    
    for (int i = 0; i < 100; i++) {
        int r = random_move(g5);
        int x = (r % 10000) / 1000;
        int y = (r % 1000) / 100;
        int x1 = (r % 100) / 10;
        int y1 = r % 10;
        //cout << "move code" << r << endl;
        //cout << "random move" << x1 << " " << y1 << endl;
        //cout << "hori"<< horizontal[1] << endl;
        //cout << "diag" << diagonal[0] << endl;
        if (r != -1) {
            g5.board[x1][y1] = 9;
        }
    }
    
    diagram(g5.board);
    
    //case 6 random moves

    clear(board);
    board[1][1] = 1;
    board[1][3] = 2;
    diagram(board);
    player = 2;
    Game_state g6(board, player);


    for (int i = 0; i < 100; i++) {
        int r = random_move(g6);
        int x = (r % 10000) / 1000;
        int y = (r % 1000) / 100;
        int x1 = (r % 100) / 10;
        int y1 = r % 10;
        //cout << "move code" << r << endl;
        //cout << "random move" << x1 << " " << y1 << endl;
        //cout << "hori"<< horizontal[1] << endl;
        //cout << "diag" << diagonal[0] << endl;
        if (r != -1) {
            board[x1][y1] = 9;
        }
    }
    cout <<"hori 1 " << g6.horizontal[1] << endl;
    diagram(board);
    
    //case 7 random moves

    clear(board);
    board[0][0] = 2;
    board[1][0] = 2;
    board[1][1] = 2;
    board[1][2] = 2;
    board[1][5] = 2;
    board[2][0] = 2;
    board[2][1] = 1;
    board[3][3] = 1;
    board[4][1] = 1;

    diagram(board);
    player = 2;
    Game_state g7(board, player);


    for (int i = 0; i < 100; i++) {
        int r = random_move(g7);
        int x = (r % 10000) / 1000;
        int y = (r % 1000) / 100;
        int x1 = (r % 100) / 10;
        int y1 = r % 10;
        //cout << "move code" << r << endl;
        //cout << "random move" << x1 << " " << y1 << endl;
        //cout << "hori"<< horizontal[1] << endl;
        //cout << "diag" << diagonal[0] << endl;
        if (r != -1) {
            board[x1][y1] = 9;
        }
    }

    diagram(board);


    //case 7 random moves

    clear(board);
    board[0][0] = 2;
    board[1][0] = 2;
    board[2][0] = 2;
    board[7][0] = 2;
    board[7][1] = 2;
    board[7][2] = 2;
    //board[2][1] = 1;
    //board[3][3] = 1;
    //board[4][1] = 1;

    diagram(board);
    player = 2;
    Game_state g8(board, player);


    for (int i = 0; i < 100; i++) {
        int r = random_move(g8);
        int x = (r % 10000) / 1000;
        int y = (r % 1000) / 100;
        int x1 = (r % 100) / 10;
        int y1 = r % 10;
        //cout << "move code" << r << endl;
        //cout << "random move" << x1 << " " << y1 << endl;
        //cout << "hori"<< horizontal[1] << endl;
        //cout << "diag" << diagonal[0] << endl;
        if (r != -1) {
            g8.board[x1][y1] = 9;
        }
    }

    diagram(g8.board);
    
   
    return true;
}



//returns player winrate in a position
float estimate_winrate(Game_state g, int rollouts, int max_depth=1000) {
    
    //std::cout << "player to move " << g.player_to_move << endl;
    //diagram(g.board);


    int res = perform_rollout(g,max_depth);
    //std::cout << "rollout result"<< res << endl;

    int white_wins = 0;
    int black_wins = 0;
    int timed_out = 0;
    int player = g.player_to_move;
    float result = 0;
    for (long k = 0; k < rollouts; k++) {

        int outcome = perform_rollout(g);
        //std::cout << "rollout result " << outcome << endl;
        if (outcome == 0) {
            timed_out += 1;
        }
        if (outcome == 2) {
            white_wins += 1;
        }
        if (outcome == 1) {
            black_wins += 1;
        }

        int conclusive_games_count = white_wins + black_wins;
        if (player == 1) {
            result = (float)black_wins / conclusive_games_count;
        }
        else {
            result = (float)white_wins / conclusive_games_count;
        }
    }
    return result;
}

int basic_monte_carlo(int board[board_size][board_size], int player_to_move,int rolls_per_move) {

    int player = player_to_move;
    Game_state g(board, player_to_move);
    int possible_moves_last = 0;
    int possible_moves[96] = {-5};
    int oponent = (g.player_to_move==1)*2+ (g.player_to_move == 2) * 1;
    
    for (int piece_idx = 0; piece_idx < g.player_piece_count; piece_idx++) {
        int x = g.player_piece_xs[piece_idx];
        int y = g.player_piece_ys[piece_idx];
        //for each direction
        bool still_alive = g.board[x][y] == g.player_to_move;
        //cout << "piece" << "(" << x << "," << y << ")" << endl;
        if (still_alive) {
            //std::cout << "alive? " << x << y << endl;

            for (int i = 0; i < 8; i++) {
                //get move length
                int move_length = 8;

                if (dir_xs[i] == 0) {
                    move_length = g.horizontal[x];
                }
                else if (dir_ys[i] == 0) {
                    move_length = g.vertical[y];
                }
                else if (dir_xs[i] + dir_ys[i] == 0) {
                    move_length = g.anti_diagonal[x + y];
                }
                else if (dir_xs[i] - dir_ys[i] == 0) {
                    move_length = g.diagonal[board_size + x - y];
                }
                //std::cout << "x y   " << x << y << "len" << move_length << endl;
                //std::cout << "dir:  " << dir_xs[i] << dir_ys[i] << "len" << move_length << endl;
                //std::cout << "move length " << move_length << endl;
                int x2 = x + move_length * dir_xs[i];
                int y2 = y + move_length * dir_ys[i];
                //cout << "x y" << x << y << endl;
                //cout << "directions" << dir_xs[i] << dir_ys[i] << endl;
                //cout << "move length" << move_length << endl;
                //cout << "x2 y2 "<<x2 << y2 << endl;


                bool is_on_the_board = (x2 >= 0) && (x2 < board_size) && (y2 >= 0) && (y2 < board_size);


                if (is_on_the_board) {

                    bool not_jumping_over = true;
                    bool does_not_capture_own_piece = (g.board[x2][y2] != g.player_to_move);
                    for (int j = 1; j < move_length; j++) {
                        int x1 = x + j * dir_xs[i];
                        int y1 = y + j * dir_ys[i];
                        //checks if (x1,y1) is on the board and does not contain enemy piece
                        if ((x1 >= 0) && (x1 < board_size) && (y1 >= 0) && (y1 < board_size) && (g.board[x1][y1] == oponent)) {
                            not_jumping_over = false;
                        };
                    }

                    if (not_jumping_over && does_not_capture_own_piece) {
                        //add to possible moves
                        possible_moves[possible_moves_last] = encode_move(x, y, x2, y2, piece_idx); //1000 * x + 100 * y + 10 * x2 + y2;
                        possible_moves_last += 1;
                        //cout << 1000 * x + 100 * y + 10 * x2 + y2 << endl;
                    }
                }
            }
        }
    }

    float oponent_winrate = 0;
    float lowest_oponent_winrate = 1.0;
    //std::cout << "possible moves " << possible_moves_last << endl;
    int best_move = 0;
    //Game_state g2(g.board, player);

    //diagram(g2.board);
    for (int j = 0; j < possible_moves_last; j++) {
        int move = possible_moves[j];
        
        //player = (player == 1) * 2 + (player == 2) * 1;
        //std::cout << "my pieces "<< Game_state(board, player_to_move).player_to_move << endl;
        //std::cout << "position" << endl;

        //diagram(board);
        Game_state g1 = Game_state(board, player_to_move).make_move(move); //(g.board, player);
        
        //std::cout <<"analyzing winrates of "<< g1.player_to_move << "move: "<<j <<endl;
        oponent_winrate = estimate_winrate(g1, rolls_per_move);
        
        
        if (oponent_winrate < lowest_oponent_winrate) {
            lowest_oponent_winrate = oponent_winrate;
            best_move = move;
        }
    }
    //std::cout << "piece counts" << g.white_piece_count << " " << g.black_piece_count << endl;
    //std::cout <<"best move: "<< best_move<<"  wr  "<<1.0- lowest_oponent_winrate <<endl;
    return best_move;
}

int basic_monte_carlo_early_cutoff(int board[board_size][board_size], int player_to_move, int rolls_per_move,int max_depth) {

    int player = player_to_move;
    Game_state g(board, player_to_move);
    int possible_moves_last = 0;
    int possible_moves[96] = { -5 };
    int oponent = (g.player_to_move == 1) * 2 + (g.player_to_move == 2) * 1;

    for (int piece_idx = 0; piece_idx < g.player_piece_count; piece_idx++) {
        int x = g.player_piece_xs[piece_idx];
        int y = g.player_piece_ys[piece_idx];
        //for each direction
        bool still_alive = g.board[x][y] == g.player_to_move;
        //cout << "piece" << "(" << x << "," << y << ")" << endl;
        if (still_alive) {
            //std::cout << "alive? " << x << y << endl;

            for (int i = 0; i < 8; i++) {
                //get move length
                int move_length = 8;

                if (dir_xs[i] == 0) {
                    move_length = g.horizontal[x];
                }
                else if (dir_ys[i] == 0) {
                    move_length = g.vertical[y];
                }
                else if (dir_xs[i] + dir_ys[i] == 0) {
                    move_length = g.anti_diagonal[x + y];
                }
                else if (dir_xs[i] - dir_ys[i] == 0) {
                    move_length = g.diagonal[board_size + x - y];
                }
                //std::cout << "x y   " << x << y << "len" << move_length << endl;
                //std::cout << "dir:  " << dir_xs[i] << dir_ys[i] << "len" << move_length << endl;
                //std::cout << "move length " << move_length << endl;
                int x2 = x + move_length * dir_xs[i];
                int y2 = y + move_length * dir_ys[i];
                //cout << "x y" << x << y << endl;
                //cout << "directions" << dir_xs[i] << dir_ys[i] << endl;
                //cout << "move length" << move_length << endl;
                //cout << "x2 y2 "<<x2 << y2 << endl;


                bool is_on_the_board = (x2 >= 0) && (x2 < board_size) && (y2 >= 0) && (y2 < board_size);


                if (is_on_the_board) {

                    bool not_jumping_over = true;
                    bool does_not_capture_own_piece = (g.board[x2][y2] != g.player_to_move);
                    for (int j = 1; j < move_length; j++) {
                        int x1 = x + j * dir_xs[i];
                        int y1 = y + j * dir_ys[i];
                        //checks if (x1,y1) is on the board and does not contain enemy piece
                        if ((x1 >= 0) && (x1 < board_size) && (y1 >= 0) && (y1 < board_size) && (g.board[x1][y1] == oponent)) {
                            not_jumping_over = false;
                        };
                    }

                    if (not_jumping_over && does_not_capture_own_piece) {
                        //add to possible moves
                        possible_moves[possible_moves_last] = encode_move(x, y, x2, y2, piece_idx); //1000 * x + 100 * y + 10 * x2 + y2;
                        possible_moves_last += 1;
                        //cout << 1000 * x + 100 * y + 10 * x2 + y2 << endl;
                    }
                }
            }
        }
    }

    float oponent_winrate = 0;
    float lowest_oponent_winrate = 1.0;
    //std::cout << "possible moves " << possible_moves_last << endl;
    int best_move = 0;
    //Game_state g2(g.board, player);

    //diagram(g2.board);
    for (int j = 0; j < possible_moves_last; j++) {
        int move = possible_moves[j];

        //player = (player == 1) * 2 + (player == 2) * 1;
        //std::cout << "my pieces "<< Game_state(board, player_to_move).player_to_move << endl;
        //std::cout << "position" << endl;

        //diagram(board);
        Game_state g1 = Game_state(board, player_to_move).make_move(move); //(g.board, player);

        //std::cout <<"analyzing winrates of "<< g1.player_to_move << "move: "<<j <<endl;
        oponent_winrate = estimate_winrate(g1, rolls_per_move, max_depth);


        if (oponent_winrate < lowest_oponent_winrate) {
            lowest_oponent_winrate = oponent_winrate;
            best_move = move;
        }
    }
    //std::cout << "piece counts" << g.white_piece_count << " " << g.black_piece_count << endl;
    //std::cout <<"best move: "<< best_move<<"  wr  "<<1.0- lowest_oponent_winrate <<endl;
    return best_move;
}

void test2() {
    int board[board_size][board_size] = { 0 };
    set_up_pieces(board);

    std::cout <<" rand max " <<RAND_MAX <<endl;
    auto t = clock();


    int N = 50;
    int player = 1;



    int move = 0;
    int maxSimulations = 800;
    //estimate_winrate(Game_state g, int rollouts)
    int black_score = 0;
    int white_score = 0;
    int budget = 2000;

    for (int i = 0; i < N; i++) {
        Game_state g(board, 1);
        
        int move_in_the_game = 1;
        while (game_result(g) == 0) {
            cout << "player to move" << g.player_to_move <<" move nr " << move_in_the_game << endl;
            cout << "black: " << black_score << " white: " << white_score << "black percentage: "<< 100*(float)black_score/(white_score+black_score)<<" %"<<endl;
            //diagram(g.board);

             
            if (g.player_to_move == 1) {
                //move = basic_monte_carlo(g.board, g.player_to_move, 2);
                move = random_move(g);//basic_monte_carlo_early_cutoff(g.board, g.player_to_move, 10, 10);
            }
            else {
                
                move = guided_random_move(g, largest_compo_heuristic);//basic_monte_carlo_early_cutoff(g.board, g.player_to_move, 50,2);
            }
            g.list_all_moves();
            g = g.make_move(move);
            move_in_the_game += 1;
        }
        //cout << "player " << game_result(g)<<" won " << endl;
        if (game_result(g) == 1) { black_score += 1; }else if (game_result(g) == 2) { white_score += 1; }
        
        cout << "black: " << black_score << " white: " << white_score << endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }


    t = clock() - t;
    std::cout << "total time " << ((float)t) / CLOCKS_PER_SEC << endl;
    std::cout << N / (((float)t) / CLOCKS_PER_SEC) << " /second" << endl;
}

class cl {
    public:
        int a = 10;
        int b = 20;
};
int fun() {
    return 0;
}

void test3() {
    int board[board_size][board_size] = { 0 };
    set_up_pieces(board);


    int N = 10000;
    int player = 1;



    int move = 0;
    int maxSimulations = 8000;
    //estimate_winrate(Game_state g, int rollouts)
    int black_score = 0;
    int white_score = 0;
   
    Game_state g(board, 1);
    int r = random_move(g);
    int ax[64] = {0};
    int bx[64] = { 0 };
    cl x;

    auto t = clock();
    for (int i = 0; i < N; i++) {
        //g.make_move_faster(r);
        //perform_rollout(g); //1100 per sec
        perform_rollout_v3(g);  //1600 per sec->2300/sec
        for (int j = 0; j < 200; j++) {
            //fun();
            //x.a;
        //rand();
        //int b[3] = {0};
            //ax[16] = i;
            //bx[16] = j;
            //i % 10;
        //}
            /*
            g.list_all_moves();
            int r = rand() % g.legal_moves_count;
            g.player_legal_moves[r];
            */
            
            //g.make_move_faster(1133);       //28500 for 200
            //g.make_move(1010);              //1400 for 200
            //g = Game_state(board, 1);       //1500 for 200 -> 4.2k per 200
            //random_move(g);                 //1100 for 200 ->2k for 200
            //rand();                         //20k for 200
            //random_move_v3(g);                //3.4k for 200
            //int r =random_move(g);

        }
        /*
        
        g.list_all_moves();
        int r = rand() % g.legal_moves_count;
        cout<<g.player_legal_moves[5]<<endl;


        int move_in_the_game = 1;
        while (game_result(g) == 0) {
            move = random_move(g);
            g = g.make_move(move);
            move_in_the_game += 1;
        }
        //cout << "player " << game_result(g)<<" won " << endl;
        if (game_result(g) == 1) { black_score += 1; }
        else if (game_result(g) == 2) { white_score += 1; }

        cout << "black: " << black_score << " white: " << white_score << endl;
        cout << "length " << move_in_the_game << endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        */
    }


    t = clock() - t;

    std::cout << "total time " << ((float)(t)) / CLOCKS_PER_SEC << endl;
    std::cout << N / (((float)t) / CLOCKS_PER_SEC) << " /second" << endl;
}

int fun(int x) {
    return exp(x);
}

void test4() {
 

    
   
    
}

void tests()
{
    bool t1 = test1();
    bool all_good = t1;
    if (all_good) {
        std::cout << "lgtm" << endl;
    }
    else {
        std::cout << "fail" << endl;
    }
}


int read_python_move(string arg) {
    return stoi(arg);
}

int main(int argc, char** argv)
{
    int rolls_per_move = 100;
    auto t = clock();
    if (argc > 1) {

        int position[board_size][board_size] = { 0 };
        //string s = argv[1];
        char* position_ = argv[1];
        int last_move = stoi(argv[2]);
        int player = stoi(argv[3]);
        //string position_ = argv[1];
        for (int r = 0; r < board_size; r++) {
            for (int c = 0; c < board_size; c++) {
              
                if (position_[8 * r + c] == '0') {
                    position[r][c] = 0;
                }
                if (position_[8 * r + c] == '1') {
                    position[r][c] = 1;
                }
                if (position_[8 * r + c] == '2') {
                    position[r][c] = 2;
                }

            }
        }
        
        //cout<<"player"<<player<<endl;
        //cout << last_move << endl;
        //diagram(position);
        //player = (player == 1) * 2 + (player == 2) * 1;
        Game_state g(position, player);
        int move = basic_monte_carlo(position, player, rolls_per_move);//random_move(g) % 10000;
        std:: cout << move << endl;

    }
    //test4();
    test3();
    //test2();
    //test1();
    //int player = from_user%10; //player is coded in last decimal digit
    //int move = from_user / 10;

    //Game_state g(starting_position, player);
    //diagram(starting_position);
    /*
    Game_state g(starting_position, 1);
    //std::cout << "player to move " << g.player_to_move << endl;
    //diagram(g.board);
   
    
    int res = perform_rollout(g);
    */



    //std::cout << " black: " << (float)black_wins / N  << " white: " << (float)white_wins / N << " time_out " << (float)timed_out / N<<endl;
    //t = clock() - t;
    //std::cout <<"total time " << ((float)t)/CLOCKS_PER_SEC << endl;
    //std::cout << N/(((float)t) / CLOCKS_PER_SEC) << " /second" << endl;
    //tests();
    //tests2();
    //test();
    //test();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
