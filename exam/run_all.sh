#! /bin/bash

if [ ! -d ../netsnap/ ]; then
    git clone https://github.com/zzjjzzgggg/netsnap.git ../netsnap
fi

cd ..

make

echo -e "\n\nsplitting graph into blocks ..."
read -p "Press [Enter] key to continue ..."
./genbits -g exam/hepth_be.gz -job 0

echo -e "\n\ngenerating bit-strings ..."
read -p "Press [Enter] key to continue ..."
./genbits -g exam/hepth_be.gz -job 1

echo -e "\n\nvalidating approximation accuracy..."
read -p "Press [Enter] key to continue ..."
./vali_measure -g exam/hepth_be.gz -job 2
./vali_measure -g exam/hepth_be.gz -job 1

echo -e "\n\nRun monotone IO-greedy algorithm ..."
read -p "Press [Enter] key to continue ..."
./vali_measure -g exam/hepth_be.gz -job 3 -a 0
./iogreedy -g exam/hepth_be.gz -job 0 -B 100 -a 0

echo -e "\n\nRun non-monotone IO-greedy algorithm ..."
read -p "Press [Enter] key to continue ..."
./vali_measure -g exam/hepth_be.gz -job 3 -a 1
./iogreedy -g exam/hepth_be.gz -job 1 -B 100 -a 1


echo "done."
