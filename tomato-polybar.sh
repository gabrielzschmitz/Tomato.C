#
#         .             .              .		    
#         |             |              |           .	    
# ,-. ,-. |-. ,-. . ,-. |  ,_, ,-. ,-. |-. ,-,-. . |- ,_,  
# | | ,-| | | |   | |-' |   /  `-. |   | | | | | | |   /   
# `-| `-^ ^-' '   ' `-' `' '"' `-' `-' ' ' ' ' ' ' `' '"'  
#  ,|							    
#  `'							    
# tomato-polybar.sh
#
#!/bin/sh

TIME="$(cat $HOME/.local/share/tomato/time.log)"

case $TIME in
    '00:00')
        echo ""
    ;;
    *)
        echo $TIME
    ;;
esac

