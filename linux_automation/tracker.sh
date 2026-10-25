#!/bin/bash

# Go to project file
cd /home/Vasilis/Desktop/Vasilis/Rating_Project

# Execute the program
./rate_24

# Check if today is Sunday ( date +%u returns 7 for Sunday)
if [ $(date +%u) -eq 7 ]; then
    echo ""
    echo "========================================"
    echo "           WEEKLY SUMMARY               "
    echo "========================================"

    # Read file, find last 7 "Total Day Score" and add them
    # awk isolates the number and returns the result
    WEEKLY_SCORE=$(grep "Total Day Score:" performance.txt | tail -n 7 | awk '{sum+=$4} END {print sum}')

    echo "Your total score for the last 7 days is: $WEEKLY_SCORE / 700"

    # Congratulations check
    if (( $(echo "$WEEKLY_SCORE >= 700.0" | bc -l) )); then
        echo "🔥 OUTSTANDING WEEK! You broke the 700 barrier! 🔥"
    else
        echo "Good job! Let's aim higher next week."
    fi
    echo "========================================"
fi

echo ""
echo "Press [ENTER] to close the terminal..."
read
