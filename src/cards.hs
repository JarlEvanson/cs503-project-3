module Cards where

data Suit = Hearts | Diamonds | Clubs | Spades deriving (Eq, Enum, Bounded)
data Rank = Ace | Two | Three | Four | Five
    | Six | Seven | Eight | Nine | Ten | Jack | Queen | King deriving (Eq, Enum, Bounded)

data Card = Card { rank :: Rank, suit :: Suit } deriving (Eq)

instance Show Suit where
  show Hearts   = "♥"
  show Diamonds = "♦"
  show Clubs    = "♣"
  show Spades   = "♠"

instance Show Rank where
  show Ace   = "A"
  show Two   = "2"
  show Three = "3"
  show Four  = "4"
  show Five  = "5"
  show Six   = "6"
  show Seven = "7"
  show Eight = "8"
  show Nine  = "9"
  show Ten   = "10"
  show Jack  = "J"
  show Queen = "Q"
  show King  = "K"

instance Show Card where show (Card r s) = show r ++ show s

rank_value :: Rank -> Int
rank_value r
    | r == Ace = 1
    | r == Two = 2
    | r == Three = 3
    | r == Four = 4
    | r == Five = 5
    | r == Six = 6
    | r == Seven = 7
    | r == Eight = 8
    | r == Nine = 9
    | r == Ten = 10
    | r == Jack = 10
    | r == Queen = 10
    | r == King = 10

card_value :: Card -> Int
card_value (Card r _) = rank_value r

has_ace :: [Card] -> Bool
has_ace hand = (length $ filter (\(Card r _) -> r == Ace) hand) > 0

best_hand_value :: [Card] -> Int
best_hand_value hand = do
    let base = sum $ map card_value hand
    if base <= 11 && has_ace hand then base + 10 else base

showHand :: [Card] -> String
showHand hand = (foldl (\string card -> string ++ ", " ++ show card) (show $ hand !! 0) (drop 1 hand))
    ++ " ("
    ++ (show $ best_hand_value hand)
    ++ ")"

full_deck :: [Card]
full_deck = [ Card r s | s <- [minBound .. maxBound], r <- [minBound .. maxBound] ]
