module Util where

import System.IO -- Imported for hFlush stdout to work

rng64 :: Integer -> Integer
rng64 seed = ((214013 * seed) + 2531011) `mod` (2 ^ 32)

remove_at_index :: Int -> [a] -> [a]
remove_at_index n xs
    | n < 0 || n >= length xs = undefined -- Crash if out of bounds index was selected
    | otherwise = take n xs ++ drop (n + 1) xs

prompt :: Read a => String -> IO a
prompt s = putStr s >> hFlush stdout >> readLn
