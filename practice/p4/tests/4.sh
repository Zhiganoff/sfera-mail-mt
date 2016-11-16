echo blabla > file.txt
echo aaa2 > file1.txt
cat file.txt file.txt file.txt file1.txt | sort | uniq

false | true && echo 1-true
true | false || echo 2-false

