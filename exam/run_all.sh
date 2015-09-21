#! /bin/bash

echo -e "\n\nsplitting graph into blocks ..."
read -p "Press [Enter] key to continue ..."
./genbits -g hepth.gz -job 0

echo -e "\n\ngenerating bit-strings ..."
read -p "Press [Enter] key to continue ..."
./genbits -g hepth.gz -job 1

echo -e "\n\nvalidating approximation accuracy..."
read -p "Press [Enter] key to continue ..."
./iogreedy -g hepth.gz -job 0 -R 10
echo "Real  Approx  Approx/Real"

echo -e "\n\nRun IO-greedy algorithm ..."
read -p "Press [Enter] key to continue ..."
./iogreedy -g hepth.gz -job 1
./iogreedy -g hepth.gz -job 2 -B 100

echo "done."
