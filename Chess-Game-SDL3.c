#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 640
#define TILE_SIZE     80
#define NUM_PIECES    12


typedef enum {
    EMPTY = -1,
    WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
    BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING
} PieceType;

PieceType board[8][8] = {
    {BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK},
    {BLACK_PAWN, BLACK_PAWN,   BLACK_PAWN,   BLACK_PAWN,  BLACK_PAWN, BLACK_PAWN,   BLACK_PAWN,   BLACK_PAWN},
    {EMPTY,      EMPTY,        EMPTY,        EMPTY,       EMPTY,      EMPTY,        EMPTY,        EMPTY},
    {EMPTY,      EMPTY,        EMPTY,        EMPTY,       EMPTY,      EMPTY,        EMPTY,        EMPTY},
    {EMPTY,      EMPTY,        EMPTY,        EMPTY,       EMPTY,      EMPTY,        EMPTY,        EMPTY},
    {EMPTY,      EMPTY,        EMPTY,        EMPTY,       EMPTY,      EMPTY,        EMPTY,        EMPTY},
    {WHITE_PAWN, WHITE_PAWN,   WHITE_PAWN,   WHITE_PAWN,  WHITE_PAWN, WHITE_PAWN,   WHITE_PAWN,   WHITE_PAWN},
    {WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK}
};

SDL_Texture **loadPieceTextures(SDL_Renderer *renderer) {
    static const char *paths[NUM_PIECES] = {
        "pieces/wp.png", "pieces/wn.png", "pieces/wb.png",
        "pieces/wr.png", "pieces/wq.png", "pieces/wk.png",
        "pieces/bp.png", "pieces/bn.png", "pieces/bb.png",
        "pieces/br.png", "pieces/bq.png", "pieces/bk.png"
    };

    SDL_Texture **textures = malloc(NUM_PIECES * sizeof(*textures));
    if (!textures) {
        printf("Failed to allocate memory for textures.\n");
        return NULL;
    }

    for (int i = 0; i < NUM_PIECES; i++) {
        textures[i] = IMG_LoadTexture(renderer, paths[i]);
        if (!textures[i]) {
            printf("Failed to load %s: %s\n", paths[i], SDL_GetError());
        }
    }

    return textures;
}

bool isWhitePiece(PieceType piece) {
    return piece >= WHITE_PAWN && piece <= WHITE_KING;
}
bool isBlackPiece(PieceType piece) {
    return piece >= BLACK_PAWN && piece <= BLACK_KING;
}
bool isSameColor(PieceType a, PieceType b) {
    if (a == EMPTY || b == EMPTY) return false;
    return (isWhitePiece(a) && isWhitePiece(b)) || (isBlackPiece(a) && isBlackPiece(b));
}

// --- CHECK LOGIC HELPERS ---
bool isSquareAttacked(int kr, int kc, bool byWhite) {
    
    if (byWhite) {
        int r = kr - 1;
        if (r >= 0) {
            if (kc - 1 >= 0 && board[r][kc-1] == WHITE_PAWN) return true;
            if (kc + 1 < 8 && board[r][kc+1] == WHITE_PAWN) return true;
        }
    } else {
        int r = kr + 1;
        if (r < 8) {
            if (kc - 1 >= 0 && board[r][kc-1] == BLACK_PAWN) return true;
            if (kc + 1 < 8 && board[r][kc+1] == BLACK_PAWN) return true;
        }
    }

    // Knights
    int nkx[8] = {-2,-1,1,2,2,1,-1,-2};
    int nky[8] = {1,2,2,1,-1,-2,-2,-1};
    for (int i = 0; i < 8; i++) {
        int nr = kr + nkx[i], nc = kc + nky[i];
        if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
            if (byWhite && board[nr][nc] == WHITE_KNIGHT) return true;
            if (!byWhite && board[nr][nc] == BLACK_KNIGHT) return true;
        }
    }

    // Straight lines for rook/queen
    int rdirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (int d = 0; d < 4; d++) {
        int r = kr + rdirs[d][0], c = kc + rdirs[d][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (board[r][c] != EMPTY) {
                if (byWhite && (board[r][c] == WHITE_ROOK || board[r][c] == WHITE_QUEEN)) return true;
                if (!byWhite && (board[r][c] == BLACK_ROOK || board[r][c] == BLACK_QUEEN)) return true;
                break;
            }
            r += rdirs[d][0]; c += rdirs[d][1];
        }
    }

    // Diagonals for bishop/queen
    int bdirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (int d = 0; d < 4; d++) {
        int r = kr + bdirs[d][0], c = kc + bdirs[d][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            if (board[r][c] != EMPTY) {
                if (byWhite && (board[r][c] == WHITE_BISHOP || board[r][c] == WHITE_QUEEN)) return true;
                if (!byWhite && (board[r][c] == BLACK_BISHOP || board[r][c] == BLACK_QUEEN)) return true;
                break;
            }
            r += bdirs[d][0]; c += bdirs[d][1];
        }
    }

    // King adjacency
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int r = kr + dr, c = kc + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8) {
                if (byWhite && board[r][c] == WHITE_KING) return true;
                if (!byWhite && board[r][c] == BLACK_KING) return true;
            }
        }
    }

    return false;
}

