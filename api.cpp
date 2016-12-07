#include "Stockfish/bitboard.h"
#include "Stockfish/position.h"
#include "Stockfish/movegen.h"
#include "Stockfish/types.h"

#include <string>

using namespace std;

// Chess Tree Node
struct Node
{
    Node *parent, *children;
    
    Position position;
    float evaluation;
    
    Node(Position position, Move m) : position(position, m) {}
    Node() : position(Position()) {}
    
    // Set/Get postion FEN
    void   fen(const string& fen);
    string fen();
    
    // Check if draw, Only when no children generated
    // Repition gets checked here since stateinfo from source was deleted
    // ~children and check = checkmate
    // ~children and ~check = stalemate
    bool isDraw();
    
    // Turn info
    Color whoseTurn();
    int   whatTurn();
    
    // Gets position as convenient NN input format arr[8][8][12]
    float getBits();
    
    // Generate and play all LEGAL moves from this position
    void playMoves();
};


/////////////////////////
/* Set/Get postion FEN */
/////////////////////////
void   Node::fen(const string& fen)
{
    return position.fen(fen);
}

string Node::fen()
{
    return position.fen();
}


////////////////
/* Check DRAW */
////////////////
bool Node::isDraw()
{
    if (position.is_draw()) return true;
    if (position == parent->parent->position) return true;
    
    return false;
}


///////////////
/* Turn Info */
///////////////
Color Node::whoseTurn()
{
    return position.sideToMove;
}

int   Node:: whatTurn()
{
    return position.turn;
}


//////////////
/* NN Input */
//////////////
float Node::getBits()
{
    return 0;
}


//////////////////////////
/* Play all LEGAL moves */
//////////////////////////
void Node::playMoves()
{
    // TODO
}
