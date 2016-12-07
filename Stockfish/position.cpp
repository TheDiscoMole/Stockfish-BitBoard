#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

#include "position.h"

const std::string StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

namespace {

  const std::string PieceToChar(" PNBRQK  pnbrqk");
  
  // min_attacker() is a helper function used by see_ge() to locate the least
  // valuable attacker for the side to move, remove the attacker we just found
  // from the bitboards and scan for new X-ray attacks behind it.
  
  template<int Pt>
  PieceType min_attacker(const Bitboard* bb, Square to, Bitboard stmAttackers,
                         Bitboard& occupied, Bitboard& attackers)
  {
  
    Bitboard b = stmAttackers & bb[Pt];
    if (!b)
        return min_attacker<Pt+1>(bb, to, stmAttackers, occupied, attackers);
  
    occupied ^= b & ~(b - 1);
  
    if (Pt == PAWN || Pt == BISHOP || Pt == QUEEN)
        attackers |= attacks_bb<BISHOP>(to, occupied) & (bb[BISHOP] | bb[QUEEN]);
  
    if (Pt == ROOK || Pt == QUEEN)
        attackers |= attacks_bb<ROOK>(to, occupied) & (bb[ROOK] | bb[QUEEN]);
  
    attackers &= occupied; // After X-ray that may add already processed pieces
    return (PieceType)Pt;
  }
  
  template<>
  PieceType min_attacker<KING>(const Bitboard*, Square, Bitboard, Bitboard&, Bitboard&)
  {
    return KING; // No need to update bitboards: it is the last cycle
  }

} // namespace


////////////////////////////
/* Postion Initialisation */
////////////////////////////
Position::Position(const Position& pos, Move m)
{
  *this = pos;
  move(m);
}

Position::Position(const std::string& fen)
{
  set(fen);
  assert(pos_is_ok());
}

Position::Position() : Position(StartFEN) {}


///////////////////////////////////
/* Position Operator Overloading */
///////////////////////////////////
Position& Position::operator=  (const Position& pos)
{
  *this = pos;
}

bool Position::operator== (const Position& pos)
{
  for (int i=0; i<SQUARE_NB; i++)
      if (board[i] == pos.board[i]) return false;
  return true;
}


////////////////////
/* FEN String I/O */
////////////////////
const std::string Position::fen() const
{
    int emptyCnt;
    std::ostringstream ss;
    
    for (Rank r = RANK_8; r >= RANK_1; --r)
    {
        for (File f = FILE_A; f <= FILE_H; ++f)
        {
            for (emptyCnt = 0; f <= FILE_H && empty(make_square(f, r)); ++f)
                ++emptyCnt;
            
            if (emptyCnt)
                ss << emptyCnt;
            
            if (f <= FILE_H)
                ss << PieceToChar[piece_on(make_square(f, r))];
        }
        
        if (r > RANK_1)
            ss << '/';
    }
    
    ss << (sideToMove == WHITE ? " w " : " b ");

    if (can_castle(WHITE_OO))
        ss << 'K';
    
    if (can_castle(WHITE_OOO))
        ss << 'Q';
    
    if (can_castle(BLACK_OO))
        ss << 'k';
    
    if (can_castle(BLACK_OOO))
        ss << 'q';

    if (!can_castle(WHITE) && !can_castle(BLACK))
      ss << '-';
    
    ss << (epSquare == SQ_NONE ? " - " : " " + to_string(epSquare) + " ")
       << rule50 << " " << 1 + (turn - (sideToMove == BLACK)) / 2;

    return ss.str();
}

