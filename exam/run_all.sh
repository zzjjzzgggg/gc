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

echo -e "\n\nRun IO-greedy algorithm ..."
read -p "Press [Enter] key to continue ..."
./vali_measure -g exam/hepth_be.gz -job 3
./iogreedy -g exam/hepth_be.gz -job 0 -B 100

rm -rf exam/greedy/

echo "done."