bool isKingInCheck(bool white, int *kr_out, int *kc_out) {
    PieceType king = white ? WHITE_KING : BLACK_KING;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (board[r][c] == king) {
                if (kr_out) *kr_out = r;
                if (kc_out) *kc_out = c;
                return isSquareAttacked(r, c, !white);
            }
        }
    }
    return false;
}

bool pseudoLegalMove(int sr, int sc, int tr, int tc) {
    if (sr == tr && sc == tc) return false;
    PieceType p = board[sr][sc];
    if (p == EMPTY) return false;

    // target same color
    if (board[tr][tc] != EMPTY && isSameColor(p, board[tr][tc])) return false;

    // Pawn
    if (p == WHITE_PAWN || p == BLACK_PAWN) {
        int dir = (p == WHITE_PAWN) ? -1 : 1;
        int startRow = (p == WHITE_PAWN) ? 6 : 1;
        // forward
        if (tc == sc) {
            if (tr == sr + dir && board[tr][tc] == EMPTY) return true;
            if (sr == startRow && tr == sr + 2*dir && board[sr + dir][sc] == EMPTY && board[tr][tc] == EMPTY) return true;
            return false;
        }
        // capture diagonal
        if (abs(tc - sc) == 1 && tr == sr + dir && board[tr][tc] != EMPTY && !isSameColor(p, board[tr][tc])) return true;
        return false;
    }

    // Knight
    if (p == WHITE_KNIGHT || p == BLACK_KNIGHT) {
        int dr = abs(tr - sr), dc = abs(tc - sc);
        return (dr == 2 && dc == 1) || (dr == 1 && dc == 2);
    }

    // Bishop
    if (p == WHITE_BISHOP || p == BLACK_BISHOP) {
        if (abs(tr - sr) != abs(tc - sc)) return false;
        int rstep = (tr > sr) ? 1 : -1;
        int cstep = (tc > sc) ? 1 : -1;
        int r = sr + rstep, c = sc + cstep;
        while (r != tr && c != tc) {
            if (board[r][c] != EMPTY) return false;
            r += rstep; c += cstep;
        }
        return true;
    }

    // Rook
    if (p == WHITE_ROOK || p == BLACK_ROOK) {
        if (tr != sr && tc != sc) return false;
        int rstep = (tr == sr) ? 0 : (tr > sr ? 1 : -1);
        int cstep = (tc == sc) ? 0 : (tc > sc ? 1 : -1);
        int r = sr + rstep, c = sc + cstep;
        while (r != tr || c != tc) {
            if (board[r][c] != EMPTY) return false;
            r += rstep; c += cstep;
        }
        return true;
    }

    // Queen
    if (p == WHITE_QUEEN || p == BLACK_QUEEN) {
        if (tr == sr || tc == sc) {
            int rstep = (tr == sr) ? 0 : (tr > sr ? 1 : -1);
            int cstep = (tc == sc) ? 0 : (tc > sc ? 1 : -1);
            int r = sr + rstep, c = sc + cstep;
            while (r != tr || c != tc) {
                if (board[r][c] != EMPTY) return false;
                r += rstep; c += cstep;
            }
            return true;
        }
        if (abs(tr - sr) == abs(tc - sc)) {
            int rstep = (tr > sr) ? 1 : -1;
            int cstep = (tc > sc) ? 1 : -1;
            int r = sr + rstep, c = sc + cstep;
            while (r != tr && c != tc) {
                if (board[r][c] != EMPTY) return false;
                r += rstep; c += cstep;
            }
            return true;
        }
        return false;
    }

    // King
    if (p == WHITE_KING || p == BLACK_KING) {
        int dr = abs(tr - sr), dc = abs(tc - sc);
        return dr <= 1 && dc <= 1;
    }

    return false;
}