void Position::fen(const std::string& s)
{
    unsigned char col, row, token;
    size_t idx;
    Square sq = SQ_A8;
    std::istringstream ss(s);
    
    clear();
    ss >> std::noskipws;
    
    // PIECE PLACEMENT
    while ((ss >> token) && !isspace(token))
    {
        if (isdigit(token))
            sq += Square(token - '0');
        
        else if (token == '/')
            sq -= Square(16);
        
        else if ((idx = PieceToChar.find(token)) != std::string::npos)
        {
            put_piece(Piece(idx), sq);
            ++sq;
        }
    }

    // ACTIVE COLOR
    ss >> token;
    sideToMove = (token == 'w' ? WHITE : BLACK);
    ss >> token;
    
    // CASTLING AVAILABILITY
    while ((ss >> token) && !isspace(token))
    {
        Square rsq;
        Color c = islower(token) ? BLACK : WHITE;
        
        token = char(toupper(token));
        
        if (token == 'K')
            for (rsq = relative_square(c, SQ_H1); type_of(piece_on(rsq)) != ROOK; --rsq) {}
        
        else if (token == 'Q')
            for (rsq = relative_square(c, SQ_A1); type_of(piece_on(rsq)) != ROOK; ++rsq) {}
        
        else if (token >= 'A' && token <= 'H')
            rsq = make_square(File(token - 'A'), relative_rank(c, RANK_1));
        
        else
            continue;
        
        set_castling_right(c, rsq);
    }
    
    // EN PASSANTE SQUARE
    if (   ((ss >> col) && (col >= 'a' && col <= 'h'))
        && ((ss >> row) && (row == '3' || row == '6')))
    {
        epSquare = make_square(File(col - 'a'), Rank(row - '1'));
        
        if (   !(attackers_to(epSquare) & pieces(sideToMove, PAWN))
            || !(pieces(~sideToMove, PAWN) & (epSquare + pawn_push(~sideToMove))))
            epSquare = SQ_NONE;
    }
    else
        epSquare = SQ_NONE;
    
    // TURN NUMBER
    ss >> std::skipws >> rule50 >> turn;
}


//////////////
/* Castling */
//////////////
void Position::do_castling(Square from, Square& to, Square& rfrom, Square& rto)
{
  bool kingSide = to > from;
  rfrom = to;
  
  rto = relative_square(sideToMove, kingSide ? SQ_F1 : SQ_D1);
  to  = relative_square(sideToMove, kingSide ? SQ_G1 : SQ_C1);
  
  remove_piece(make_piece(sideToMove, KING), from);
  remove_piece(make_piece(sideToMove, ROOK), rfrom);
  
  board[from] = board[rfrom] = NO_PIECE;
  
  put_piece(make_piece(sideToMove, KING), to);
  put_piece(make_piece(sideToMove, ROOK), rto);
}

/// Position::set_castling_right() is a helper function used to set castling
/// rights given the corresponding color and the rook starting square.
void Position::set_castling_right(Color c, Square rfrom)
{
  Square kfrom = square<KING>(c);
  CastlingSide cs = kfrom < rfrom ? KING_SIDE : QUEEN_SIDE;
  CastlingRight cr = (c | cs);

  castlingRights |= cr;
  castlingRightsMask[kfrom] |= cr;
  castlingRightsMask[rfrom] |= cr;
  castlingRookSquare[cr] = rfrom;

  Square kto = relative_square(c, cs == KING_SIDE ? SQ_G1 : SQ_C1);
  Square rto = relative_square(c, cs == KING_SIDE ? SQ_F1 : SQ_D1);

  for (Square s = std::min(rfrom, rto); s <= std::max(rfrom, rto); ++s)
      if (s != kfrom && s != rfrom)
          castlingPath[cr] |= s;

  for (Square s = std::min(kfrom, kto); s <= std::max(kfrom, kto); ++s)
      if (s != kfrom && s != rfrom)
          castlingPath[cr] |= s;
}


//////////////
/* Checking */
//////////////
Bitboard Position::slider_blockers(Bitboard sliders, Square s, Bitboard& pinners) const
{
  Bitboard result = 0;
  pinners = 0;

  // Snipers are sliders that attack 's' when a piece is removed
  Bitboard snipers = (  (PseudoAttacks[ROOK  ][s] & pieces(QUEEN, ROOK))
                      | (PseudoAttacks[BISHOP][s] & pieces(QUEEN, BISHOP))) & sliders;

  while (snipers)
  {
    Square sniperSq = pop_lsb(&snipers);
    Bitboard b = between_bb(s, sniperSq) & pieces();

    if (!more_than_one(b))
    {
        result |= b;
        if (b & pieces(color_of(piece_on(s))))
            pinners |= sniperSq;
    }
  }
  return result;
}

