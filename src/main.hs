module Main where

import Gameplay
import Cards

main :: IO ()

-- Used to search for various interesting starting states.
main = print_initial_hands 0

print_initial_hands :: Integer -> IO ()
print_initial_hands seed = do
    putStrLn $ "Seed: " ++ show seed ++ " Deck: " ++ (show $ initial_deal HandState {
        game_state = GameState { rng = seed, balance = 0, deck = full_deck },
        bet = 0,
        player_hand = [],
        dealer_hand = []
    })
    print_initial_hands $ seed + 1


-- Seed 19762: Insurance opportunity - failure
-- Seed 8935347: Player Natural blackjack
-- Seed 8935370: Dealer Natural blackjack
