#if defined _chess_const_included
  #endinput
#endif
#define _chess_const_included


#define MAX_BOARDS 2

#define BOARD_ROWS 8
#define BOARD_COLUMNS 8

#define SQUARE_COUNT (BOARD_ROWS * BOARD_COLUMNS)
#define PIECE_COUNT 32

enum _:PieceRank {
    None,
    Pawn,
    Bishop,
    Knight,
    Rook,
    Queen,
    King
};

enum _:PieceID {
  InvalidID = -1
};

enum _:PieceStatus {
  Battle,
  Captured
};

enum _:PieceColor {
  White,
  Black
};

enum _:PieceStruct {
    Id,
    Rank,
    Color,
    Row, 
    Column,
    Status
};

enum _:BoardStruct {
  bool:IsInitialized,
};

enum MoveTypeEnum {
  Move,
  PawnLongMove, // when pawn moves 2 squares 
  Capture,
  CaptureOnPassing,
  Castling,
  Promote
};

enum _:MoveStruct {
  MoveTypeEnum:MoveType,
  MovingPieceID,
  OtherPieceID, // for rook castling, capture
  // moving piece start position
  StartRow,
  StartColumn,
  // moving piece end position
  EndRow,
  EndColumn,
  // rook position after castling OR en-passant-captured piece position
  Row2,
  Column2,
  // captured piece rank OR pawn's new rank after promotion
  CapturedRank,
  IsFirstMove,
  MoveStructSize
}
