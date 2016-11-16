echo trap 'echo sigint ; exit' INT ; kill -INT $PPID ; sleep 0.1 ; echo still alive | bash 
echo done