bool Position::gives_check(Move m) const
{
  assert(color_of(moved_piece(m)) == sideToMove);

  Square from = from_sq(m);
  Square to = to_sq(m);

  // Is there a direct check?
  if (checkSquares[type_of(piece_on(from))] & to)
      return true;

  // Is there a discovered check?
  if (   (discovered_check_candidates() & from)
      && !aligned(from, to, square<KING>(~sideToMove)))
      return true;

  switch (type_of(m))
  {
  case NORMAL:
      return false;

  case PROMOTION:
      return attacks_bb(Piece(promotion_type(m)), to, pieces() ^ from) & square<KING>(~sideToMove);

  // En passant capture with check? We have already handled the case
  // of direct checks and ordinary discovered check, so the only case we
  // need to handle is the unusual case of a discovered check through
  // the captured pawn.
  case ENPASSANT:
  {
      Square capsq = make_square(file_of(to), rank_of(from));
      Bitboard b = (pieces() ^ from ^ capsq) | to;

      return  (attacks_bb<  ROOK>(square<KING>(~sideToMove), b) & pieces(sideToMove, QUEEN, ROOK))
            | (attacks_bb<BISHOP>(square<KING>(~sideToMove), b) & pieces(sideToMove, QUEEN, BISHOP));
  }
  case CASTLING:
  {
      Square kfrom = from;
      Square rfrom = to; // Castling is encoded as 'King captures the rook'
      Square kto = relative_square(sideToMove, rfrom > kfrom ? SQ_G1 : SQ_C1);
      Square rto = relative_square(sideToMove, rfrom > kfrom ? SQ_F1 : SQ_D1);

      return   (PseudoAttacks[ROOK][rto] & square<KING>(~sideToMove))
            && (attacks_bb<ROOK>(rto, (pieces() ^ kfrom ^ rfrom) | rto | kto) & square<KING>(~sideToMove));
  }
  default:
      assert(false);
      return false;
  }
}


///////////////
/* Attacking */
///////////////
Bitboard Position::attackers_to(Square s, Bitboard occ) const
{
  return  (attacks_from<PAWN>(s, BLACK) & pieces(WHITE, PAWN))
        | (attacks_from<PAWN>(s, WHITE) & pieces(BLACK, PAWN))
        | (attacks_from<KNIGHT>(s)      & pieces(KNIGHT))
        | (attacks_bb<ROOK>(s, occ)     & pieces(ROOK, QUEEN))
        | (attacks_bb<BISHOP>(s, occ)   & pieces(BISHOP, QUEEN))
        | (attacks_from<KING>(s)        & pieces(KING));
}


/////////////////////
/* Move Evaluation */
/////////////////////
bool Position::legal(Move m) const
{
  assert(is_ok(m));

  Color us = sideToMove;
  Square from = from_sq(m);

  assert(color_of(moved_piece(m)) == us);
  assert(piece_on(square<KING>(us)) == make_piece(us, KING));

  // En passant captures are a tricky special case. Because they are rather
  // uncommon, we do it simply by testing whether the king is attacked after
  // the move is made.
  if (type_of(m) == ENPASSANT)
  {
      Square ksq = square<KING>(us);
      Square to = to_sq(m);
      Square capsq = to - pawn_push(us);
      Bitboard occupied = (pieces() ^ from ^ capsq) | to;

      assert(to == epSquare);
      assert(moved_piece(m) == make_piece(us, PAWN));
      assert(piece_on(capsq) == make_piece(~us, PAWN));
      assert(piece_on(to) == NO_PIECE);

      return   !(attacks_bb<  ROOK>(ksq, occupied) & pieces(~us, QUEEN, ROOK))
            && !(attacks_bb<BISHOP>(ksq, occupied) & pieces(~us, QUEEN, BISHOP));
  }

  // If the moving piece is a king, check whether the destination
  // square is attacked by the opponent. Castling moves are checked
  // for legality during move generation.
  if (type_of(piece_on(from)) == KING)
      return type_of(m) == CASTLING || !(attackers_to(to_sq(m)) & pieces(~us));

  // A non-king move is legal if and only if it is not pinned or it
  // is moving along the ray towards or away from the king.
  return   !(pinned_pieces(us) & from)
        ||  aligned(from, to_sq(m), square<KING>(us));
}


