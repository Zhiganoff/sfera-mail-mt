true&&echo 1-true 
false&&echo 1-false
true || echo 2-true
false   ||echo 2-false

true&&false&&echo 3-1
true||false&&echo 3-2

echo 123 && echo 456   &&echo 789

echo 123 > file.file && cat < file.file > file1.file && cat file1.file file1.file
