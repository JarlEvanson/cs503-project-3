module Gameplay where

import System.IO -- Imported for hFlush stdout

import Cards
import Util

data GameState = GameState {
    rng :: Integer, -- Rng state for the program.
    balance :: Integer, -- The balance of the player (not updated until the end of a hand).
    deck :: [Card] -- The active deck.
} deriving Show

data HandState = HandState {
    game_state :: GameState,

    bet :: Integer,
    player_hand :: [Card],
    dealer_hand :: [Card]
} deriving Show

-- Deals a random card from the active deck, switching to a new deck if required.
--
-- Uses this function is equivalent to shuffling the deck.
deal :: GameState -> (GameState, Card)
deal input_state = do
    let state = if (length $ deck input_state) == 0 then input_state { deck = full_deck } else input_state
    -- Generate random number and truncate it to the length of the remaininng cards in the deck.
    let index = fromIntegral $ (rng64 $ rng state) `mod` (fromIntegral $ length $ deck state)
    --- Update the GameState and return the selected card.
    (state { rng = rng64 $ rng state, deck = remove_at_index index $ deck state}, deck state !! index)

-- Handles the prompting for how much the player wants to bet.
prompt_bet :: GameState -> IO Integer
prompt_bet state = do
    putStrLn $ "Your current balance is " ++ (show $ balance state)
    bet_value <- prompt "Please place a bet: "
    if bet_value == 0 || bet_value > (balance state) then do
        putStrLn "Invalid bet: Try Again"
        prompt_bet state
    else return bet_value

-- Handles the prompting for how much the player's insurance bet will be.
prompt_insurance_bet :: HandState -> IO Integer
prompt_insurance_bet state = do
    putStrLn $ "Your current balance is " ++ (show $ balance $ game_state state)
    bet_value <- prompt "Please place an insurance bet (0 means none): "
    if bet_value > (balance $ game_state state) || bet_value > ((bet state) `div` 2) then do
        putStrLn "Invalid bet: Try Again"
        prompt_insurance_bet state
    else return bet_value

-- Handles the prompting for whether the player wants to double down.
prompt_double_down :: GameState -> IO String
prompt_double_down state = do
    putStrLn $ "Your current balance is " ++ (show $ balance state)
    putStr "Would you like to double down (yes/no): " >> hFlush stdout
    response <- getLine
    case response of
        "yes" -> return response
        "no" -> return response
        _ -> putStrLn "Invalid input." >> prompt_double_down state

-- Handles the initial dealing of the cards to the player and dealer.
initial_deal :: HandState -> HandState
initial_deal hand_state = do
    let (state0, player_card0) = deal $ game_state hand_state
    let (state1, dealer_card0) = deal state0
    let (state2, player_card1) = deal state1
    let (state3, dealer_card1) = deal state2
    hand_state {
        game_state = state3,
        player_hand = player_card0 : [player_card1],
        dealer_hand = dealer_card0 : [dealer_card1]
    }

-- Handles the repeated "hit or stand" question and results.
player_turn :: HandState -> IO HandState
player_turn hand_state
    | best_hand_value (player_hand hand_state) >= 21 = return hand_state -- Finish the turn if blackjack or busted
    | otherwise = do
        putStr "Please enter hit or stand: " >> hFlush stdout
        response <- getLine
        case response of
            "hit" -> do
                let (new_game_state, new_card) = deal (game_state hand_state) -- Deal a new card
                let new_hand_state = hand_state { -- Update game data.
                    game_state = new_game_state,
                    player_hand = (player_hand hand_state) ++ [new_card]
                }
                putStrLn $ "Your cards: " ++ showHand (player_hand new_hand_state) -- Display new cards to player.
                player_turn new_hand_state -- Move to next repetition.
            "stand" -> return hand_state
            _ -> putStrLn "Invalid Choice" >> player_turn hand_state

