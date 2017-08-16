#!/bin/bash
op=0
threadnum=20

echo "======================== $threadnum thread ======================="

while [ $op -le 8 ];do
#echo ***********************************************************************
#echo "compile ..."
g++ -O3 -pthread -DOPERATION_TYPE=$op pstream.c -o pstream_exe

#echo "running pstream"

#echo "======================== 1 thread ========================"
#./pstream_exe 1
#echo
#
#echo "======================== 2 thread ========================"
#./pstream_exe 2
#echo

#echo "======================== 10 thread ======================="
#./pstream_exe 10
#echo

./pstream_exe $threadnum
echo
#echo

op=$((op+1))
done

echo "removing exe..."
rm pstream_exe
echo "removing done!"
