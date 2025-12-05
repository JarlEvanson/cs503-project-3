module Main where

import GHC.IO.Handle
import System.IO
import Control.Exception

import Gameplay
import Cards

compareFileContents :: FilePath -> FilePath -> IO Bool
compareFileContents file1 file2 = do
    content1 <- readFile file1
    content2 <- readFile file2
    return (content1 == content2)

withRedirectedStdout :: FilePath -> IO a -> IO a
withRedirectedStdout filePath action =
    bracket (openFile filePath WriteMode) hClose $ \newHandle -> do
        oldHandle <- hDuplicate stdout
        hDuplicateTo newHandle stdout
        result <- action
        hDuplicateTo oldHandle stdout
        return result

withRedirectedStdin filePath action =
    bracket (openFile filePath ReadMode) hClose $ \newHandle -> do
        oldHandle <- hDuplicate stdin
        hDuplicateTo newHandle stdin
        result <- action
        hDuplicateTo oldHandle stdin
        return result

test_file :: String -> Integer -> IO ()
test_file name seed = do
    withRedirectedStdin 
        ("tests/" ++ name ++ ".input")
        (withRedirectedStdout 
            ("tests/" ++ name ++ ".output")
            (run_game GameState { rng = seed, balance = 100, deck = full_deck } >> return ())
        )
    are_equal <- compareFileContents ("tests/" ++ name ++ ".output") ("tests/" ++ name ++ ".expected")
    if are_equal then
        putStrLn $ "Test " ++ name ++ " succeeded"
    else
        putStrLn $ "Test " ++ name ++ " failed"

main :: IO ()
main = do
    test_file "general" 0
    test_file "insurance-decline" 19762
    test_file "insurance-accept-failure" 19762
    test_file "insurance-accept-success" 29089
    test_file "natural-player-blackjack" 8935347
    test_file "natural-dealer-blackjack" 8935370
    test_file "double-down-accept" 30579
    test_file "double-down-reject" 30579
    test_file "double-down-dont-show" 30579

-- Seed 19762: Insurance opportunity - failure
-- Seed 8935347: Player Natural blackjack
-- Seed 8935370: Dealer Natural blackjack