-- Handles the dealing the dealer's hand.
dealer_turn :: HandState -> IO HandState
dealer_turn hand_state
    -- Dealer must stand with 17 or more points (this also covers the bust situation.
    | best_hand_value (dealer_hand hand_state) >= 17 = return hand_state
    | otherwise = do
        let (new_game_state, new_card) = deal (game_state hand_state)
        let new_hand_state = hand_state {
            game_state = new_game_state,
            dealer_hand = (dealer_hand hand_state) ++ [new_card]
        }
        dealer_turn new_hand_state

-- Plays a single hand according to the rules of Blackjack.
play_hand :: GameState -> IO GameState
play_hand initial_game_state = do
    bet_value <- prompt_bet initial_game_state
    let initial_hand_state = HandState {
        game_state = initial_game_state { balance = balance initial_game_state - bet_value },
        bet = bet_value,
        player_hand = [],
        dealer_hand = []
    }
    let dealt_state = initial_deal initial_hand_state
    putStrLn $ "Dealer shows: " ++ (show $ dealer_hand dealt_state !! 0)
    putStrLn $ "Your cards: " ++ showHand (player_hand dealt_state)

    insurance_state <- if rank (dealer_hand dealt_state !! 0) == Ace then do
        putStrLn ""
        insurance <- prompt_insurance_bet dealt_state
        if insurance /= 0 then do
            -- They placed a bid.
            if card_value (dealer_hand dealt_state !! 1) == 10 then do
                putStrLn "Insurance Bet Successful"
                putStrLn $ "Your current balance is " ++ (show $ (balance $ game_state dealt_state) - insurance)
                putStrLn $ "Dealer's Hand: " ++ showHand (dealer_hand dealt_state)
                return dealt_state { game_state = (game_state dealt_state) {
                    balance = (balance $ game_state dealt_state) + insurance
                }}
            else do
                putStrLn "Insurance Bet Unsuccessful"
                putStrLn $ "Your current balance is " ++ (show $ (balance $ game_state dealt_state) - insurance)
                return dealt_state { game_state = (game_state dealt_state) {
                    balance = (balance $ game_state dealt_state) - insurance
                }}
        else return dealt_state
    else return dealt_state

    if best_hand_value (player_hand insurance_state) == 21 && best_hand_value (dealer_hand insurance_state) == 21 then do
        -- Handle dealer and player natural blackjack.
        putStrLn "Natural Blackjack Tie"
        putStrLn $ "Dealer's Hand: " ++ showHand (dealer_hand insurance_state)
        return (game_state insurance_state) {
            balance = balance (game_state insurance_state) + bet_value
        }
    else if best_hand_value (dealer_hand insurance_state) == 21 then do
        -- Handle dealer-only natural blackjack.
        putStrLn "Natural Dealer Blackjack"
        putStrLn $ "Dealer's Hand: " ++ showHand (dealer_hand insurance_state)
        return $ game_state insurance_state
    else if best_hand_value (player_hand insurance_state) == 21 then do
        -- Handle player natural blackjack.
        putStrLn "Natural Player Blackjack"
        putStrLn $ "Dealer's Hand: " ++ showHand (dealer_hand insurance_state)
        return (game_state insurance_state) {
            balance = fromIntegral (
                balance (game_state insurance_state)
                + (bet insurance_state) * 2
                + ((bet insurance_state) `div` 2)
            )
        }
    else do
        (double_down_state, can_play) <- if best_hand_value (player_hand insurance_state) >= 9
            && best_hand_value (player_hand insurance_state) <= 11
            && (balance $ game_state insurance_state) >= (bet insurance_state)
            then do
            -- Prompt whether doubling down.
            putStr "Would you like to double down (yes/no): " >> hFlush stdout
            response <- getLine
            case response of
                "yes" -> do
                    let (new_game_state, new_card) = deal (game_state insurance_state)
                    let new_hand_state = insurance_state {
                        game_state = new_game_state { balance = balance new_game_state - (bet insurance_state) },
                        player_hand = (player_hand insurance_state) ++ [new_card]
                    }
                    putStrLn $ "Your cards: " ++ showHand (player_hand new_hand_state)
                    return (new_hand_state, False)
                "no" -> return (insurance_state, True)
        else return (insurance_state, True)

        player_turn_finished_state <- if can_play then player_turn double_down_state else return double_down_state
        if best_hand_value (player_hand player_turn_finished_state) <= 21 then do
            final_hand_state <- dealer_turn player_turn_finished_state
            putStrLn $ "Dealer's Hand: " ++ showHand (dealer_hand final_hand_state)
            if best_hand_value (dealer_hand final_hand_state) > 21 then do
                putStrLn "Dealer went bust!"
                return (game_state final_hand_state) {
                    balance = balance (game_state final_hand_state) + (bet insurance_state) * 2
                }
            else if best_hand_value (dealer_hand final_hand_state) == best_hand_value (player_hand final_hand_state) then do
                putStrLn "Push!"
                return (game_state final_hand_state) {
                    balance = balance (game_state final_hand_state) + (bet insurance_state)
                }
            else if best_hand_value (dealer_hand final_hand_state) > best_hand_value (player_hand final_hand_state) then do
                putStrLn "Dealer wins!"
                return $ game_state final_hand_state
            else do
                putStrLn "You win!"
                return (game_state final_hand_state) {
                    balance = balance (game_state final_hand_state) + (bet insurance_state) * 2
                }

        else do
            -- Since there is only one player, if the player goes bust, we don't need to play
            -- the rest of the game out. We simply reveal the hidden card and move on.
            putStrLn "You went bust!"
            putStrLn $ "Dealer's Hand: " ++ showHand (dealer_hand insurance_state)
            return $ game_state player_turn_finished_state

-- Repeatedly plays hands, exiting when the player runs out of money.
run_game :: GameState -> IO GameState
run_game state = do
    state <- play_hand state
    if (balance state) > 0 then do
        putStrLn ""
        run_game state
    else putStrLn "" >> putStrLn "Game Over!" >> return state

-- Provides a simple way to call the Blackjack program without knowing anything about the program.
run :: IO ()
run = run_game GameState { rng = 29089, balance = 100, deck = full_deck } >> return ()