bool sideHasLegalMove(bool white) {
    for (int sr = 0; sr < 8; sr++) {
        for (int sc = 0; sc < 8; sc++) {
            PieceType p = board[sr][sc];
            if (p == EMPTY) continue;
            if (white && !isWhitePiece(p)) continue;
            if (!white && !isBlackPiece(p)) continue;

            // Try all target squares
            for (int tr = 0; tr < 8; tr++) {
                for (int tc = 0; tc < 8; tc++) {
                    if (!pseudoLegalMove(sr, sc, tr, tc)) continue;

                    // make move
                    PieceType savedFrom = board[sr][sc];
                    PieceType savedTo = board[tr][tc];
                    board[tr][tc] = savedFrom;
                    board[sr][sc] = EMPTY;

                    // find king pos and test check
                    bool kingInCheck = isKingInCheck(white, NULL, NULL);

                    // undo
                    board[sr][sc] = savedFrom;
                    board[tr][tc] = savedTo;

                    if (!kingInCheck) return true;
                }
            }
        }
    }
    return false;
}

void drawChessboard(SDL_Renderer* renderer, SDL_Texture** pieceTextures) {
    
   
	
	int kr, kc;
    bool whiteCheck = isKingInCheck(true, &kr, &kc);
    int wkr = kr, wkc = kc;
    bool blackCheck = isKingInCheck(false, &kr, &kc);
    int bkr = kr, bkc = kc;
    
  
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            SDL_FRect tile = { col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE };
            if ((row + col) % 2 == 0)
                SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255);
                
            else
                SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);
                

            // highlight checked king square in red
            if ((whiteCheck && row == wkr && col == wkc) || (blackCheck && row == bkr && col == bkc)) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            }

            SDL_RenderFillRect(renderer, &tile);

            PieceType piece = board[row][col];
            if (piece != EMPTY && pieceTextures[piece])
                SDL_RenderTexture(renderer, pieceTextures[piece], NULL, &tile);
        }
    }

    SDL_RenderPresent(renderer);
}

