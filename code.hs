module Main where
main :: IO ()

main = print $ blackjack [1, 1]

backjack :: [Int] -> Int

backjack list =
    ace list (sum $ map value list)

value = card
    | card < 11 = card
    | otherwise = 10

ace = list basescore
    | basescore > 11 = basescore
    | elem 1 list = basescore + 10
    | otherwise = basescore