////////////////////
/* Move Execution */
////////////////////
void Position::move(Move m)
{
  // Increment ply counters. In particular, rule50 will be reset to zero later on
  // in case of a capture or a pawn move.
  turn++;
  rule50++;
  pliesFromNull++;
  
  Color us   = sideToMove;
  Color them = ~us;
  
  Square from = from_sq(m);
  Square to   = to_sq(m);
  
  Piece pc = piece_on(from);
  Piece captured = type_of(m) == ENPASSANT ? make_piece(them, PAWN) : piece_on(to);
  
  // CASTLING
  if (type_of(m) == CASTLING)
  {
      Square rfrom, rto;
      do_castling(from, to, rfrom, rto);
      captured = NO_PIECE;
  }
  
  // CAPTURES
  if (captured)
  {
      Square cap = to;
      
      if (type_of(m) == ENPASSANT)
      {
          cap -= pawn_push(us);
          board[cap] = NO_PIECE; // Not done by remove_piece()
      }
      
      remove_piece(captured, cap);
      rule50 = 0;
  }
  
  // RESET EN PASSANT
  if (epSquare != SQ_NONE)
      epSquare  = SQ_NONE;
  
  // CASTLING RIGHTS
  if (castlingRights &&  (castlingRightsMask[from] | castlingRightsMask[to]))
      castlingRights &= ~(castlingRightsMask[from] | castlingRightsMask[to]);
  // MOVE
  if (type_of(m) != CASTLING)
      move_piece(pc, from, to);
  
  // PAWN MAGIC
  if (type_of(pc) == PAWN)
  {
      // SET EN PASSANTE
      if (  ((int(to) ^ int(from)) == 16)
          && (attacks_from<PAWN>(to - pawn_push(us), us) & pieces(them, PAWN)))
          epSquare = (from + to) / 2;
      
      // DO PROMOTION
      if (type_of(m) == PROMOTION)
      {
          Piece promotion = make_piece(us, promotion_type(m));
          
          remove_piece(pc, to);
          put_piece(promotion, to);
      }
      
      rule50 = 0;
  }
  
  checkersBB = gives_check(m) ? attackers_to(square<KING>(them)) & pieces(us) : 0;
  
  // UPDATE TURN COLOR
  sideToMove = ~sideToMove;
}


//////////////////////
/* Draw Information */
//////////////////////
bool Position::is_draw() const {return rule50 > 50; }


///////////
/* Other */
///////////
void Position::clear()
{
    std::memset(this, 0, sizeof(Position));
    epSquare = SQ_NONE;
    
    for (int i = 0; i < PIECE_NB; ++i)
        for (int j = 0; j < 16; ++j)
            pieceList[i][j] = SQ_NONE;
}

bool Position::pos_is_ok(int* failedStep) const
{
  const bool Fast = true; // Quick (default) or full check?

  enum { Default, King, Bitboards, State, Lists, Castling };

  for (int step = Default; step <= (Fast ? Default : Castling); step++)
  {
      if (failedStep)
          *failedStep = step;

      if (step == Default)
          if (   (sideToMove != WHITE && sideToMove != BLACK)
              || piece_on(square<KING>(WHITE)) != W_KING
              || piece_on(square<KING>(BLACK)) != B_KING
              || (   ep_square() != SQ_NONE
                  && relative_rank(sideToMove, ep_square()) != RANK_6))
              return false;

      if (step == King)
          if (   std::count(board, board + SQUARE_NB, W_KING) != 1
              || std::count(board, board + SQUARE_NB, B_KING) != 1
              || attackers_to(square<KING>(~sideToMove)) & pieces(sideToMove))
              return false;

      if (step == Bitboards)
      {
          if (  (pieces(WHITE) & pieces(BLACK))
              ||(pieces(WHITE) | pieces(BLACK)) != pieces())
              return false;

          for (PieceType p1 = PAWN; p1 <= KING; ++p1)
              for (PieceType p2 = PAWN; p2 <= KING; ++p2)
                  if (p1 != p2 && (pieces(p1) & pieces(p2)))
                      return false;
      }

      if (step == State)
      {
          StateInfo si = *st;
          set_state(&si);
          if (std::memcmp(&si, st, sizeof(StateInfo)))
              return false;
      }

      if (step == Lists)
          for (Piece pc : Pieces)
          {
              if (pieceCount[pc] != popcount(pieces(color_of(pc), type_of(pc))))
                  return false;

              for (int i = 0; i < pieceCount[pc]; ++i)
                  if (board[pieceList[pc][i]] != pc || index[pieceList[pc][i]] != i)
                      return false;
          }

      if (step == Castling)
          for (Color c = WHITE; c <= BLACK; ++c)
              for (CastlingSide s = KING_SIDE; s <= QUEEN_SIDE; s = CastlingSide(s + 1))
              {
                  if (!can_castle(c | s))
                      continue;

                  if (   piece_on(castlingRookSquare[c | s]) != make_piece(c, ROOK)
                      || castlingRightsMask[castlingRookSquare[c | s]] != (c | s)
                      ||(castlingRightsMask[square<KING>(c)] & (c | s)) != (c | s))
                      return false;
              }
  }

  return true;
}
