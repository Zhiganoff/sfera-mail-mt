pgc++ -o tabulation tabulation.cpp
./tabulation > test.out
pgc++ -acc -Minfo=accel -o tabulation_acc tabulation.cpp
./tabulation_acc >> test.out
cat test.out