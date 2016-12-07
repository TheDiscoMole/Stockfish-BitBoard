/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad
  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <cassert>
#include <cstddef>
#include <string>

#include "bitboard.h"
#include "types.h"

/// Position class stores information regarding the board representation as
/// pieces, side to move, hash keys, castling info, etc. Important methods are
/// do_move() and undo_move(), used by the search to update node info when
/// traversing the search tree.

struct Position
{
  
  
  ///////////////////////
  /* Postion Functions */
  ///////////////////////
  
  
  // Position Initialisation
  Position(const Position& pos, Move m);
  Position(const std::string& fen);
  Position();
  
  // Position Operator Overloading
  Position& operator=  (const Position& pos);
      bool  operator== (const Position& pos);
  
  // FEN String I/O
  const std::string fen()                                const;
  void fen(const std::string& s);
  char to_char(File f, bool tolower = true)              const;
  char to_char(Rank r)                                   const;
  const std::string to_string(Square s)                  const;
  
  // Board Representation
  Bitboard pieces()                                      const;
  Bitboard pieces(PieceType pt)                          const;
  Bitboard pieces(PieceType pt1, PieceType pt2)          const;
  Bitboard pieces(Color c)                               const;
  Bitboard pieces(Color c, PieceType pt)                 const;
  Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const;
  
  // Pieces
  Piece piece_on(Square s)                               const;
  Piece moved_piece(Move m)                              const;
  Piece captured_piece()                                 const;
  void put_piece(Piece pc, Square s);
  void remove_piece(Piece pc, Square s);
  void move_piece(Piece pc, Square from, Square to);
  
  // Squares
  bool empty(Square s)                                   const;
  template<PieceType Pt> int count(Color c)              const;
  template<PieceType Pt> const Square* squares(Color c)  const;
  template<PieceType Pt> Square square(Color c)          const;
  
  // Castling
  int can_castle(Color c)                                const;
  int can_castle(CastlingRight cr)                       const;
  bool castling_impeded(CastlingRight cr)                const;
  Square castling_rook_square(CastlingRight cr)          const;
  void do_castling(Square from, Square& to, Square& rfrom, Square& rto);
  void set_castling_right(Color c, Square rfrom);
    
  // Checking
  Bitboard checkers()                                    const;
  Bitboard discovered_check_candidates()                 const;
  Bitboard pinned_pieces(Color c)                        const;
  Bitboard check_squares(PieceType pt)                   const;

  // Attacking
  Bitboard attackers_to(Square s)                        const;
  Bitboard attackers_to(Square s, Bitboard occ)          const;
  Bitboard attacks_from(Piece pc, Square s)              const;
  template<PieceType>
  Bitboard attacks_from(Square s)                        const;
  template<PieceType>
  Bitboard attacks_from(Square s, Color c)               const;
  Bitboard slider_blockers(Bitboard sliders, Square s, Bitboard& pinners) const;
    
  // Move Evalution
  bool legal(Move m)                                     const;
  bool gives_check(Move m)                               const;
    
  // Move Execution
  void move(Move m);
  
  // Draw Information
  bool is_draw()                                         const;

  // Other
  void clear();
  bool pos_is_ok();
  
  
  //////////////////
  /* Postion Data */
  //////////////////
  
  
  // Board & Pieces
  Piece board[SQUARE_NB];
  Bitboard byTypeBB[PIECE_TYPE_NB];
  Bitboard byColorBB[COLOR_NB];
  
  // En Passante Square
  Square epSquare;

  // Piece Info
  int pieceCount[PIECE_NB];
  int index[SQUARE_NB];
  
  Square pieceList[PIECE_NB][16];
  
  // Castling Info
  int      castlingRightsMask[SQUARE_NB];
  Square   castlingRookSquare[CASTLING_RIGHT_NB];
  Bitboard castlingPath[CASTLING_RIGHT_NB];
  
  // Checking Info
  Bitboard checkersBB;
  Bitboard blockersForKing[COLOR_NB];
  Bitboard pinnersForKing[COLOR_NB];
  Bitboard checkSquares[PIECE_TYPE_NB];
    
  // Other Info
  Color sideToMove;
  int castlingRights, rule50, pliesFromNull, turn;
  
};


////////////////////
/* FEN String I/O */
////////////////////
inline char Position::to_char(File f, bool tolower/*= true*/) const
{
    return char(f - FILE_A + (tolower ? 'a' : 'A'));
}

inline char Position::to_char(Rank r) const
{
    return char(r - RANK_1 + '1');
}

inline const std::string Position::to_string(Square s) const {
    char ch[] = { to_char(file_of(s)), to_char(rank_of(s)), 0 };
    return ch;
}


//////////////////////////
/* Board Representation */
//////////////////////////
inline Bitboard Position::pieces() const
{
    return byTypeBB[ALL_PIECES];
}

inline Bitboard Position::pieces(PieceType pt) const
{
    return byTypeBB[pt];
}

inline Bitboard Position::pieces(PieceType pt1, PieceType pt2) const
{
    return byTypeBB[pt1] | byTypeBB[pt2];
}