bool getTileFromMouse(int x, int y, int *row, int *col) {
    if (x < 0 || y < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)
        return false;
    *col = x / TILE_SIZE;
    *row = y / TILE_SIZE;
    return true;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Chess Game", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Window Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        printf("Renderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture **pieces = loadPieceTextures(renderer);
    if (!pieces) {
        printf("Error loading piece textures!\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int running = 1;
    SDL_Event e;
    int selectedRow = -1, selectedCol = -1;
    bool whiteTurn = true;
    bool gameOver = false;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = 0;
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !gameOver) {
                int row, col;
                if (getTileFromMouse(e.button.x, e.button.y, &row, &col)) {
                    PieceType piece = board[row][col];

                    if (selectedRow == -1) {
                        if ((whiteTurn && isWhitePiece(piece)) || (!whiteTurn && isBlackPiece(piece))) {
                            selectedRow = row;
                            selectedCol = col;
                        }
                    } else {
                        
                        PieceType selPiece = board[selectedRow][selectedCol];
                        PieceType target = board[row][col];
                        bool didMove = false;

                        // Pawn move rules 
                        if (selPiece == WHITE_PAWN || selPiece == BLACK_PAWN) {
                            int dir = (selPiece == WHITE_PAWN) ? -1 : 1;
                            int startRow = (selPiece == WHITE_PAWN) ? 6 : 1;
                            // forward single
                            if (col == selectedCol && row == selectedRow + dir && board[row][col] == EMPTY) {
                                // move
                                board[row][col] = selPiece;
                                board[selectedRow][selectedCol] = EMPTY;
                                didMove = true;
                            }
                            // forward double from start
                            else if (col == selectedCol && selectedRow == startRow && row == selectedRow + 2*dir &&
                                     board[selectedRow + dir][col] == EMPTY && board[row][col] == EMPTY) {
                                board[row][col] = selPiece;
                                board[selectedRow][selectedCol] = EMPTY;
                                didMove = true;
                            }
                            // capture diagonally
                            else if (abs(col - selectedCol) == 1 && row == selectedRow + dir &&
                                     board[row][col] != EMPTY && !isSameColor(selPiece, board[row][col])) {
                                board[row][col] = selPiece;
                                board[selectedRow][selectedCol] = EMPTY;
                                didMove = true;
                            }
                        }
                        // Knight
                        else if (selPiece == WHITE_KNIGHT || selPiece == BLACK_KNIGHT) {
                            int dr = abs(row - selectedRow), dc = abs(col - selectedCol);
                            if ((dr == 2 && dc == 1) || (dr == 1 && dc == 2)) {
                                if (!isSameColor(selPiece, board[row][col])) {
                                    board[row][col] = selPiece;
                                    board[selectedRow][selectedCol] = EMPTY;
                                    didMove = true;
                                }
                            }
                        }
                        // Bishop
                        else if (selPiece == WHITE_BISHOP || selPiece == BLACK_BISHOP) {
                            if (abs(row - selectedRow) == abs(col - selectedCol)) {
                                int rStep = (row > selectedRow) ? 1 : -1;
                                int cStep = (col > selectedCol) ? 1 : -1;
                                int r = selectedRow + rStep, c = selectedCol + cStep;
                                bool blocked = false;
                                while (r != row && c != col) {
                                    if (board[r][c] != EMPTY) { blocked = true; break; }
                                    r += rStep; c += cStep;
                                }
                                if (!blocked && !isSameColor(selPiece, board[row][col])) {
                                    board[row][col] = selPiece;
                                    board[selectedRow][selectedCol] = EMPTY;
                                    didMove = true;
                                }
                            }
                        }
                        // Rook
                        else if (selPiece == WHITE_ROOK || selPiece == BLACK_ROOK) {
                            if (row == selectedRow || col == selectedCol) {
                                int rStep = (row > selectedRow) ? 1 : (row < selectedRow ? -1 : 0);
                                int cStep = (col > selectedCol) ? 1 : (col < selectedCol ? -1 : 0);
                                int r = selectedRow + rStep, c = selectedCol + cStep;
                                bool blocked = false;
                                while (r != row || c != col) {
                                    if (board[r][c] != EMPTY) { blocked = true; break; }
                                    r += rStep; c += cStep;
                                }
                                if (!blocked && !isSameColor(selPiece, board[row][col])) {
                                    board[row][col] = selPiece;
                                    board[selectedRow][selectedCol] = EMPTY;
                                    didMove = true;
                                }
                            }
                        }
                        // Queen
                        else if (selPiece == WHITE_QUEEN || selPiece == BLACK_QUEEN) {
                            if (row == selectedRow || col == selectedCol ||
                                abs(row - selectedRow) == abs(col - selectedCol)) {
                                int rStep = (row > selectedRow) ? 1 : (row < selectedRow ? -1 : 0);
                                int cStep = (col > selectedCol) ? 1 : (col < selectedCol ? -1 : 0);
                                int r = selectedRow + rStep, c = selectedCol + cStep;
                                bool blocked = false;
                                while (r != row || c != col) {
                                    if (board[r][c] != EMPTY) { blocked = true; break; }
                                    r += rStep; c += cStep;
                                }
                                if (!blocked && !isSameColor(selPiece, board[row][col])) {
                                    board[row][col] = selPiece;
                                    board[selectedRow][selectedCol] = EMPTY;
                                    didMove = true;
                                }
                            }
                        }
                        // King
                        else if (selPiece == WHITE_KING || selPiece == BLACK_KING) {
                            int dr = abs(row - selectedRow);
                            int dc = abs(col - selectedCol);
                            if (dr <= 1 && dc <= 1 && !isSameColor(selPiece, board[row][col])) {
                                board[row][col] = selPiece;
                                board[selectedRow][selectedCol] = EMPTY;
                                didMove = true;
                            }
                        }

                        // If a move was attempted, ensure it doesn't leave own king in check.
                        if (didMove) {
                            bool ownWhite = isWhitePiece(selPiece);
                            // If after move own king is in check, undo and cancel move.
                            if (isKingInCheck(ownWhite, NULL, NULL)) {
                                // undo
                                board[selectedRow][selectedCol] = selPiece;
                                board[row][col] = target;
                                didMove = false;
                            } else {
                                // move successful, flip turn
                                whiteTurn = !whiteTurn;
                            }
                        }

                        selectedRow = selectedCol = -1;

                        // After successful move, check for checkmate on the side to move
                        if (!gameOver && didMove) {
                            bool oppWhite = whiteTurn; // side to move after flip
                            bool inCheck = isKingInCheck(oppWhite, NULL, NULL);
                            if (inCheck) {
                                // if side to move has no legal move, checkmate
                                if (!sideHasLegalMove(oppWhite)) {
                                    // show message box and exit
                                    const char *title = "Checkmate";
                                    const char *message = oppWhite ? "Black Wins!" : "White Wins!";
                                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title, message, NULL);
                                    gameOver = true;
                                    // exit loop after message: break outer loops by setting running=0
                                    running = 0;
                                }
                            }
                        }
                    }
                }
            }
        }

        drawChessboard(renderer, pieces);
        SDL_Delay(16);
    }

    for (int i = 0; i < NUM_PIECES; i++)
        if (pieces[i]) SDL_DestroyTexture(pieces[i]);
    free(pieces);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}