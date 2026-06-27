#!/bin/bash
set -e

RATE=48000
DURATION=3

echo "Generating test tones and playing through screamalsa (hw:0,0)..."

for FMT in S16_LE S24_3LE S24_LE S32_LE; do
    FILE=/tmp/sine_${FMT}.raw
    echo ""
    echo "=== Testing $FMT ==="

    case $FMT in
        S16_LE)
            sox -r $RATE -n -t raw -b 16 -e signed -L -c 2 $FILE synth $DURATION sine 1000
            ;;
        S24_3LE)
            sox -r $RATE -n -t raw -b 24 -e signed -L -c 2 $FILE synth $DURATION sine 1000
            ;;
        S24_LE)
            sox -r $RATE -n -t raw -b 24 -e signed -L -c 2 $FILE synth $DURATION sine 1000
            ;;
        S32_LE)
            sox -r $RATE -n -t raw -b 32 -e signed -L -c 2 $FILE synth $DURATION sine 1000
            ;;
    esac

    aplay -D hw:0,0 -f $FMT -r $RATE -c 2 $FILE -d $DURATION
done

echo ""
echo "All PCM format tests completed."