inline Bitboard Position::pieces(Color c) const
{
    return byColorBB[c];
}

inline Bitboard Position::pieces(Color c, PieceType pt) const
{
    return byColorBB[c] & byTypeBB[pt];
}

inline Bitboard Position::pieces(Color c, PieceType pt1, PieceType pt2) const
{
    return byColorBB[c] & (byTypeBB[pt1] | byTypeBB[pt2]);
}


////////////
/* Pieces */
////////////
inline Piece Position::piece_on(Square s) const
{
    return board[s];
}

inline Piece Position::moved_piece(Move m) const
{
    return board[from_sq(m)];
}

inline void Position::put_piece(Piece pc, Square s)
{
    board[s] = pc;
    byTypeBB[ALL_PIECES] |= s;
    byTypeBB[type_of(pc)] |= s;
    byColorBB[color_of(pc)] |= s;
    index[s] = pieceCount[pc]++;
    pieceList[pc][index[s]] = s;
    pieceCount[make_piece(color_of(pc), ALL_PIECES)]++;
}

inline void Position::remove_piece(Piece pc, Square s)
{
  // WARNING: This is not a reversible operation. If we remove a piece in
  // do_move() and then replace it in undo_move() we will put it at the end of
  // the list and not in its original place, it means index[] and pieceList[]
  // are not invariant to a do_move() + undo_move() sequence.
  byTypeBB[ALL_PIECES] ^= s;
  byTypeBB[type_of(pc)] ^= s;
  byColorBB[color_of(pc)] ^= s;
  Square lastSquare = pieceList[pc][--pieceCount[pc]];
  index[lastSquare] = index[s];
  pieceList[pc][index[lastSquare]] = lastSquare;
  pieceList[pc][pieceCount[pc]] = SQ_NONE;
  pieceCount[make_piece(color_of(pc), ALL_PIECES)]--;
}

inline void Position::move_piece(Piece pc, Square from, Square to)
{
  // index[from] is not updated and becomes stale. This works as long as index[]
  // is accessed just by known occupied squares.
  Bitboard from_to_bb = SquareBB[from] ^ SquareBB[to];
  byTypeBB[ALL_PIECES] ^= from_to_bb;
  byTypeBB[type_of(pc)] ^= from_to_bb;
  byColorBB[color_of(pc)] ^= from_to_bb;
  board[from] = NO_PIECE;
  board[to] = pc;
  index[to] = index[from];
  pieceList[pc][index[to]] = to;
}


/////////////
/* Squares */
/////////////
inline bool Position::empty(Square s) const
{
    return board[s] == NO_PIECE;
}

template<PieceType Pt>
inline int Position::count(Color c) const
{
    return pieceCount[make_piece(c, Pt)];
}

template<PieceType Pt>
inline const Square* Position::squares(Color c) const
{
    return pieceList[make_piece(c, Pt)];
}

template<PieceType Pt>
inline Square Position::square(Color c) const
{
    assert(pieceCount[make_piece(c, Pt)] == 1);
    return pieceList[make_piece(c, Pt)][0];
}


//////////////
/* Castling */
//////////////
inline int Position::can_castle(CastlingRight cr) const
{
    return castlingRights & cr;
}

inline int Position::can_castle(Color c) const
{
    return castlingRights & ((WHITE_OO | WHITE_OOO) << (2 * c));
}

inline bool Position::castling_impeded(CastlingRight cr) const
{
    return byTypeBB[ALL_PIECES] & castlingPath[cr];
}

inline Square Position::castling_rook_square(CastlingRight cr) const
{
    return castlingRookSquare[cr];
}


//////////////
/* Checking */
//////////////
inline Bitboard Position::checkers() const
{
    return checkersBB;
}

inline Bitboard Position::discovered_check_candidates() const
{
    return blockersForKing[~sideToMove] & pieces(sideToMove);
}

inline Bitboard Position::pinned_pieces(Color c) const
{
    return blockersForKing[c] & pieces(c);
}

inline Bitboard Position::check_squares(PieceType pt) const
{
    return checkSquares[pt];
}


///////////////
/* Attacking */
///////////////
inline Bitboard Position::attackers_to(Square s) const
{
    return attackers_to(s, byTypeBB[ALL_PIECES]);
}

template<PieceType Pt>
inline Bitboard Position::attacks_from(Square s) const
{
    return  Pt == BISHOP || Pt == ROOK ? attacks_bb<Pt>(s, byTypeBB[ALL_PIECES])
            : Pt == QUEEN  ? attacks_from<ROOK>(s) | attacks_from<BISHOP>(s)
            : StepAttacksBB[Pt][s];
}

template<>
inline Bitboard Position::attacks_from<PAWN>(Square s, Color c) const
{
    return StepAttacksBB[make_piece(c, PAWN)][s];
}

inline Bitboard Position::attacks_from(Piece pc, Square s) const
{
    return attacks_bb(pc, s, byTypeBB[ALL_PIECES]);
}

#endif // POSITION_H_INCLUDED
