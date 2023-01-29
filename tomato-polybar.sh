#!/bin/sh

TIME="$(cat /home/gabrielzschmitz/.local/share/tomato/time.log)"

case $TIME in
    '00:00')
        echo ""
    ;;
    *)
        echo $TIME
    ;;
esac
