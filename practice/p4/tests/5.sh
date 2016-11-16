sleep 0.1 && false || echo running in background | cat &
echo pre
sleep 0.2
echo post

echo exit 71 |  bash &
sleep 0.1
