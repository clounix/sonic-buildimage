#! /bin/bash
s3ip_start(){
    /usr/local/bin/s3ip_load.py create
    echo "s3ip service start"
}

s3ip_stop(){
    /usr/local/bin/s3ip_load.py remove
    echo "s3ip service stop"

}

case "$1" in
    start)
        s3ip_start
        ;;

    stop)
        s3ip_stop
        ;;

    status)
        tree -l /sys_switch
        ;;

    restart)
        s3ip_stop
        s3ip_start
        ;;

    *)
        echo "Usage: $0 {start|stop|status|restart}"
	exit 1
esac
exit

